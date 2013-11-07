#ifndef _maplebus_h__
#define _maplebus_h__

/* Most of the information here is from 
 * http://mc.pp.se/dc/maplebus.html
 */

#define MAPLE_CMD_RQ_DEV_INFO		1
#define MAPLE_CMD_RQ_EXT_DEV_INFO	2
#define MAPLE_CMD_RESET_DEVICE		3
#define MAPLE_CMD_SHUTDOWN_DEV		4
#define MAPLE_CMD_GET_CONDITION		9
#define MAPLE_CMD_BLOCK_WRITE		12

#define MAPLE_FUNC_CONTROLLER	0x001
#define MAPLE_FUNC_MEMCARD		0x002
#define MAPLE_FUNC_LCD			0x004
#define MAPLE_FUNC_CLOCK		0x008
#define MAPLE_FUNC_MIC			0x010
#define MAPLE_FUNC_AR_GUN		0x020
#define MAPLE_FUNC_KEYBOARD		0x040
#define MAPLE_FUNC_LIGHT_GUN	0x080
#define MAPLE_FUNC_PURUPURU		0x100
#define MAPLE_FUNC_MOUSE		0x200

#define MAPLE_ADDR_PORT(id)		((id)<<6)
#define MAPLE_ADDR_PORTA		MAPLE_ADDR_PORT(0)
#define MAPLE_ADDR_PORTB		MAPLE_ADDR_PORT(1)
#define MAPLE_ADDR_PORTC		MAPLE_ADDR_PORT(2)
#define MAPLE_ADDR_PORTD		MAPLE_ADDR_PORT(3)
#define MAPLE_ADDR_MAIN			0x20
#define MAPLE_ADDR_SUB(id)		((1)<<id) /* where id is 0 to 4 */

#define MAPLE_DC_ADDR	0
#define MAPLE_HEADER(cmd,dst_addr,src_addr,len)	( (((cmd)&0xfful)<<24) | (((dst_addr)&0xfful)<<16) | (((src_addr)&0xfful)<<8) | ((len)&0xff))

void maple_init(void);

void maple_sendFrame(uint8_t cmd, uint8_t dst_addr, uint8_t src_addr, int data_len, uint8_t *data);
void maple_sendFrame1W(uint8_t cmd, uint8_t dst_addr, uint8_t src_addr, uint32_t data);
int maple_receiveFrame(uint8_t *data, unsigned int maxlen);

void maple_sendRaw(uint8_t *data, unsigned char len);

void maple_sendFrame_P(uint8_t cmd, uint8_t dst_addr, uint8_t src_addr, int data_len, PGM_P data);

#endif // _maplebus_h__
