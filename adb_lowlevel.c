#include <linux/device.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/completion.h>

#include "adb_lowlevel.h"


int _adb_state;
enum {
	STATE_ADB_IDLE,
	STATE_ADB_WAITING_LO,
	STATE_ADB_TIMING_LO,
	STATE_ADB_TIMING_HI,
};

adb_packet* _adb_in;
static void _adb_set_bit(int8_t bit, uint8_t byte) {
	if (bit == -1) {
		return;
	}
	_adb_in->data[byte] |= (1 << (7-bit));
}


static struct hrtimer _adb_wdt;
ktime_t _adb_period;
uint8_t _adb_byte_counter;
int8_t _adb_bit_counter;
void _adb_timer_kick(void) {
	ktime_t rt;
	rt = ktime_add_ns(ktime_set(0, 0), ktime_to_ns(_adb_period));
	hrtimer_start(&_adb_wdt, rt, HRTIMER_MODE_REL);
}
void _adb_timer_start(void) {
	hrtimer_start(&_adb_wdt, _adb_period, HRTIMER_MODE_REL);
}
struct completion _adb_compl;
enum hrtimer_restart _adb_timer_func(struct hrtimer* timer) {
	switch (_adb_state) {
		case STATE_ADB_TIMING_HI:
			if (_adb_byte_counter) {
				_adb_in->error = ADB_ERROR_OK;
			} else {
				_adb_in->error = ADB_ERROR_UNKNOWN;
			}
			break;
		case STATE_ADB_WAITING_LO:
			_adb_in->error = ADB_ERROR_NO_DATA;
			break;
		default:
			_adb_in->error = ADB_ERROR_UNKNOWN;
			break;
	}
	_adb_in->len = _adb_byte_counter;
	_adb_state = STATE_ADB_IDLE;
	complete(&_adb_compl);
	return HRTIMER_NORESTART;
}


struct timeval _adb_tv_old;
long _adb_last_l;
static irqreturn_t adb_irq(int irq, void* data) {
	struct timeval tv;
	long deltat;
	do_gettimeofday(&tv);
	deltat = tv.tv_usec - _adb_tv_old.tv_usec;

	if (_adb_state == STATE_ADB_IDLE) {
		_adb_tv_old = tv;
		return IRQ_HANDLED;
	}

	if (!gpio_get_value(ADB_PIN)) {
		switch (_adb_state) {
			case STATE_ADB_WAITING_LO:
				_adb_state = STATE_ADB_TIMING_LO;
				break;

			case STATE_ADB_TIMING_HI:
				if (_adb_last_l < deltat) {
					_adb_set_bit(_adb_bit_counter, _adb_byte_counter);
				}
				_adb_bit_counter++;
				if (_adb_bit_counter == 8) {
					_adb_byte_counter++;
					_adb_bit_counter = 0;
				}
				_adb_timer_kick();
				_adb_state = STATE_ADB_TIMING_LO;
				break;
		}
	} else {
		switch (_adb_state) {
			case STATE_ADB_TIMING_LO:
				_adb_last_l = deltat;
				_adb_state = STATE_ADB_TIMING_HI;
				break;
		}
	}

	_adb_tv_old = tv;
	return IRQ_HANDLED;
}


static inline void _adb_pin_low(void) {
	gpio_direction_output(ADB_PIN, 0);
}
static inline void _adb_pin_high(void) {
	gpio_direction_input(ADB_PIN);
}
static inline void _adb_bit_0(void) {
	_adb_pin_low();
	udelay(65);
	//usleep_range(64/2, 66/2);
	_adb_pin_high();
	udelay(35);
	//usleep_range(34/2, 36/2);
}
static inline void _adb_bit_1(void) {
	_adb_pin_low();
	udelay(35);
	//usleep_range(34/2, 36/2);
	_adb_pin_high();
	udelay(65);
	//usleep_range(64/2, 66/2);
}
static inline void _adb_send_byte(char d) {
	int i;
	for (i = 0; i < 8; i++) {
		if (d & (0x80 >> i)) {
			_adb_bit_1();		
		} else {
			_adb_bit_0();
		}
	}
}


bool adb_transfer(adb_packet* p) {
	unsigned long flags;
	uint8_t cmd;
	uint8_t i;
	
	local_irq_save(flags);

	_adb_pin_low();
	usleep_range(710, 720); //for some reason usleep range is not really precise
	_adb_bit_1();
	_adb_send_byte(p->command);
	_adb_bit_0();

	cmd = p->command & 0x0c;
	switch(cmd) {
		case ADB_COMMAND_WRITE: {
			udelay(200);
			_adb_bit_1();
			for (i = 0; i < p->len; i++) {
				_adb_send_byte(p->data[i]);
			}
			_adb_bit_0();
			local_irq_restore(flags);	
			break;
		}
		case ADB_COMMAND_READ: {
			local_irq_restore(flags);
			_adb_in = p;
			_adb_bit_counter = -1;
			_adb_byte_counter = 0;
			_adb_state = STATE_ADB_WAITING_LO;
			reinit_completion(&_adb_compl);
			_adb_timer_start();

			wait_for_completion(&_adb_compl);
			if (p->error == ADB_ERROR_UNKNOWN) {
				return 0;
			}	
			break;
		}
		default: {
			local_irq_restore(flags);
			break;

		}
	}
	return 1;
}


int _adb_ival;
int adb_lowlevel_init(void) {
	int r;

	_adb_state = STATE_ADB_IDLE;

	r = gpio_request_one(ADB_PIN, GPIOF_DIR_IN, "adb_gpio");
	if (r < 0) {
		return r;
	}	
	
	r = gpio_request_one(ADB_PIN_IRQ, GPIOF_DIR_IN, "adb_gpio");
	if (r < 0) {
		gpio_free(ADB_PIN);
		return r;
	}
	
	r = gpio_to_irq(ADB_PIN_IRQ);
	if (r <= 0) {	
		gpio_free(ADB_PIN);
		gpio_free(ADB_PIN_IRQ);
		return r;
	}

	_adb_ival = r;
	r = request_irq(_adb_ival, adb_irq, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "adb_irq", NULL);
	if (r != 0) {
		gpio_free(ADB_PIN);
		gpio_free(ADB_PIN_IRQ);
		return r;
	}

	init_completion(&_adb_compl);

	hrtimer_init(&_adb_wdt, CLOCK_REALTIME, HRTIMER_MODE_REL);
	_adb_period = ktime_set(0, 500000);
	_adb_wdt.function = _adb_timer_func;
	
	_adb_pin_high();

	msleep(2000);

	return 1;
}
void adb_lowlevel_exit(void) {
	free_irq(_adb_ival, NULL);
	gpio_free(ADB_PIN);
	gpio_free(ADB_PIN_IRQ);
	hrtimer_cancel(&_adb_wdt);
}
