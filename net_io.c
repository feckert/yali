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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/errno.h>
#include <assert.h>
#include <sys/socket.h>
#include <fcntl.h>

#include "yali.h"

/*!\brief array used to store TCP/IP client data */
struct netClientDat_s _cli[CLI_NUM];


/*!\brief print hex dump of pointer packet to stdout
 * \param p pointer to packet structure
 * \return N/A
 */
void netPakPrint(struct pak_s *p)
{
  int i;

  printf("Type=%i Len=%i", p->type, p->len);

  for (i=0; i<p->len; i++)
    {
      printf(" %02X", p->data[i]);
    }

  printf("\n");
}


/*!\brief free memory associated to pointer packet strcture (and structure itself!)
 * \param p pointer to packet structure
 * \return N/A
 */
void netPakFree(struct pak_s *p)
{
  if (p)
    {
      if (p->data)
	{
	  free(p->data);
	  p->data = NULL;
	}
      
      free(p);
    }
}


/*!\brief send packet to socket connection
 * \param inSock socket to send to
 * \param p pointer to packet structure
 * \return N/A
 */
void netPakSend(int inSock, struct pak_s *p)
{
  int n;
  int ret;
  unsigned char buf[3];

  assert(p != NULL);
  assert((p->len == 0) || (p->data != NULL));

  if (_conf.showTcpTraffic)
    {
      printf("NET>>> ");
      netPakPrint(p);
    }

  buf[0] = p->type;
  buf[1] = p->len >> 8;
  buf[2] = p->len & 0xFF;
  
  n = 0;
  while (n<3)
    {
      ret = send(inSock, &buf[n], 3-n, 0);
      if (ret<0) break;
      n += ret;
    }

  if (ret >= 0)
    {
      n = 0;
      while (n < p->len)
	{
	  ret = send(inSock, &p->data[n], p->len - n, 0);
	  if (ret<0) break;
	  n += ret;
	}
    }
}


/*!\brief send packet containing local time to socket connection
 * \param inSock socket to send to
 * \return N/A
 */
void netTimeSend(int inSock)
{
  uint32_t tm;
  struct pak_s pak;

  tm = htonl(_yaliTime);

  pak.type = NET_TIMEREPORT;
  pak.len  = 4;
  pak.data = (unsigned char*) &tm;

  netPakSend(inSock, &pak);
}


/*!\brief send packet containing local version to socket connection
 * \param inSock socket to send to
 * \return N/A
 */
void netVersionSend(int inSock)
{
  int i;
  struct pak_s pak;

  _yaliBuf[0] = _yaliVersionMayor;
  _yaliBuf[1] = _yaliVersionMinor;
  _yaliBuf[2] = 1; /* LCN */

  i = 0;
  while ( (i < (512-4)) && (_yaliVersionText[i]!=0) )
    {
      _yaliBuf[3+i] = _yaliVersionText[i];
      i++;
    }

  _yaliBuf[3+i] = 0;

  pak.type = NET_VERSIONREPORT;
  pak.len  = 4+i;
  pak.data = _yaliBuf;

  netPakSend(inSock, &pak);
}


/*!\brief send packet containing error report to socket connection
 * \param inSock socket to send to
 * \param code error code to send
 * \param text error string to send
 * \return N/A
 */
void netErrorSend(int inSock, int code, char *text)
{
  int i;
  struct pak_s pak;

  _yaliBuf[0] = code;

  i = 0;
  while ( (i < (512-2)) && (text[i]!=0) )
    {
      _yaliBuf[i+1] = text[i];
      i++;
    }

  _yaliBuf[i+1] = 0;

  pak.type = NET_ERRORREPORT;
  pak.len  = i+2;
  pak.data = _yaliBuf;

  netPakSend(inSock, &pak);
}


/*!\brief send packet containing light status report to socket connection
 * \param inSock socket to send to
 * \param module ID of LCN module the light is connected to
 * \param output output of LCN module the light is connected to
 * \param value state of the light 0(off) .. 100(on)
 * \return N/A
 */
