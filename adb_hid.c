#include <linux/module.h>
#include <linux/input.h>

#include "adb_hid.h"
#include "adb_lowlevel.h"


enum {
	ADB_HID_KEYBOARD_ADDR = 2,
	ADB_HID_KEYBOARD_REG_0 = 0,
	ADB_HID_KEYBOARD_REG_1 = 1,
	ADB_HID_KEYBOARD_REG_2 = 2,
	ADB_HID_KEYBOARD_REG_3 = 3,
};

extern uint16_t adb_hid_codes[128];

struct input_dev* adb_hid_idev;
bool adb_hid_init(uint8_t flags) {
	int i;

	adb_hid_idev = input_allocate_device();

	adb_hid_idev->name = "adbkbd";
	adb_hid_idev->phys = "adb/input0";
	adb_hid_idev->id.bustype = BUS_USB;//BUS_HOST;
	adb_hid_idev->id.vendor = 1;
	adb_hid_idev->id.product = 1;
	adb_hid_idev->id.version = 1;
	adb_hid_idev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_REP);

	adb_hid_idev->keycode = adb_hid_codes;
	
	if (flags != ADB_HID_ENABLE_KEYBOARD) {
		//Only keyboard handling is implemented for now
		//TODO: pretty this up when adding support for more devices
		return false;
	}

	adb_hid_idev->keycodesize = sizeof(uint16_t);
	adb_hid_idev->keycodemax = sizeof(adb_hid_codes)/sizeof(uint16_t);
	for (i = 1; i < 0x256; i++) {
		set_bit(i, adb_hid_idev->keybit);
	}

	if (input_register_device(adb_hid_idev) != 0) {
		return false;
	}

	input_report_key(adb_hid_idev, KEY_NUMLOCK, 1);
	input_sync(adb_hid_idev);



	adb_packet p = {
		.command = (ADB_COMMAND_READ | (ADB_HID_KEYBOARD_ADDR << 4)| ADB_HID_KEYBOARD_REG_3),
	};
	if (!adb_transfer(&p)) {
		return false;
	}
	i = 0;
	switch (p.data[0]) {
		case 0x04: case 0x05: case 0x07: case 0x09: case 0x0D:
		case 0x11: case 0x14: case 0x19: case 0x1D: case 0xC1:
		case 0xC4: case 0xC7:			
			i = 1;
			break;
	}
	printk("iso: %d", i);
	if (i) {
		i = adb_hid_codes[10];
		adb_hid_codes[10] = adb_hid_codes[50];
		adb_hid_codes[50] = i;	
	}

	return true;
}



void _adb_hid_scan_key(uint8_t k) {
	uint8_t keycode, flags;
	keycode = k & 0x7f;
	flags = k & 0x80;
	printk("pollcina %d\n", k);
	int key = adb_hid_codes[keycode];
	if (key) {
		input_report_key(adb_hid_idev, key, !flags);
		input_sync(adb_hid_idev);
	}
}
void _adb_hid_set_led(uint8_t l) {
	adb_packet lp = {
		.command = (ADB_COMMAND_WRITE | (ADB_HID_KEYBOARD_ADDR << 4) | ADB_HID_KEYBOARD_REG_2),
		.len = 2,
		.data = {0, (~l) & 0x07},
	};
	adb_transfer(&lp);
}
bool _adb_hid_tmp = false;
void adb_hid_poll(void) {
	if (!_adb_hid_tmp) {
		_adb_hid_tmp = true;
		adb_packet ip = {
			.command = (ADB_COMMAND_WRITE | (ADB_HID_KEYBOARD_ADDR << 4 ) | ADB_HID_KEYBOARD_REG_3),
			.len = 2,
			.data = {ADB_HID_KEYBOARD_ADDR, 3}, //3 is the handler extended protocol
		};
		adb_transfer(&ip);
	} else {
		adb_packet kpp = {
			.command = (ADB_COMMAND_READ | (ADB_HID_KEYBOARD_ADDR << 4) | ADB_HID_KEYBOARD_REG_0),
		};
		if (!adb_transfer(&kpp)) {
			_adb_hid_tmp = false;
			return;
		}
		if (kpp.error == ADB_ERROR_NO_DATA) {
			return;
		}
		if (kpp.len == 2) {
			_adb_hid_scan_key(kpp.data[0]);
			if (kpp.data[1] != 0xff) {
				_adb_hid_scan_key(kpp.data[1]);
			}
		}
	}	

}
void adb_hid_exit(void) {
	input_unregister_device(adb_hid_idev);
}

