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

#ifndef _STATE_H
#define _STATE_H

extern void stateLightUpdate(int module, int output, int value);
extern struct pak_s *stateLightHistGet(void);
extern void stateBufInit(void);
extern void stateLightLog(int module, int output, int value);
extern void stateShutUpdate(int inModule, int inShutNum, int inDirection);
extern void stateShutCheck(void);

/*!\brief size of the history buffer */
#define STATE_BUF_SIZE (8*512)

/*!\brief pointer to state history buffer */
extern unsigned char *_stateHistBuf;

struct shutter_s
{
  char *name;
  int module;
  int rnum;
  float upTimeTotal;
  float downTimeTotal;
  float posMin;
  float posMax;
  double time;
  int move;
  struct shutter_s *next;
};

extern struct shutter_s *_stateShutRoot;

extern void stateShutUpdate(int inModule, int inShutNum, int inDirection);
extern int stateShutGet(int inModule, int inShut);
extern void stateShutCreate(int inModule, int inShut, char *inName, double inTUp, double inTDown);
extern struct shutter_s *stateShutPtrGet(int inModule, int inShut);
extern void stateShutDbSend(int inSock);
extern void stateShutAdapt(struct shutter_s *sp, float inPos);
extern void stateShutCommand(int inModule, int inShutNum, int inMin, int inMax);

#endif /* _STATE_H */
