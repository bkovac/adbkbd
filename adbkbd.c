#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/jiffies.h>
#include <linux/delay.h>

#include "adb_lowlevel.h"
#include "adb_hid.h"


static struct workqueue_struct* adbkbd_wq;
static struct delayed_work adbkbd_dw;
static bool adbkbd_has_died = false;
static void adbkbd_worker(struct work_struct* work) {
	adb_hid_poll();
	if (!adbkbd_has_died) {
		queue_delayed_work(adbkbd_wq, &adbkbd_dw, msecs_to_jiffies(15));
	}
}

int __init adbkbd_init(void) {
	int r;

	r = adb_lowlevel_init();
	if (r != 1) {
		return -EAGAIN;
	}

	r = adb_hid_init(ADB_HID_ENABLE_KEYBOARD);
	if (r != 1) {
		return -EAGAIN;
	}

	adbkbd_wq = create_workqueue("adbkbd");
	INIT_DELAYED_WORK(&adbkbd_dw, adbkbd_worker);
	queue_delayed_work(adbkbd_wq, &adbkbd_dw, msecs_to_jiffies(15)); //Polling delay can be changed here	

	printk(KERN_INFO "adbkbd kernel module attached!\n");
	return 0;
}

void __exit adb_exit(void) {
	adbkbd_has_died = true;
	cancel_delayed_work_sync(&adbkbd_dw);
	flush_workqueue(adbkbd_wq);
	destroy_workqueue(adbkbd_wq);

	adb_lowlevel_exit();

	adb_hid_exit();

	printk(KERN_INFO "adbkbd kernel module detached!\n");
}

MODULE_LICENSE("GPL");
module_init(adbkbd_init);
module_exit(adb_exit);
