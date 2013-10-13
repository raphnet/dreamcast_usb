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

static uint32_t request_device_info[1] = { 0x01200000 };
static uint32_t get_condition[2] = { 0x09200001, 0x00000001 };

static void dcInit(void)
{
	dcUpdate();

	dcGamepad.reportDescriptor = (void*)dcPadReport;
	dcGamepad.reportDescriptorSize = sizeof(dcPadReport);
	
	dcGamepad.deviceDescriptor = (void*)dcPadDevDesc;
	dcGamepad.deviceDescriptorSize = sizeof(dcPadDevDesc);
	
	maple_sendFrame(request_device_info, 1);
}

static void dcReadPad(void)
{
	unsigned char tmp[30];
	static unsigned char a;
	int v;
	unsigned char lrc = 0;
	int i;
	
again:

	maple_sendFrame(get_condition, 2);
	v = maple_receivePacket(tmp, 30);

	if (v<0)
		return;
	
	// 8 : Left trigger
	// 9 : Right trigger
	// 10 : Buttons
	// 11 : Buttons
	// 12 : Joy Y 2
	// 13 : Joy X 2
	// 14 : Y axis
	// 15 : X axis
	//

	last_built_report[0][0] = tmp[15]-1;
	last_built_report[0][1] = tmp[14]-1;
	last_built_report[0][2] = tmp[10] ^ 0xff;
	last_built_report[0][3] = tmp[11] ^ 0xff;
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
