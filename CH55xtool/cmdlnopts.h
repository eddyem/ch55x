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
#ifndef CMDLNOPTS_H__
#define CMDLNOPTS_H__

/*
 * here are some typedef's for global data
 */
typedef struct{
    char *pidfile;          // name of PID file
    char *binname;          // name of binary file
    int dontrestart;        // don't restart after writing
    int rest_pars_num;      // number of rest parameters
    char** rest_pars;       // the rest parameters: array of char*
} glob_pars;


glob_pars *parse_args(int argc, char **argv);

#endif // CMDLNOPTS_H__