void netLightStatusSend(int inSock, int module, int output, int value)
{
  struct pak_s pak;
  unsigned char buf[3];

  buf[0] = module;
  buf[1] = output;
  buf[2] = value;

  pak.type = NET_LIGHTSTATUSREPORT;
  pak.len  = 3;
  pak.data = buf;

  netPakSend(inSock, &pak);
}


/*!\brief send packet containing shutter status report to socket connection
 * \param inSock socket to send to
 * \param module ID of LCN module the shutter is connected to
 * \param output output of LCN module the shutter is connected to
 * \param value state of the shutter 0(closed) .. 100(open)
 * \return N/A
 */
void netShutStatusSend(int inSock, int module, int output, int value)
{
  struct pak_s pak;
  unsigned char buf[3];

  buf[0] = module;
  buf[1] = output;
  buf[2] = value;

  pak.type = NET_SHUTSTATUSREPORT;
  pak.len  = 3;
  pak.data = buf;

  netPakSend(inSock, &pak);
}


/*!\brief send packet containing light data base to socket connection
 * \param inSock socket to send to
 * \return N/A
 */
void netLightDbSend(int inSock)
{
  int len;
  int y;
  struct pak_s pak;
  struct lights_s *lp;

  len = 0;

  lp = _lights;
  while (lp)
    {
      _yaliBuf[len] = lp->module;
      _yaliBuf[len+1] = lp->output;
      _yaliBuf[len+2] = lp->state;

      y = 0;
      while (lp->name[y] != 0)
	{
	  _yaliBuf[len+3+y] = lp->name[y];
	  y++;
	}
      _yaliBuf[len+3+y] = 0;
      len += 4+y;

      lp = lp->next;
    }

  pak.type = NET_LIGHTDBREPORT;
  pak.len = len;
  pak.data = _yaliBuf;

  netPakSend(inSock, &pak);
}


/*!\brief process received packet from TCP/IP socket
 * \param p pointer to packet structure
 * \param inSock socket this packet was received from
 * \return N/A
 */
