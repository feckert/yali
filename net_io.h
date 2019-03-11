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

#ifndef _COM_YALI_H
#define _COM_YALI_H

#include <sys/socket.h>
#include <arpa/inet.h>

/* list of yali packet types */

#define NET_EMPTY             0x00
#define NET_VERSIONGET        0x01
#define NET_LIGHTSTATUSGET    0x02
#define NET_LIGHTSTATUSSET    0x03
#define NET_TIMEGET           0x04
#define NET_SHUTTERDBGET      0x05
#define NET_LIGHTDBGET        0x06
#define NET_NETHISTGET        0x07
#define NET_SHUTSTATUSGET     0x08
#define NET_SHUTSTATUSSET     0x09
#define NET_VERSIONREPORT     0x81
#define NET_LIGHTSTATUSREPORT 0x82
#define NET_TIMEREPORT        0x84
#define NET_SHUTTERDBREPORT   0x85
#define NET_LIGHTDBREPORT     0x86
#define NET_NETHISTREPORT     0x87
#define NET_SHUTSTATUSREPORT  0x88
#define NET_RAWSEND           0x70
#define NET_RAWRECEIVED       0xF0
#define NET_ERRORREPORT       0xFF

/* list of error codes used in yali error reports */

#define NET_ERR_SERVERFULL    0x01
#define NET_ERR_ILLTYPE       0x02

/*!\brief structure used for maintaining light status (linked list element) */
struct lights_s
{
  unsigned char module;   /*!<\brief LCN module the light is connected to */
  unsigned char output;   /*!<\brief output of LCN module the light is connected to */
  char *name;             /*!<\brief associated name of the light */

  signed char state;      /*!<\brief current state of the light 0(off)..100(on) */
  unsigned long time;     /*!<\brief time the state was updated */

  struct lights_s *next;  /*!<\brief pointer to next element in linked list */
};

/*!\brief structure defining a yali packet */
struct pak_s
{
  unsigned char type;     /*!<\brief type of packet */
  unsigned short len;     /*!<\brief length of packet payload */
  unsigned char *data;    /*!<\brief pointer to packet payload */
};

/*!\brief maximum number of client that are allowed at a time */
#define CLI_NUM  4

/*!\brief structure used to store yali client information */
struct netClientDat_s
{
  unsigned char rcbuf[128]; /*!<\brief receive buffer for incoming socket data */
  int rcpos;                /*!<\brief number of bytes in receive buffer */
  int dummy;                /*!<\brief number of dummy bytes received */
  int sf;                   /*!<\brief client socket */
  struct sockaddr_in sa;    /*!<\brief client IP address information */
};

/*!\brief array of client connections */
extern struct netClientDat_s _cli[CLI_NUM];

/*!\brief file descriptor of serial interface (LCN-PK connection) */
extern int _lcnSerFd;

extern void netPakPrint(struct pak_s *p);
extern void netPakSend(int inSock, struct pak_s *p);
extern void netTimeSend(int inSock);
extern void netVersionSend(int inSock);
extern void netErrorSend(int inSock, int code, char *text);
extern void netLightStatusSend(int inSock, int module, int output, int value);
extern void netLightDbSend(int inSock);
extern void netSockProc(struct pak_s *p, int inSock);
extern void netSockTerm(int inN);
extern void netSockDataGet(int inN);
extern void netClientAccept(int srvSock);
extern int netServerOpen();
extern void netShutStatusSend(int inSock, int module, int output, int value);

#endif
