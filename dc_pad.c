/* Saturn to USB : Sega dc controllers to USB adapter
 * Copyright (C) 2011-2013 Raphaël Assénat
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * The author may be contacted at raph@raphnet.net
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <string.h>
#include "usbdrv.h"
#include "gamepad.h"
#include "dc_pad.h"
#include "maplebus.h"

#define MAX_REPORT_SIZE			4
#define NUM_REPORTS				1

// report matching the most recent bytes from the controller
static unsigned char last_built_report[NUM_REPORTS][MAX_REPORT_SIZE];

// the most recently reported bytes
static unsigned char last_sent_report[NUM_REPORTS][MAX_REPORT_SIZE];

static char report_sizes[NUM_REPORTS] = { 4 };
static Gamepad dcGamepad;

static void dcUpdate(void);

/*
 * [0] X
 * [1] Y
 * [2] Btn 0-7
 * [3] Btn 8-15 
 */
static const unsigned char dcPadReport[] PROGMEM = {
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x05,                    // USAGE (Game pad)
    0xa1, 0x01,                    // COLLECTION (Application)
	0x09, 0x01,                    //   USAGE (Pointer)    
	0xa1, 0x00,                    //   COLLECTION (Physical)
    0x09, 0x30,                    //     USAGE (X)
    0x09, 0x31,                    //     USAGE (Y)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x26, 0xff, 0x00,              //     LOGICAL_MAXIMUM (255)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x95, 0x02,                    //   REPORT_COUNT (2)
    0x81, 0x02,                    //   INPUT (Data,Var,Abs)
	0x05, 0x09,                    // USAGE_PAGE (Button)
    0x19, 0x01,                    //   USAGE_MINIMUM (Button 1)
    0x29, 0x10,                    //   USAGE_MAXIMUM (Button 16)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
    0x75, 0x01,                    // REPORT_SIZE (1)
    0x95, 0x10,                    // REPORT_COUNT (16)
    0x81, 0x02,                    // INPUT (Data,Var,Abs)
    0xc0,                          // END_COLLECTION
    0xc0,                          // END_COLLECTION
};

const unsigned char dcPadDevDesc[] PROGMEM = {    /* USB device descriptor */
    18,         /* sizeof(usbDescrDevice): length of descriptor in bytes */
    USBDESCR_DEVICE,    /* descriptor type */
    0x01, 0x01, /* USB version supported */
    USB_CFG_DEVICE_CLASS,
    USB_CFG_DEVICE_SUBCLASS,
    0,          /* protocol */
    8,          /* max packet size */
	0x9B, 0x28,	// Vendor ID
    0x08, 0x00, // Product ID
	0x00, 0x01, // Version: Minor, Major
	1, // Manufacturer String
	2, // Product string
	3, // Serial number string
    1, /* number of configurations */
};


static void dcInit(void)
{
	dcUpdate();

	dcGamepad.reportDescriptor = (void*)dcPadReport;
	dcGamepad.reportDescriptorSize = sizeof(dcPadReport);
	
	dcGamepad.deviceDescriptor = (void*)dcPadDevDesc;
	dcGamepad.deviceDescriptorSize = sizeof(dcPadDevDesc);
	
}

#define MAX_ERRORS 10

#define STATE_GET_INFO	0
#define STATE_WRITE_LCD	1
#define STATE_READ_PAD	2

#define MAPLE_CMD_RQ_DEV_INFO	1
#define MAPLE_CMD_RESET_DEVICE	3
#define MAPLE_CMD_GET_CONDITION	9
#define MAPLE_CMD_BLOCK_WRITE	12

#define MAPLE_ADDR_PORT(id)		((id)<<6)
#define MAPLE_ADDR_PORTA		MAPLE_ADDR_PORT(0)
#define MAPLE_ADDR_PORTB		MAPLE_ADDR_PORT(1)
#define MAPLE_ADDR_PORTC		MAPLE_ADDR_PORT(2)
#define MAPLE_ADDR_PORTD		MAPLE_ADDR_PORT(3)
#define MAPLE_ADDR_MAIN			0x20
#define MAPLE_ADDR_SUB(id)		((1)<<id) /* where id is 0 to 4 */

#define MAPLE_DC_ADDR	0
#define MAPLE_HEADER(cmd,dst_addr,src_addr,len)	( (((cmd)&0xfful)<<24) | (((dst_addr)&0xfful)<<16) | (((src_addr)&0xfful)<<8) | ((len)&0xff))

static uint32_t request_device_info[1] = { MAPLE_HEADER(MAPLE_CMD_RQ_DEV_INFO, MAPLE_ADDR_MAIN | MAPLE_ADDR_PORTA, MAPLE_DC_ADDR, 0) };
static uint32_t get_condition[2] = { 0x09200001, 0x00000001 };
static uint32_t block_write_lcd[4] = { 0x0c300003, 0x004, 0,  0xf0f0f0f0 };

static void dcReadPad(void)
{
	static unsigned char state = STATE_GET_INFO;
	static unsigned char err_count = 0;
	static unsigned char controller_address = 0;
	unsigned char tmp[30];
	int v;

	switch (state)
	{
		case STATE_GET_INFO:
			maple_sendFrame(request_device_info, 1);
			v = maple_receivePacket(tmp, 30);
			_delay_ms(2);
			if (v==-2) {
				controller_address = tmp[1]; // sender address
				state = STATE_WRITE_LCD;
			}
			break;

		case STATE_WRITE_LCD:
			{
				int i;

				for (i=0; i<5; i++)
				{
					uint32_t request_device_info1[1] = { MAPLE_HEADER(MAPLE_CMD_RQ_DEV_INFO, MAPLE_ADDR_PORTA | MAPLE_ADDR_SUB(i), MAPLE_DC_ADDR, 0) };

					if (controller_address & (1<<i)) {
						maple_sendFrame(request_device_info1, 1);
						_delay_us(300);
					}

				}

			}
			break;

		case STATE_READ_PAD:
			maple_sendFrame(get_condition, 2);
			v = maple_receivePacket(tmp, 30);
			if (v<=0) {
				err_count++;
				if (err_count > MAX_ERRORS)
					state = STATE_GET_INFO;
				return;
			}
			err_count = 0;
	
	
			// 8 : Buttons
			// 9 : Buttons
			// 10 : R trig
			// 11 : L trig
			// 12 : Joy X axis
			// 13 : Joy Y axis
			// 14 : Joy X2 axis
			// 15 : Joy Y2 axis
			last_built_report[0][0] = tmp[12]-1;
			last_built_report[0][1] = tmp[13]-1;
			last_built_report[0][2] = tmp[8] ^ 0xff;
			last_built_report[0][3] = tmp[9] ^ 0xff;
			break;
	}
}

static void dcUpdate(void)
{
	dcReadPad();
}

static char dcBuildReport(unsigned char *reportBuffer, unsigned char report_id)
{
	report_id = 0;

	if (reportBuffer != NULL)
	{
		memcpy(reportBuffer, last_built_report[report_id], report_sizes[report_id]);
	}
	memcpy(last_sent_report[report_id], last_built_report[report_id], 
			report_sizes[report_id]);	

	return report_sizes[report_id];
}

static char dcChanged(unsigned char report_id)
{
	report_id = 0;

	return memcmp(last_built_report[report_id], last_sent_report[report_id], 
					report_sizes[report_id]);
}

static Gamepad dcGamepad = {
	num_reports: 		1,
	init: 				dcInit,
	update: 			dcUpdate,
	changed:			dcChanged,
	buildReport:		dcBuildReport
};

Gamepad *dcGetGamepad(void)
{
	return &dcGamepad;
}