uint16_t adb_hid_codes[128] = {
	KEY_A, 
	KEY_S, 
	KEY_D, 
	KEY_F, 
	KEY_H, 
	KEY_G, 
	KEY_Z, 
	KEY_X, 
	KEY_C, 
	KEY_V, 
	KEY_102ND, 
	KEY_B,
	KEY_Q,
	KEY_W,
	KEY_E,
	KEY_R,
	KEY_Y,
	KEY_T,
	KEY_1,
	KEY_2,
	KEY_3,
	KEY_4,
	KEY_6,
	KEY_5,
	KEY_EQUAL, 
	KEY_9,
	KEY_7,	
	KEY_MINUS,
	KEY_8,	
	KEY_0,	
	KEY_RIGHTBRACE,	
	KEY_O,	
	KEY_U,	
	KEY_LEFTBRACE,
	KEY_I,	
	KEY_P,	
	KEY_ENTER,
	KEY_L,	
	KEY_J,	
	KEY_APOSTROPHE,
	KEY_K,
	KEY_SEMICOLON,
	KEY_BACKSLASH,
	KEY_COMMA,
	KEY_SLASH,
	KEY_N,		
	KEY_M,
	KEY_DOT,
	KEY_TAB,
	KEY_SPACE,
	KEY_GRAVE,
	KEY_BACKSPACE,
	KEY_KPENTER,
	KEY_ESC,
	KEY_LEFTCTRL,
	KEY_LEFTMETA,
	KEY_LEFTSHIFT,
	KEY_CAPSLOCK,
	KEY_LEFTALT,
	KEY_LEFT, 
	KEY_RIGHT, 
	KEY_DOWN, 
	KEY_UP,	
	KEY_FN,	
	0,
	KEY_KPDOT,
	0,
	KEY_KPASTERISK,	
	0,
	KEY_KPPLUS, 
	0,
	KEY_NUMLOCK,
	0,
	0,
	0,
	KEY_KPSLASH, 
	KEY_KPENTER,
	0,
	KEY_KPMINUS,
	0,
	0,
	KEY_KPEQUAL,
	KEY_KP0,
	KEY_KP1,
	KEY_KP2,
	KEY_KP3, 
	KEY_KP4, 
	KEY_KP5, 
	KEY_KP6, 
	KEY_KP7,
	0,
	KEY_KP8, 
	KEY_KP9, 
	KEY_YEN, 
	KEY_RO,	
	KEY_KPCOMMA, 
	KEY_F5,
	KEY_F6,	
	KEY_F7,	
	KEY_F3,
	KEY_F8,
	KEY_F9,
	KEY_HANJA,
	KEY_F11, 
	KEY_HANGEUL,
	KEY_SYSRQ,
	0,
	KEY_SCROLLLOCK,	
	0,
	KEY_F10, 
	KEY_COMPOSE,
	KEY_F12, 
	0,
	KEY_PAUSE,
	KEY_INSERT,
	KEY_HOME, 
	KEY_PAGEUP, 
	KEY_DELETE, 
	KEY_F4,	
	KEY_END, 
	KEY_F2,	
	KEY_PAGEDOWN,
	KEY_F1,	
	KEY_RIGHTSHIFT,	
	KEY_RIGHTALT,
	KEY_RIGHTCTRL,
	KEY_RIGHTMETA,	
	KEY_POWER,
};


