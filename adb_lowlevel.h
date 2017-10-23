#ifndef _ADB_LOWLEVEL_H_
#define _ADB_LOWLEVEL_H_

#define ADB_PIN 18
#define ADB_PIN_IRQ 15


enum {
	ADB_COMMAND_RESET = 0x00,
	ADB_COMMAND_FLUSH = 0x01,
	ADB_COMMAND_WRITE = 0x08,
	ADB_COMMAND_READ = 0x0c,
};

enum {
	ADB_ERROR_NO_DATA = 1,
	ADB_ERROR_UNKNOWN,
	ADB_ERROR_OK,
};

typedef struct {
	uint8_t command;
	uint8_t len;
	uint8_t data[8];
	uint8_t error;
} adb_packet; 


int adb_lowlevel_init(void);
void adb_lowlevel_exit(void);

//Returns true on success
bool adb_transfer(adb_packet* p);

#endif
