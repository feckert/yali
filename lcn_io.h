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

#ifndef _COM_LCN_H
#define _COM_LCN_H

/*! \brief structure used to access fields of a standard LCN 8-byte packet */
struct lcnPak_s
{
  unsigned char src;    /*!<\brief ID of source module (bits in reversed order!) */
  unsigned char info;   /*!<\brief Info field */
  unsigned char crc;    /*!<\brief CRC field */
  unsigned char dstSeg; /*!<\brief destination segment (always 0 for small installations) */
  unsigned char dst;    /*!<\brief ID of destination module (or group) */
  unsigned char cmd;    /*!<\brief command byte */
  unsigned char p1;     /*!<\brief first command paramter */
  unsigned char p2;     /*!<\brief second command paramter */
};

extern int open_lcnport(void);
extern float decodeRamp(int n);
extern void decodeTime(int n);
extern void decode(unsigned char *p, int len);
extern int lcnCrcStep(int x, int store);
extern unsigned char lcnCrcCalc(unsigned char *list, int len);
extern void lcnPakSend(int inFd, struct pak_s *p);
extern void lcnCommandSend(int inFd, int inDest, int inCmd, int inP1, int inP2);
extern void lcnQueueCommandSend(int inFd, int inDest, int inCmd, int inP1, int inP2);
extern void lcnCommandSendTimed(unsigned long tm, int inFd, int inDest, int inCmd, int inP1, int inP2);
extern int lcnPakVerify(unsigned char *p, int inLen);
extern int lcnPakValidScan(unsigned char *p, int inLen);
extern void lcnPakProc(unsigned char *p, int inLen);
extern void lcnSerDataGet(int inFd);
extern void lcnPrint(unsigned char *p, int len);
extern void lcnSendNext(int inFd);

extern void lcnQueueCmdAdd(struct lcnPak_s *pk, int len);

#endif
