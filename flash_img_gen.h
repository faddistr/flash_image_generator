#ifndef _FLASH_IMG_GEN_H_
#define _FLASH_IMG_GEN_H_
#include <stdint.h>

#define MAX_FILE_NAME 27

typedef struct
{
	int8_t   fname[MAX_FILE_NAME + 1];
	uint32_t offset;
} __attribute__((packed)) tHeaderRow;

#endif