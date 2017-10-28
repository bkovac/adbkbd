#ifndef _ADB_HID_H_
#define _ADB_HID_H_

//Flags to enable different device handlers
#define ADB_HID_ENABLE_KEYBOARD 0x01


typedef struct {
	uint8_t type;
	uint8_t addr;
	bool move;
} adb_hid_config;


//adb_lowlevel_init should be called before this
bool adb_hid_init(uint8_t flags);
void adb_hid_exit(void);

void adb_hid_poll(void);


#endif