void netSockProc(struct pak_s *p, int inSock)
{
  int tmp;
  struct lights_s *lp;
  struct pak_s *ptmp;

  if (_conf.showTcpTraffic)
    {
      printf("NET<<< ");
      netPakPrint(p);
    }

  switch (p->type)
    {
    case NET_EMPTY:
      /* nothing to do */
      break;

    case NET_VERSIONGET:
      netVersionSend(inSock);
      break;

    case NET_TIMEGET:
      yaliTimeAdapt();
      netTimeSend(inSock);
      break;

    case NET_LIGHTSTATUSSET:
      if (_conf.lcnInterface)
	{
	  if (p->data[1]==1)
	    {
	      tmp = p->data[2];
	      if (tmp>100) tmp = 100;
	      tmp = tmp/2;
	      
	      lcnQueueCommandSend(_lcnSerFd, p->data[0], 4, tmp, 4);
	    }
	  else if (p->data[1]==2)
	    {
	      tmp = p->data[2];
	      if (tmp>100) tmp = 100;
	      tmp = tmp/2;
	      
	      lcnQueueCommandSend(_lcnSerFd, p->data[0], 5, tmp, 4);
	    }
	  else if (p->data[1]==3)
	    {
	      tmp = p->data[2];
	      if (tmp>100) tmp = 100;
	      tmp = tmp/2;
	      
	      lcnQueueCommandSend(_lcnSerFd, p->data[0], 3, tmp, 4);
	    }
	}
      else
	{
	  /* operation with out LCN (test mode) */
	  stateLightUpdate(p->data[0], p->data[1], p->data[2]);
	}
      break;

    case NET_LIGHTSTATUSGET:
      lp = _lights;
      while (lp)
	{
	  if (p->data[0] == lp->module
	      && p->data[1] == lp->output)
	    {
	      netLightStatusSend(inSock, lp->module, lp->output, lp->state);
	      break;
	    }
	  lp = lp->next;
	}
      break;

    case NET_SHUTSTATUSSET:
      if (_conf.lcnInterface)
	{
	  struct shutter_s *sp;
	  float inMin, inMax;
	  
	  inMin = 0.01 * p->data[2];
	  inMax = 0.01 * p->data[3];
	  
	  sp = stateShutPtrGet(p->data[0], p->data[1]);
#ifdef DBG
	  printf("shut %i is at %1.2f .. %1.2f cmd to %1.2f .. %1.2f\n",
		 p->data[0], sp->posMin, sp->posMax, inMin, inMax);
#endif
	  if (sp != NULL)
	    {
	      stateShutCommand(p->data[0], p->data[1], p->data[2], p->data[3]);
	    }
	}
      else
	{
	  /* operation with out LCN (test mode) */
#ifdef DBG
	  printf("set roll %i to %i\n", p->data[0], p->data[1]);
#endif
	}
      break;

    case NET_SHUTSTATUSGET:
      tmp = stateShutGet(lp->module, lp->output);
      if (tmp >= 0)
	{
	  netShutStatusSend(inSock, lp->module, lp->output, tmp);
	}
      break;

    case NET_SHUTTERDBGET:
      stateShutDbSend(inSock);
      break;

    case NET_LIGHTDBGET:
      netLightDbSend(inSock);
      break;

    case NET_NETHISTGET:
      ptmp = stateLightHistGet();
      netPakSend(inSock, ptmp);
      netPakFree(ptmp);
      ptmp = NULL;
      break;

    default:
      /* unknown type */
      netErrorSend(inSock, NET_ERR_ILLTYPE, "received illegal code");
      break;
    }
}


/*!\brief close TCP/IP connection and remove from list of open connections
 * \param inN index to client table
 * \return N/A
 */
void netSockTerm(int inN)
{
  assert(inN >= 0);
  assert(inN < CLI_NUM);
  assert(_cli[inN].sf != -1);

  if (_conf.showTcpTraffic)
    {
      printf("terminate client %i\n", inN);
    }

  close(_cli[inN].sf);

  _cli[inN].sf = -1;
  _cli[inN].rcpos = 0;
  _cli[inN].dummy = 0;
}


/*!\brief read arbitrary number of bytes from socket connection
 * \param inN index to client table
 * \return N/A
 *
 * As soon as a packet is completed the function netSockProc is called
 * with the received packet.
 */
void netSockDataGet(int inN)
{
  unsigned char *rcbuf;
  int rcpos;
  int ret;
  int i;
  int len;
  struct pak_s pak;
  unsigned char dummybuf[128];
  int dummy;
  int inSock;
  
  assert(inN >= 0);
  assert(inN < CLI_NUM);
  assert(_cli[inN].sf != -1);

  inSock = _cli[inN].sf;
  rcbuf = _cli[inN].rcbuf;
  rcpos = _cli[inN].rcpos;
  dummy = _cli[inN].dummy;

  /*printf("%i: rcpos=%i\n", inN, rcpos);*/

  if (rcpos >= 128)
    {
      len = (pak.len + 3) - rcpos - dummy;
      assert(len>0);
      if (len>128) len = 128;
      ret = recv(inSock, dummybuf, len, 0);
      if (ret > 0)
	{
	  /*printf("received %i dummy-bytes from client %i\n", ret, inN);*/
	  dummy += ret;
	  len = (pak.len + 3) - rcpos - dummy;
	  if (len==0)
	    {
	      /*printf("<<< consumed illegal packet (len=%i)\n", pak.len);*/
	      rcpos = 0;
	      dummy = 0;
	    }
	}
      else
	{
	  netSockTerm(inN);
	  rcpos = 0;
	  dummy = 0;
	}
    }

  ret = recv(inSock, &rcbuf[rcpos], 128 - rcpos, 0);
  if (ret > 0)
    {
      /*printf("received %i bytes from client %i\n", ret, inN);*/
      rcpos += ret;
      
      while (rcpos >= 3)
	{
	  pak.type  = rcbuf[0];
	  pak.len   = (rcbuf[1]<<8) + rcbuf[2];
	  pak.data  = &rcbuf[3];

	  len = pak.len + 3;
	  if (rcpos >= len)
	    {
	      netSockProc(&pak, inSock);
	      for (i=len; i<rcpos; i++)
		{
		  rcbuf[i-len] = rcbuf[i];
		}
	      rcpos -= len;
	    }
	  else
	    {
	      break;
	    }
	}
    }
  else
    {
      netSockTerm(inN);
      rcpos = 0;
      dummy = 0;
    }

  _cli[inN].rcpos = rcpos;
  _cli[inN].dummy = dummy;

  /*printf("%i after rcpos=%i\n", inN, rcpos);*/
}


