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
#include <fcntl.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <assert.h>
#include <string.h>

#include "yali.h"

/*! \brief temporary buffer for assembling LCN packets */
unsigned char _yaliBuf[2512];

/*! \brief storage for LCN-PK (serial device) file descriptor */
int _lcnSerFd = 0;

/*! \brief structure used for queueing outgoing LCN packets */
struct lcnQueue_s
{
  int len;                 /*!<\brief total length of the LCN packet */
  unsigned char *data;     /*!<\brief pointer to LCN packet data */
  struct lcnQueue_s *next; /*!<\brief pointer to next packet in queue */
};

/*! \brief pointer to root of queue for outgoing LCN packets */
struct lcnQueue_s *_lcnSendQueue = NULL;

/*! \brief pointer to last unacknowledged packet */
struct lcnQueue_s *_lcnSendAcqWait = NULL;

/*! \brief count how often a packet has been send without ack */
int _lcnSendRepCount = 0;


/*! \brief function used to queue a LCN packet into a queue
 *  \param pproot pointer to pointer to root of queue to add to
 *  \param len length of LCN packet to add to queue
 *  \param data pointer to LCN packet to add to queue
 *  \return N/A
 */
void lcnQueueAdd(struct lcnQueue_s **pproot, int len, unsigned char *data)
{
  struct lcnQueue_s *pnew;
  struct lcnQueue_s *ptmp;

  pnew = (struct lcnQueue_s*) malloc(sizeof(struct lcnQueue_s));
  if (pnew == NULL)
    {
      fprintf(stderr, "out of memory allocationg queue element\n");
      exit(1);
    }

  pnew->data = (unsigned char*) malloc(len);
  if (pnew->data == NULL)
    {
      fprintf(stderr, "out of memory allocationg data for queue\n");
      exit(1);
    }

  memcpy(pnew->data, data, len);

  pnew->len = len;
  pnew->next = NULL;

  ptmp = *pproot;
  if (ptmp==NULL)
    {
      *pproot = pnew;
    }
  else
    {
      while (ptmp->next)
	{
	  ptmp = ptmp->next;
	}
      ptmp->next = pnew;
    }
}


void lcnQueueCommandSend(int inFd, int inDest, int inCmd, int inP1, int inP2)
{
  unsigned char buf[8];

  buf[0] = 0x80;
  buf[1] = 0x04; /* 4 = without ACK,  5 = wait for ACK */
  buf[3] = 0x00;
  buf[4] = inDest;
  buf[5] = inCmd;
  buf[6] = inP1;
  buf[7] = inP2;
  buf[2] = lcnCrcCalc(buf, 8);

  lcnQueueAdd(&_lcnSendQueue, 8, buf);
}

void lcnQueueCmdAdd(struct lcnPak_s *pk, int len)
{
  lcnQueueAdd(&_lcnSendQueue, len, (unsigned char*)pk);
}

/*! \brief function used to remove and return first packet of a queue
 *  \param pproot pointer to pointer to root of queue to read from
 *  \return pointer to queue elemen that has been removed from the queue (NULL) if none
 */
struct lcnQueue_s * lcnQueueGet(struct lcnQueue_s **pproot)
{
  struct lcnQueue_s *ptmp;

  ptmp = *pproot;
  if (ptmp!=NULL)
    {
      *pproot = ptmp->next;
    }

  return ptmp;
}


/*! \brief function to open the serial device for communication with LCN-PK
 *  \return file descriptor for serial interface
 */
int open_lcnport(void)
{
  int fd;
  int status;
  struct termios options;

  if (_conf.lcnInterface==NULL || *_conf.lcnInterface==0)
    {
      printf("running in test mode without LCN\n");
      _conf.lcnInterface = NULL;
      return 0;
    }

  fd = open(_conf.lcnInterface, O_RDWR | O_NOCTTY | O_NDELAY);
  if (fd == -1)
  {
    perror("open_port: Unable to open LCN interface");
    return fd;
  }

  tcgetattr(fd, &options);

  cfsetispeed(&options, B9600);
  cfsetospeed(&options, B9600);

  options.c_cflag |= (CLOCAL | CREAD);
  options.c_cflag &= ~PARENB;
  options.c_cflag &= ~CSTOPB;
  options.c_cflag &= ~CSIZE;
  options.c_cflag |= CS8;
  options.c_iflag &= ~(IXON | IXOFF | IXANY);
  options.c_cflag &= ~CRTSCTS;

  tcsetattr(fd, TCSANOW, &options);

  ioctl(fd, TIOCMGET, &status);  
  status &= ~TIOCM_DTR;
  ioctl(fd, TIOCMSET, status);

  fcntl(fd, F_SETFL, 0);

  _lcnSerFd = fd;

  return fd;
}


