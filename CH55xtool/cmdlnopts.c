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

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <math.h>
#include "cmdlnopts.h"
#include "usefull_macros.h"

/*
 * here are global parameters initialisation
 */
static int help;
static glob_pars  G;

// default PID filename:
#define DEFAULT_PIDFILE "/tmp/testcmdlnopts.pid"

//            DEFAULTS
// default global parameters
static glob_pars const Gdefault = {
    .pidfile = DEFAULT_PIDFILE,
};

/*
 * Define command line options by filling structure:
 *  name        has_arg     flag    val     type        argptr              help
*/
static myoption cmdlnopts[] = {
// common options
    {"help",    NO_ARGS,    NULL,   'h',    arg_int,    APTR(&help),        _("show this help")},
    {"pidfile", NEED_ARG,   NULL,   'P',    arg_string, APTR(&G.pidfile),   _("pidfile (default: " DEFAULT_PIDFILE ")")},
    {"binname", NEED_ARG,   NULL,   'b',    arg_string, APTR(&G.binname),   _("name of binary file to flash")},
    {"dontrestart",NO_ARGS, NULL,   'd',    arg_int,    APTR(&G.dontrestart),_("don't reset MCU after writing")},
   end_option
};

/**
 * Parse command line options and return dynamically allocated structure
 *      to global parameters
 * @param argc - copy of argc from main
 * @param argv - copy of argv from main
 * @return allocated structure with global parameters
 */
glob_pars *parse_args(int argc, char **argv){
    int i;
    void *ptr;
    ptr = memcpy(&G, &Gdefault, sizeof(G)); assert(ptr);
    size_t hlen = 1024;
    char helpstring[1024], *hptr = helpstring;
    snprintf(hptr, hlen, "Usage: %%s [args]\n\n\tWhere args are:\n");
    // format of help: "Usage: progname [args]\n"
    change_helpstring(helpstring);
    // parse arguments
    parseargs(&argc, &argv, cmdlnopts);
    if(help) showhelp(-1, cmdlnopts);
    if(argc > 0){
        G.rest_pars_num = argc;
        G.rest_pars = MALLOC(char *, argc);
        for (i = 0; i < argc; i++)
            G.rest_pars[i] = strdup(argv[i]);
    }
    return &G;
}