/*!\brief accept a possbly pending client connection
 * \param srcSock socket where connections are to be accepted
 * \return N/A
 *
 * When a new client has been accepted, the client is added to the
 * list of open connections.
 */
void netClientAccept(int srvSock)
{
  int sock;
  socklen_t sLen;
  struct sockaddr_in tmpClient;
  int idx;
  int i;

  sLen = sizeof(tmpClient);

  idx = -1;
  for (i=0; i<CLI_NUM; i++)
    {
      if (_cli[i].sf == -1)
	{
	  idx = i;
	  break;
	}
    }

  /*printf("idx=%i\n", idx);*/

  if (idx == -1)
    {
      sock = accept(srvSock, (struct sockaddr *) &tmpClient, &sLen);
      if (sock == -1)
	{
	  if (errno==EWOULDBLOCK) return;

	  perror("accept srvSock failed");
	  exit(1);
	}
      netErrorSend(sock, NET_ERR_SERVERFULL, "too many clients");
      close(sock);

      if (_conf.showTcpTraffic)
	{
	  printf("Client refused\n");
	}
    }
  else
    {
      sock = accept(srvSock, (struct sockaddr *) &_cli[idx].sa, &sLen);
      if (sock == -1)
	{
	  if (errno==EWOULDBLOCK) return;

	  perror("accept srvSock failed");
	  exit(1);
	}

      _cli[idx].sf = sock;

      if (_conf.showTcpTraffic)
	{
	  printf("Client %i accepted\n", idx);
	}
    }
}


/*!\brief open server socket where client can connect to
 * \return socket
 */
int netServerOpen()
{
  int i;
  int srvSock;
  socklen_t sLen;
  struct sockaddr_in srv;
  int tmp;

  for (i=0; i<CLI_NUM; i++)
    {
      _cli[i].rcpos = 0;
      _cli[i].dummy = 0;
      _cli[i].sf = -1;
    }

  stateBufInit();

  srvSock = socket(AF_INET, SOCK_STREAM, 0);
  if (srvSock == -1)
    {
      perror("socket srvSock failed");
      exit(1);
    }

  srv.sin_addr.s_addr = INADDR_ANY;
  srv.sin_port = htons( _conf.tcpPort );
  srv.sin_family = AF_INET;
  sLen = sizeof(srv);

  tmp = bind(srvSock, (struct sockaddr *) &srv, sLen);
  if (tmp == -1)
    {
      perror("bind srvSock failed");
      exit(1);
    }

  tmp = listen(srvSock, 2);
  if (tmp == -1)
    {
      perror("listen srvSock failed");
      exit(1);
    }

  tmp = fcntl(srvSock, F_SETFL, O_NONBLOCK);
  if (tmp == -1)
    {
      perror("fcntl srvSock NONBLOCK failed");
      exit(1);
    }

  return srvSock;
}
