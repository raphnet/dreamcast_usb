#ifndef _maplebus_h__
#define _maplebus_h__

#include <stdint.h>

void maple_init(void);
void maple_sendPacket(unsigned char *data, unsigned char len);
void maple_sendFrame(uint32_t *words, unsigned char nwords);

int maple_receivePacket(unsigned char *data, unsigned int maxlen);

#endif // _maplebus_h__
