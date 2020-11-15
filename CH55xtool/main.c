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

#include "cmdlnopts.h"
#include "usb.h"

#include <signal.h>         // signal
#include <stdio.h>          // printf
#include <stdlib.h>         // exit, free
#include <string.h>         // strdup
#include <sys/types.h>      // pid_t
#include <unistd.h>         // sleep
#include <usefull_macros.h>

static glob_pars *GP = NULL;  // for GP->pidfile need in `signals`

/**
 * We REDEFINE the default WEAK function of signal processing
 */
void signals(int sig){
    if(sig){
        signal(sig, SIG_IGN);
        DBG("Get signal %d, quit.\n", sig);
    }
    if(GP->pidfile) // remove unnesessary PID file
        unlink(GP->pidfile);
    restore_console();
    exit(sig);
}

void iffound_default(pid_t pid){
    ERRX(_("Another copy of this process found, pid=%d. Exit."), pid);
}

int main(int argc, char *argv[]){
    initial_setup();
    char *self = strdup(argv[0]);
    GP = parse_args(argc, argv);
    if(GP->rest_pars_num){
        printf(_("%d extra options:\n"), GP->rest_pars_num);
        for(int i = 0; i < GP->rest_pars_num; ++i)
            printf("%s\n", GP->rest_pars[i]);
        return 1;
    }
    check4running(self, GP->pidfile);
    free(self);
    signal(SIGTERM, signals); // kill (-15) - quit
    signal(SIGHUP, SIG_IGN);  // hup - ignore
    signal(SIGINT, signals);  // ctrl+C - quit
    signal(SIGQUIT, signals); // ctrl+\ - quit
    signal(SIGTSTP, SIG_IGN); // ignore ctrl+Z
    setup_con();

    const ch55descr *descr = detect_chip();
    if(!descr){
        ERRX(_("Chip not found"));
    }
    char *ver = getver();
    if(!ver) ERRX(_("Bad chip version"));
    green(_("Found %s, version %s; flash size %d\n"), descr->devname, ver, descr->flash_size);
    if(!GP->binname) signals(0); // just check chip
    if(erasechip()) ERRX(_("Can't erase chip"));
    green(_("Try to write %s\n"), GP->binname);
    if(writeflash(GP->binname)) ERRX(_("Can't write flash"));
    green(_("Verify data\n"));
    if(verifyflash(GP->binname)) ERRX(_("Verification of flash failed"));
    if(endflash()) ERRX(_("Can't fix writing"));
    if(!GP->dontrestart){
        green(_("Reset MCU\n"));
        restart();
    }
    // clean everything
    signals(0);
    return 0;
}
