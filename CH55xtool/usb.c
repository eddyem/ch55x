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

#include <libusb.h>
#include <stdio.h>
#include <string.h>
#include <usefull_macros.h>
#include "usb.h"

#define DETECT_CHIP_LEN         (6)
#define GETVER_LEN              (30)
#define SENDKEYLEN              (6)
#define ERASELEN                (6)
#define WRITELEN                (6)
#define WRITEPACKETLEN          (56)
#define WRITEVERIFYSZ           (64)
#define FIXLEN                  (6)
static uint8_t DETECT_CHIP_CMD_V2[] = "\xA1\x12\x00\x52\x11MCU ISP & WCH.CN";
static uint8_t READ_CFG_CMD_V2[] = {0xa7, 0x02, 0x00, 0x1f, 0x00};
static uint8_t SEND_KEY_CMD_V230[48] = {0xa3, 0x30, 0x00};
static uint8_t SEND_KEY_CMD_V240[56] = {0xa3, 0x38, 0x00};
static uint8_t ERASE_CHIP_CMD_V2[] = {0xa4, 0x01, 0x00, 0x08};
static uint8_t WRITE_CMD_V2[64] = {0xa5, 0x00};
static uint8_t VERIFY_CMD_V2[64] = {0xa6, 0x00};
static uint8_t END_FLASH_CMD_V2[] = {0xa2, 0x01, 0x00, 0x00};
static uint8_t RESET_RUN_CMD_V2[] = {0xa2, 0x01, 0x00, 0x01};

static const ch55descr devlist[] = {
    {"CH551", 10240, 0x51},
    {"CH552", 16384, 0x52},
    {"CH553", 10240, 0x53},
    {"CH554", 14336, 0x54},
    {"CH559", 61440, 0x59},
    {NULL, 0, 0}
};

static uint8_t buf[64];
uint8_t *getusbbuf(){
    return buf;
}

static libusb_device_handle *devh = NULL;
int usbcmd(const uint8_t *data, int olen, int ilen){
    FNAME();
    static libusb_context *ctx = NULL;
    int inum, onum;
    if(!olen) return 0;
    if(!devh){
        if(libusb_init(&ctx)) ERR("libusb_init()");
        devh = libusb_open_device_with_vid_pid(ctx, CH55VID, CH55PID);
        if(!devh) ERR(_("No devices found"));
        if(libusb_claim_interface(devh, 0)) ERR("libusb_claim_interface()");
    }
#ifdef EBUG
    green("usbcmd() send:\n");
    for(int i = 0; i < olen; ++i){
        printf("0x%02X ", data[i]);
    }
    printf("\n");
#endif
    int oret = libusb_bulk_transfer(devh, EPOUT, (unsigned char*)data, olen, &onum, USB_TIMEOUT);
    int iret = libusb_bulk_transfer(devh, EPIN, buf, ilen, &inum, USB_TIMEOUT);
    if(oret || iret || onum != olen || inum != ilen){
        WARN("libusb_bulk_transfer()");
        libusb_release_interface(devh, 0);
        libusb_close(devh);
        libusb_exit(NULL);
        exit(3);
    }
#ifdef EBUG
    green("usbcmd() got:\n");
    for(int i = 0; i < inum; ++i){
        printf("0x%02X ", buf[i]);
    }
    printf("\n");
#endif
    return inum;
}

static uint8_t chipid = 0;
const ch55descr *detect_chip(){
    int got = usbcmd(DETECT_CHIP_CMD_V2, sizeof(DETECT_CHIP_CMD_V2)-1, DETECT_CHIP_LEN);
    if(DETECT_CHIP_LEN != got) return NULL;
    const ch55descr *ptr = devlist;
    while(ptr->devname){
        if(buf[4] == ptr->chipid){
            chipid = ptr->chipid;
            return ptr;
        }
        ++ptr;
    }
    return NULL;
}