/*! \brief subfunction for calculation LCN CRC bytes
 *  \param x new data byte to add
 *  \param store CRC calculated so far
 *  \return new CRC value
 */
int lcnCrcStep(int x, int store)
{ 
  int c; 
  int new = x + store; 

  c = (( new & 0x7f ) << 2 ) | (( new & 0x180 ) >> 7 ); 
  if ( c > 0xff ) { c = c-0xff; } 

  return c;
} 


/*! \brief function for calculation LCN CRC bytes
 *  \param data pointer to LCN packet
 *  \param len length of LCN packet
 *  \return calculated CRC value
 */
unsigned char lcnCrcCalc(unsigned char *data, int len)
{ 
  int crc = 0; 
  int count; 

  for (count = 0; count < len;count++)
    {       
      if ( count != 2 )
        {
          crc = lcnCrcStep(data[count], crc); 
        } 
    }

  return crc;
}  


/*! \brief function for queueing a single LCN packet for sending to serial interface
 *  \param inFd file descriptor for serial device (unused)
 *  \param p pointer to structure describing packet to send
 *  \sa lcnCommandSend
 *  \return N/A
 *
 *  Calculates the CRC of the packet and adds it to the send-queue. This function
 *  should be used each time a LCN packet has to be send to the LCN-PK.
 *  For the typical 8-Byte LCN command packet there is another function.
 */
void lcnPakSend(int inFd, struct pak_s *p)
{
  int i;

  assert(p != NULL);
  assert(p->len > 5);
  assert(p->data != NULL);

  _yaliBuf[0] = p->data[0];
  _yaliBuf[1] = p->data[1];

  for (i=2; i<p->len; i++)
    {
      _yaliBuf[i+1] = p->data[i];
    }

  _yaliBuf[2] = lcnCrcCalc(_yaliBuf, p->len + 1);

  lcnQueueAdd(&_lcnSendQueue, p->len + 1, _yaliBuf);
}


/*! \brief send the next LCN packet in the send-queue to the LCN-PK
 *  \param inFd file descriptor for serial device
 *  \return N/A
 *
 *  This function is to be called with a fixed rate, for example 10 Hz.
 */
void lcnSendNext(int inFd)
{
  struct lcnQueue_s *p;
  int i,n;
  int ret;

  if (_lcnSendAcqWait != NULL)
    {
      _lcnSendRepCount++;
      if (_lcnSendRepCount < 5) /* try it up to 5 times */
	{
	  p = _lcnSendAcqWait;
	}
      else
	{
	  /* error packet could not be delivered */
	  free(p->data);
	  p->data = NULL;
	  free(p);

	  _lcnSendAcqWait = NULL;
	}
    }

  if (_lcnSendAcqWait == NULL)
    {
      p = lcnQueueGet(&_lcnSendQueue);
      _lcnSendRepCount =  0;

      if ( (p != NULL) && (p->len >= 6) && (p->data[1] == 5) )
	{
	  _lcnSendAcqWait = p;
	}
    }

  if (p != NULL)
    {
      if (_conf.showLcnTraffic)
	{
	  printf("LCN>>> %li ", _tick);
	  for (i=0; i<p->len; i++)
	    {
	      printf(" %02X", p->data[i]);
	    }
	  printf("\n");
	}
      
      if (_conf.lcnInterface)
	{
	  n = 0;
	  while (n < p->len)
	    {
	      ret = write(inFd, &p->data[n], p->len - n);
	      if (ret < 0) break;
	      n += ret;
	    }
	}
      
      if (_lcnSendAcqWait == NULL)
	{
	  free(p->data);
	  p->data = NULL;
	  free(p);
	}
    }
}


