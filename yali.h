/*
  YALI - Yet Another LCN Interface

Copyright (C) 2009 Daniel Dallmann

This program is free software; you can redistribute it and/or modify it under 
the terms of the GNU General Public License as published by the 
Free Software Foundation; either version 3 of the License, 
or (at your option) any later version.

This program is distributed in the hope that it will be useful, but 
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
or FITNESS FOR A PARTICULAR PURPOSE. 
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along 
with this program; if not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _YALI_H
#define _YALI_H

#if OSX_V == 8
/* MAC OSX Version 10.4 ? */
typedef unsigned int uint32_t;
typedef int socklen_t;
#include "netinet/in.h"
#endif

#include "conf.h"
#include "net_io.h"
#include "lcn_io.h"
#include "state.h"
#include "time_queue.h"
#include "netinet/in.h"

extern unsigned long _yaliTime;
extern unsigned short _yaliVersionMayor;
extern unsigned short _yaliVersionMinor;
extern char *_yaliVersionText;
extern unsigned char _yaliBuf[2512];
extern struct lights_s *_lights;
extern unsigned long volatile _tick;

extern void yaliTimeAdapt(void);
extern void yaliScheduleRefresh(int inModule);
extern int confLoad(char *filename);

#endif