static int old = -1; // ==1 for V2.30, ==0 for V2.31 or V2.40
static uint8_t chk_sum = 0;
/**
 * @brief getver - get version string
 * @return version or NULL if failed
 */
char *getver(){
    static char v[32];
    if(GETVER_LEN != usbcmd(READ_CFG_CMD_V2, sizeof(READ_CFG_CMD_V2), GETVER_LEN)) return NULL;
    snprintf(v, 32, "V%d.%d%d", buf[19], buf[20], buf[21]);
    int s = buf[22] + buf[23] + buf[24] + buf[25];
    chk_sum = s&0xff;
    DBG("chk_sum=0x%02X", chk_sum);
    if(strcmp(v, "V2.30") == 0){ // ver 2.30, sendkey
        for(int i = 3; i < 48; ++i) SEND_KEY_CMD_V230[i] = chk_sum;
        DBG("Write key");
        if(SENDKEYLEN != usbcmd(SEND_KEY_CMD_V230, sizeof(SEND_KEY_CMD_V230), SENDKEYLEN)) return NULL;
        if(buf[3]) return NULL;
        old = 1;
    }else if(strcmp(v, "V2.31") == 0 || strcmp(v, "V2.40") == 0){
        if(SENDKEYLEN != usbcmd(SEND_KEY_CMD_V240, sizeof(SEND_KEY_CMD_V240), SENDKEYLEN)) return NULL;
        if(buf[3]) return NULL;
        old = 0;
    }else{
        WARNX(_("Version %s not supported\n"), v);
        return NULL;
    }
    return v;
}

int erasechip(){
    if(ERASELEN != usbcmd(ERASE_CHIP_CMD_V2, sizeof(ERASE_CHIP_CMD_V2), ERASELEN)) return 1;
    if(buf[3]) return 2;
    return 0;
}

static int writeverify(char *filename, uint8_t *cmd){
    uint8_t packet[WRITEPACKETLEN];
    if(old < 0) ERRX(_("Wrong getver()?"));
    if(!chipid) ERRX(_("Wrong detect_chip()?"));
    FILE *f = fopen(filename, "r");
    if(!f) ERR(_("Can't open %s"), filename);
    size_t n = 0, curr_addr = 0;
    do{
        memset(packet, 0, WRITEPACKETLEN);
        if(!(n = fread(packet, 1, WRITEPACKETLEN, f))) break;
        n = WRITEPACKETLEN;
        for(size_t i = 0; i < n; i++){
            if(i % 8 == 7) packet[i] = packet[i] ^ ((chk_sum + chipid) & 0xff);
            else if(old == 0) packet[i] ^= chk_sum;
        }
        cmd[1] = (n + 5) & 0xff;
        cmd[3] = curr_addr & 0xff;
        cmd[4] = (curr_addr >> 8) & 0xff;
        cmd[7] = 56;//n & 0xff;
        curr_addr += n;
        memcpy(&cmd[8], packet, n);
        if(WRITELEN != usbcmd(cmd, n+8, WRITELEN)){
            fclose(f);
            return 1;
        }
        if(buf[4]) WARNX("buf[4]==0x%02X", buf[4]);
    }while(n == WRITEPACKETLEN);
    fclose(f);
    return 0;
}

int writeflash(char *filename){
    return writeverify(filename, WRITE_CMD_V2);
}

int verifyflash(char *filename){
    return writeverify(filename, VERIFY_CMD_V2);
}

int endflash(){
    if(FIXLEN != usbcmd(END_FLASH_CMD_V2, sizeof(END_FLASH_CMD_V2), FIXLEN) || buf[4]) return 1;
    return 0;
}

void restart(){
    int onum;
    libusb_bulk_transfer(devh, EPOUT, (unsigned char*)RESET_RUN_CMD_V2, sizeof(RESET_RUN_CMD_V2), &onum, USB_TIMEOUT);
}