void lcnCommandSendTimed(unsigned long tm, int inFd, int inDest,
			 int inCmd, int inP1, int inP2)
{
  struct timeQueue_s *pq;

  pq = (struct timeQueue_s*) malloc(sizeof(struct timeQueue_s));
  if (pq == NULL)
    {
      printf("out of memory 1\n");
      exit(1);
    }

  pq->time       = tm;
  pq->lcn.src    = 0x80;
  pq->lcn.info   = 0x04; /* 4 = without ACK,  5 = wait for ACK */
  pq->lcn.dstSeg = 0x00;
  pq->lcn.dst    = inDest;
  pq->lcn.cmd    = inCmd;
  pq->lcn.p1     = inP1;
  pq->lcn.p2     = inP2;
  pq->lcn.crc    = lcnCrcCalc((unsigned char*)&pq->lcn, 8);

  timeQueueAdd(pq);
}

/*! \brief send standard 8 byte LCN packet (create and add to send-queue)
 *  \param inFd file descriptor for serial device (unused)
 *  \param inDest destination LCN module
 *  \param inCmd LCN command byte
 *  \paran inP1 parameter byte 1 for command 
 *  \paran inP1 parameter byte 2 for command 
 *  \return N/A
 */
void lcnCommandSend(int inFd, int inDest, int inCmd, int inP1, int inP2)
{
  unsigned char buf[8];
  int n;
  int ret;

  buf[0] = 0x80;
  buf[1] = 0x05; /* 4 = without ACK,  5 = wait for ACK */
  buf[3] = 0x00;
  buf[4] = inDest;
  buf[5] = inCmd;
  buf[6] = inP1;
  buf[7] = inP2;
  buf[2] = lcnCrcCalc(buf, 8);

  n = 0;
  while (n<8)
    {
      ret = write(inFd, &buf[n], 8-n);
      if (ret < 0) break;
      n += ret;
    }
}


/*! \brief check if pointed data is a valid (and known) LCN packet
 *  \paran p pointer to packet data
 *  \paran inLen length of packet in bytes
 *  \return 0=crc-error, 1=unknown+crcOk, 2=incomplete, 3=valid, 4=known+crcError
 */
int lcnPakVerify(unsigned char *p, int inLen)
{
  int expLen;
  struct lcnPak_s *lcn;
  int i;
  int source;
  int srcOk, dstOk;
  struct lights_s *lp;

  lcn = (struct lcnPak_s*) p;

  if (inLen<3) return 2;

  expLen = 0;
  if (lcn->info == 0) expLen = 6;
  else if (lcn->info >=4 && lcn->info <=7) expLen = 8;
  else if (lcn->info == 8) expLen = 12;
  else if (lcn->info == 74) expLen = 12;
  else if (lcn->info == 12) expLen = 20;

  source = 0;
  for (i=1; i<256; i*=2)
    {
      source *= 2;
      if ( (i & lcn->src)!=0 )
	{
	  source += 1;
	}
    }

  srcOk = 0;
  dstOk = 0;

  lp = _lights;
  while (lp)
    {
      if (source == lp->module) srcOk = 1;
      if (lcn->dst == lp->module) dstOk = 1;
      lp = lp->next;
    }

  if (source==1) srcOk = 1;
  if (lcn->dst==1) dstOk = 1;

  if (expLen!=0 || srcOk==0 || dstOk==0)
    {
      if (inLen<expLen) return 2;
      if (lcnCrcCalc(p, expLen)==lcn->crc)
	{
	  return 3;
	}
      else
	{
	  return 4;
	}
    }

  if (lcnCrcCalc(p, inLen)==lcn->crc)
    {
      return 1;
    }
  else
    {
      return 0;
    }
}


/*! \brief check if pointed data contains a valid (and known) LCN packet
 *  \paran p pointer to received data block
 *  \paran inLen length of data block
 *  \return number of garbage bytes before start of packet
 */
