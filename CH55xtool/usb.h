/*
 * This file is part of the CH55tool project.
 * Copyright 2020 Edward V. Emelianov <edward.emelianoff@gmail.com>.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once
#ifndef USB_H__
#define USB_H__

#include <stdint.h>
#include <stdlib.h>

#define CH55VID     (0x4348)
#define CH55PID     (0x55E0)
#define EPOUT       (0x02)
#define EPIN        (0x82)
#define USB_TIMEOUT (2000)

typedef struct{
    char *devname;          // device name
    uint16_t flash_size;    // flash size
    uint8_t chipid;         // chip ID
} ch55descr;

uint8_t *getusbbuf();
int usbcmd (const uint8_t *data, int olen, int ilen);
const ch55descr *detect_chip();
char *getver();
int erasechip();
int writeflash(char *filename);
int verifyflash(char *filename);
int endflash();
void restart();

#endif // USB_H__