int lcnPakValidScan(unsigned char *p, int inLen)
{
  int start;
  int len;
  int ret;

  for (start=1; start<inLen; start++)
    {
      len = 1;
      ret = 0;
      while ( len<=(inLen-start) )
	{
	  ret = lcnPakVerify(&p[start], len);
	  if (ret!=2) break;
	  len++;
	}

      if (ret==3) return start;

      /* valid CRC, typical length */
      if (ret==1 && (len==6 || len==8 || len==12 || len==20)) return start;
    }

  return 0;
}


/*! \brief process received LCN packet
 *  \paran p pointer to received LCN packet
 *  \paran inLen length of LCN packet
 *  \return N/A
 */
void lcnPakProc(unsigned char *p, int inLen)
{
  int i;
  struct lcnPak_s *lcn;
  int tmp;

  /* hex-dump of LCN packet */
  if (_conf.showLcnTraffic)
    {
      printf("LCN<<< ");
      for (i=0; i<inLen; i++)
	{
	  printf(" %02X", p[i]);
	}
      printf("\n");
    }

  lcn = (struct lcnPak_s*) p;

  /* print LCN packet in human readable format */
  if (_conf.showLcnTraffic)
    {
      lcnPrint(p, inLen);
    }

  /* generate binary log file for this packet */
  if (_conf.lcnBinLogBasename!=NULL)
    {
      static int pkcnt = 1;
      FILE *fp;
      char cbuf[256];

      sprintf(cbuf, "%s_%04i.lcn", _conf.lcnBinLogBasename, pkcnt++);
      fp = fopen(cbuf, "wb");
      if (fp)
	{
	  for (i=0; i<inLen; i++)
	    {
	      fputc(p[i], fp);
	    }
	  fclose(fp);
	}
    }

  yaliTimeAdapt();

  /* positive acknowledge to last command ? */

  if (inLen==8 && lcn->info==0)
    {
      int source;
      int rdst;
      struct lcnPak_s *tc;

      source = 0;
      rdst = 0;
      for (i=1; i<256; i*=2)
	{
	  source *= 2;
	  if ( (i & lcn->src)!=0 )
	    {
	      source += 1;
	    }

	  rdst *= 2;
	  if ( (i & lcn->dst)!=0 )
	    {
	      rdst += 1;
	    }
	}

      /*printf("ACK: from M%i to M%i\n", source, lcn->dst);*/

      if (_lcnSendAcqWait != NULL)
	{
	  tc = (struct lcnPak_s*) _lcnSendAcqWait->data;

	  if ( (tc->dst == source)
	       && (tc->src == rdst) )
	    {
	      /* got positive ack to last sent command */

	      free(_lcnSendAcqWait->data);
	      _lcnSendAcqWait->data = NULL;

	      free(_lcnSendAcqWait);
	      _lcnSendAcqWait = NULL;

	      _lcnSendRepCount = 0;
	    }
	}
    }

  /* module output status report */

  if (inLen==20 && lcn->info==12 
      && lcn->cmd==0x6E && lcn->p1==0x7B && lcn->p2==0x01)
    {
      int source;

      source = 0;
      for (i=1; i<256; i*=2)
	{
	  source *= 2;
	  if ( (i & lcn->src)!=0 )
	    {
	      source += 1;
	    }
	}

      stateLightUpdate(source, 1, p[8]/2);
      stateLightUpdate(source, 2, p[11]/2);
      stateLightUpdate(source, 3, p[14]/2);
    }

  /* direct module output command */

  if (inLen==8 && (lcn->info==4 || lcn->info==5))
    {
      if (lcn->cmd==4 && lcn->p1<=0xFA)
	{
	  stateLightUpdate(lcn->dst, 1, lcn->p1 * 2);
	}
      else if (lcn->cmd==5 && lcn->p1<=0xFA)
	{
	  stateLightUpdate(lcn->dst, 2, lcn->p1 * 2);
	}
      else if (lcn->cmd==3 && lcn->p1<=0xFA)
	{
	  stateLightUpdate(lcn->dst, 3, lcn->p1 * 2);
	}
      else if (lcn->cmd==1 && lcn->p1==0xFA)
	{
	  stateLightUpdate(lcn->dst, 1, 0);
	  stateLightUpdate(lcn->dst, 2, 0);
	  stateLightUpdate(lcn->dst, 3, 0);
	}      
      else if (lcn->cmd==1 && lcn->p1==0xF8)
	{
	  stateLightUpdate(lcn->dst, 1, 100);
	  stateLightUpdate(lcn->dst, 2, 100);
	  stateLightUpdate(lcn->dst, 3, 100);
	}    
      else if (lcn->cmd==1 && lcn->p1==0xCC && lcn->p2==0xCC)
	{
	  stateLightUpdate(lcn->dst, 1, 100);
	  stateLightUpdate(lcn->dst, 2, 100);
	}
      else if (lcn->cmd==1 && lcn->p1==0xFD && lcn->p2==0xFD)
	{
	  stateLightUpdate(lcn->dst, 1, 100);
	  stateLightUpdate(lcn->dst, 2, 100);
	}
      else if (lcn->cmd==1 && lcn->p1==0x00 && lcn->p2==0x00)
	{
	  stateLightUpdate(lcn->dst, 1, 0);
	  stateLightUpdate(lcn->dst, 2, 0);
	}
      else if (lcn->cmd==19)
	{
	  for (i=0; i<4; i++)
	    {
	      tmp = (((lcn->p1 >> (i * 2)) & 3) << 4) + ((lcn->p2 >> (i * 2)) & 3);
	      if (tmp==0x11)
		{
		  /* stop */
		  stateShutUpdate(lcn->dst, i+1, 0);
		}
	      else if (tmp==0x32)
		{
		  /* up */
		  stateShutUpdate(lcn->dst, i+1, 1);
		}
	      else if (tmp==0x30)
		{
		  /* down */
		  stateShutUpdate(lcn->dst, i+1, -1);
		}
	    }
	}
      else
	{
	  /* if command is unknown, trigger status request for this module */
	  yaliScheduleRefresh(lcn->dst);
	}
    }
}

/*! \brief read arbitrary number of bytes from serial interface
 *  \paran inFd file descriptor to read from 
 *  \return N/A
 *
 *  As soon as a LCN packet is completed, the funciton lcnPakProc
 *  is called further to process the received packet.
 */
void lcnSerDataGet(int inFd)
{
  static unsigned char rcbuf[128];
  static int rcpos = 0;
  static int ckpos = 0;
  static unsigned long ltime = 0;
  unsigned long ntime;
  int i;
  int ret;

  if (rcpos >= 128)
    {
      /* buffer full of garbage - flush and start over from 0 */
      rcpos = 0;
      ckpos = 0;
      /*printf("LCN receive buffer with 128 bytes flushed\n");*/
    }

  ntime = _tick;
  if ( (ntime - ltime)>1 && rcpos!=0 )
    {
      /*printf("flush old unknown data\n");*/
      lcnPakProc(rcbuf, rcpos);
      rcpos = 0;
      ckpos = 0;
    }
  ltime = ntime;

  ret = read(inFd, &rcbuf[rcpos], 128 - rcpos);
  /*printf("input from LCN (%i bytes)\n", ret);*/

  if (ret > 0)
    {
      rcpos += ret;
      
      while (ckpos < rcpos)
	{
	  ckpos++;
	  ret = lcnPakVerify(rcbuf, ckpos);

	  if (ret==2) continue; /* incomplete */

	  if (ret==3) /* known + crcOk */
	    {
	      lcnPakProc(rcbuf, ckpos);
	      for (i=ckpos; i<rcpos; i++)
		{
		  rcbuf[i-ckpos] = rcbuf[i];
		}
	      rcpos -= ckpos;
	      ckpos = 0;
	      continue;
	    }

	  ret = lcnPakValidScan(rcbuf, ckpos);
	  if (ret!=0)
	    {
	      /*printf("lcnPakValidScan returned %i\n", ret);*/
	      lcnPakProc(rcbuf, ret);
	      for (i=ret; i<rcpos; i++)
		{
		  rcbuf[i-ret] = rcbuf[i];
		}
	      ckpos -= ret;
	      rcpos -= ret;
	    }
	}
    }
}
