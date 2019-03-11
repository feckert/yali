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
#include <assert.h>
#include <string.h>
#include <math.h>

#include "yali.h"

/*!\brief pointer to history buffer */
unsigned char *_stateHistBuf = NULL;

/*!\brief index to history buffer (index of next written byte) */
int _stateHistBufPtr = 0;


/*!\brief initialize history buffer
 * \return N/A
 */
void stateBufInit(void)
{
  int i;

  _stateHistBuf = (unsigned char*) malloc(STATE_BUF_SIZE);
  if (_stateHistBuf==NULL)
    {
      fprintf(stderr, "out of memory");
      exit(1);
    }
  
  _stateHistBufPtr = 0;
  for (i=0; i<STATE_BUF_SIZE; i++)
    {
      _stateHistBuf[i] = 0;
    }
}


/*!\brief log stats change of a light into history buffer
 * \param module ID of LCN module the light is connected to
 * \param output output of LCN module the light is connected to
 * \param value new state of the light (0=off ... 100=on)
 * \return N/A
 */
void stateLightLog(int module, int output, int value)
{
  unsigned char *p;
  unsigned long tm;

  yaliTimeAdapt();
  tm = _yaliTime;

  p = &_stateHistBuf[_stateHistBufPtr];

  p[0] = (tm >> 24) & 0xFF;
  p[1] = (tm >> 16) & 0xFF;
  p[2] = (tm >> 8) & 0xFF;
  p[3] = tm & 0xFF;
  p[4] = 1;
  p[5] = module;
  p[6] = output;
  p[7] = value;

  _stateHistBufPtr = (_stateHistBufPtr + 8) % STATE_BUF_SIZE;
}


/*!\brief log stats change of a shutter into history buffer
 * \param module ID of LCN module the light is connected to
 * \param shutter shutter of LCN module the light is connected to
 * \param value new state of the light (0=off ... 100=on)
 * \return N/A
 */
void stateShutLog(int module, int shutter, int value)
{
  unsigned char *p;
  unsigned long tm;

  yaliTimeAdapt();
  tm = _yaliTime;

  p = &_stateHistBuf[_stateHistBufPtr];

  p[0] = (tm >> 24) & 0xFF;
  p[1] = (tm >> 16) & 0xFF;
  p[2] = (tm >> 8) & 0xFF;
  p[3] = tm & 0xFF;
  p[4] = 2;
  p[5] = module;
  p[6] = shutter;
  p[7] = value;

  _stateHistBufPtr = (_stateHistBufPtr + 8) % STATE_BUF_SIZE;
}


/*!\brief create yali packet containing the history data
 * \return pointer to packet definition structure
 */
struct pak_s *stateLightHistGet(void)
{
  struct pak_s *p;
  int i,j;
  int idx;

  p = (struct pak_s*) malloc(sizeof(struct pak_s));
  if (p == NULL)
    {
      fprintf(stderr, "out of memory");
      return NULL;
    }

  idx = _stateHistBufPtr;
  for (i=0; i<STATE_BUF_SIZE; i+=8)
    {
      for (j=0; j<8; j++)
	{
	  if (_stateHistBuf[idx+j] != 0) break;
	}
      if (j!=8) break;
      idx = (idx + 8) % STATE_BUF_SIZE;
    }

  p->len = STATE_BUF_SIZE - i;
  p->type = NET_NETHISTREPORT;

  p->data = (unsigned char*) malloc(p->len);
  if (p->data == NULL)
    {
      fprintf(stderr, "out of memroy");
      exit(1);
    }

  for (i=0; i<p->len; i++)
    {
      p->data[i] = _stateHistBuf[idx];
      idx++;
      if (idx >= STATE_BUF_SIZE)
	{
	  idx = 0;
	}
    }
  
  return p;
}


/*!\brief update light status data
 * \param module ID of LCN module the light is connected to
 * \param output output of LCN module the light is connected to
 * \param value new state of the light (0=off ... 100=on)
 * \return N/A
 *
 *  check if there is a light associated with the given
 *  module ID and output number, then check if the
 *  state of the light has changed. If so, then log the
 *  change in the history buffer and inform all connected
 *  clients.
 */
void stateLightUpdate(int module, int output, int value)
{
  struct lights_s *lp;
  int i;

  lp = _lights;
  while (lp != 0)
    {
      if (lp->module == module)
	{
	  if (lp->output==output)
	    {
	      if (_conf.showLcnTraffic)
		{
		  printf("LCN: \"%s\" auf %i%%%s\n", lp->name, value,
			 (lp->state!=value)?" (changed)":"" );
		}

	      lp->time  = _yaliTime;

	      if (lp->state != value)
		{
		  stateLightLog(module, output, value);
		  lp->state = value;

		  for (i=0; i<CLI_NUM; i++)
		    {
		      if (_cli[i].sf != -1)
			{
			  netLightStatusSend(_cli[i].sf, module, output, value);
			}
		    }
		}
	    }
	}
      lp = lp->next;
    }
}

struct shutter_s *_stateShutRoot = NULL;

void stateShutUpdate2(struct shutter_s *p, int inDirection)
{
  double time;
  double diff;
  double diffMin;
  double diffMax;
  double posMin;
  double posMax;
  int i;

  time = 0.1 * _tick; /*getCurrentTime();*/

  diff = time - p->time;
  while (diff < 0) diff += 429496729.6;

  /* +/- 0.064s for 90% confidence after 3 (or more) measurements */

  diffMin = diff - 0.064;
  if (diffMin < 0.0) diffMin = 0.0;
  diffMax = diff + 0.064;
  posMin = p->posMin;
  posMax = p->posMax;

  if (p->move > 0)
    {
      posMin += diffMin / p->upTimeTotal;
      posMax += diffMax / p->upTimeTotal;
      if (posMin > 1.0) posMin = 1.0;
      if (posMax > 1.0) posMax = 1.0;
    }
  else if (p->move < 0)
    {
      posMin -= diffMax / p->downTimeTotal;
      posMax -= diffMin / p->downTimeTotal;
      if (posMin < 0.0) posMin = 0.0;
      if (posMax < 0.0) posMax = 0.0;
    }

  p->time = time;
  p->posMin = posMin;
  p->posMax = posMax;

  stateShutLog(p->module, p->rnum, floor(0.5 + 50.0*(p->posMin + p->posMax)) );

#ifdef DBG
  printf("M%02i/%i is now at %1.1f%% .. %1.1f%%", p->module, p->rnum,
	 100.0 * p->posMin, 100.0 * p->posMax);
#endif

  if (p->move != 0)
    {
      for (i=0; i<CLI_NUM; i++)
	{
	  if (_cli[i].sf != -1)
	    {
	      netShutStatusSend(_cli[i].sf, p->module, p->rnum,
				   floor(0.5 + 50.0*(p->posMin + p->posMax)) );
	    }
	}

#ifdef DBG
      printf(" (%1.2fs/%1.2fs %s)", 0.5*(diffMin + diffMax),
	     (p->move > 0) ? p->upTimeTotal:p->downTimeTotal,
	     (p->move > 0) ? "up":"down");
#endif
    }

#ifdef DBG
  if (inDirection < 0) printf(" (now going down)");
  if (inDirection > 0) printf(" (now going up)");
#endif

  p->move = inDirection;

#ifdef DBG
  printf("\n");
#endif
}

void stateShutUpdate(int inModule, int inShutNum, int inDirection)
{
  struct shutter_s *p;

  p = _stateShutRoot;
  while (p != NULL)
    {
      if ( (p->module == inModule) && (p->rnum == inShutNum) )
	{
	  stateShutUpdate2(p, inDirection);
	  return;
	} 
      p = p->next;
    }

  /* unkown - ignore */
#ifdef DBG
  printf("Command to unknown shutter (M%02i/%i)\n", inModule, inShutNum);
#endif
}

void stateShutCreate(int inModule, int inShut, char *inName, double inTUp, double inTDown)
{
  struct shutter_s *p;

  /*printf("M%02i/%i \"%s\" up=%1.1fs down=%1.1fs\n", inModule, inShut, inName, inTUp, inTDown);*/

  p = (struct shutter_s*) malloc(sizeof(struct shutter_s));
  if (p == NULL)
    {
      printf("out of memory\n");
      exit(1);
    }

  p->module = inModule;
  p->rnum = inShut;
  p->name = strdup(inName);
  p->upTimeTotal   = inTUp;
  p->downTimeTotal = inTDown;
  p->posMin = 0.0; /* unknown */
  p->posMax = 1.0; /* unknown */
  p->move = 0; /* doesn't move */
  p->time = 0.0;
  p->next = _stateShutRoot;

  _stateShutRoot = p;
}

/*!\brief send packet containing shutter data base to socket connection
 * \param inSock socket to send to
 * \return N/A
 */
void stateShutDbSend(int inSock)
{
  int len;
  int y;
  struct pak_s pak;
  struct shutter_s *sp;

  len = 0;

  sp = _stateShutRoot;
  while (sp)
    {
      _yaliBuf[len] = sp->module;
      _yaliBuf[len+1] = sp->rnum;
      _yaliBuf[len+2] = floor(0.5 + (100.0 * sp->posMin));
      _yaliBuf[len+3] = floor(0.5 + (100.0 * sp->posMax));

      y = 0;
      while (sp->name[y] != 0)
	{
	  _yaliBuf[len+4+y] = sp->name[y];
	  y++;
	}
      _yaliBuf[len+4+y] = 0;
      len += 5+y;

      sp = sp->next;
    }

  pak.type = NET_SHUTTERDBREPORT;
  pak.len = len;
  pak.data = _yaliBuf;

  netPakSend(inSock, &pak);
}

void stateShutCheck(void)
{
  struct shutter_s *p;
  double time;
  double diff;
  double diffMin;
  double diffMax;
  double posMin;
  double posMax;
  int flag;
  int i;

  time = 0.1 * _tick; /*getCurrentTime();*/

  p = _stateShutRoot;
  while (p != NULL)
    {
      diff = time - p->time;
      while (diff < 0) diff += 429496729.6;
      
      /* +/- 0.064s for 90% confidence after 3 (or more) measurements */
      
      diffMin = diff - 0.064;
      if (diffMin < 0.0) diffMin = 0.0;
      diffMax = diff + 0.064;
      posMin = p->posMin;
      posMax = p->posMax;
      
      flag = 0;
      if (p->move > 0 && posMin < 1.0)
	{
	  posMin += diffMin / p->upTimeTotal;
	  if (posMin > 1.0)
	    {
	      posMin = 1.0;
	      posMax = 1.0;
	      flag = 1;
	    }
	}
      else if (p->move < 0 && posMax > 0.0)
	{
	  posMax -= diffMin / p->downTimeTotal;
	  if (posMax < 0.0)
	    {
	      posMin = 0.0;
	      posMax = 0.0;
	      flag = 1;
	    }
	}
      
      if (flag != 0)
	{
	  p->move = 0;
	  p->posMin = posMin;
	  p->posMax = posMax;

	  stateShutLog(p->module, p->rnum, floor(0.5 + 50.0*(p->posMin + p->posMax)) );

	  for (i=0; i<CLI_NUM; i++)
	    {
	      if (_cli[i].sf != -1)
		{
		  netShutStatusSend(_cli[i].sf, p->module, p->rnum,
				       floor(0.5 + 50.0*(p->posMin + p->posMax)) );
		}
	    }

#ifdef DBG
	  printf("M%02i/%i is now at %1.1f%% (endstop)\n", p->module, p->rnum,
		 100.0 * p->posMin);
#endif
	}

      p = p->next;
    }
}

struct shutter_s *stateShutPtrGet(int inModule, int inShut)
{
  struct shutter_s *p;
  
  p = _stateShutRoot;
  while (p != NULL)
    {
      if ( (p->module == inModule) && (p->rnum == inShut) )
	{
	  return p;
	}
      
      p = p->next;
    }
  
  return NULL;
}

int stateShutGet(int inModule, int inShut)
{
  struct shutter_s *p;
  
  p = stateShutPtrGet(inModule, inShut);
  if (p != NULL)
    {
      /* return mean of min and max guess */
      return floor(0.5 + 50.0*(p->posMin + p->posMax));
    }
  
  return -1;
}

void stateShutAdapt(struct shutter_s *sp, float inPos)
{
  float pos;
  float diff;
  float tm;
  int i;
  int cmd;

  if (inPos == 0.0)
    {
      pos = sp->posMax + 0.5;
    }
  else if (inPos == 1.0)
    {
      pos = sp->posMin - 0.5;
    }
  else
    {
      pos = 0.5 * (sp->posMin + sp->posMax);
    }

  diff = inPos - pos;

  if (diff > 0.0)
    {
      tm = diff * sp->upTimeTotal;
      sp->posMin += (tm - 0.1) / sp->upTimeTotal;
      sp->posMax += (tm + 0.1) / sp->upTimeTotal;
      cmd = 0x32;
    }
  else
    {
      tm = -diff * sp->downTimeTotal;
      sp->posMax += (tm - 0.1) / sp->upTimeTotal;
      sp->posMin += (tm + 0.1) / sp->upTimeTotal;
      cmd = 0x30;
    }

  if (sp->posMin < 0.0) sp->posMin = 0.0;
  if (sp->posMax < 0.0) sp->posMax = 0.0;
  if (sp->posMin > 1.0) sp->posMin = 1.0;
  if (sp->posMax > 1.0) sp->posMax = 1.0;

  for (i=0; i<CLI_NUM; i++)
    {
      if (_cli[i].sf != -1)
	{
	  netShutStatusSend(_cli[i].sf, sp->module, sp->rnum,
			       floor(0.5 + 50.0*(sp->posMin + sp->posMax)) );
	}
    }

#ifdef DBG
  printf("shutter move time %1.2fs\n", tm);
#endif

  /* generate two LCN commands:
     1. start movement of shutter in the correct direction
     2. stop movement after the calculated time
  */

  i = 2*(sp->rnum - 1);

  lcnCommandSendTimed(_tick + 1, 
		      _lcnSerFd, sp->module, 0x13, ((cmd>>4)&3)<<i, (cmd&3)<<i );

  lcnCommandSendTimed(_tick + 1 + floor(10.0 * tm + 0.5), 
		      _lcnSerFd, sp->module, 0x13, 1<<i, 1<<i );

#ifdef DBG
  timeQueuePrint();
#endif
}


void stateShutCommand(int inModule, int inShutNum, int inMin, int inMax)
{
  struct shutter_s *p;
  int pos;

  assert(inMin <= inMax);

  p = _stateShutRoot;
  while (p != NULL)
    {
      if ( (p->module == inModule) && (p->rnum == inShutNum) )
	{
	  pos = -1;
	  if (inMax == 0 || (inMin == 0 && inMax > p->posMax)) pos = 0;
	  else if (inMin == 100 || (inMax == 100 && inMin < p->posMin)) pos = 100;
	  else if (inMax < p->posMin || inMin > p->posMax) pos = (inMin + inMax)/2;
	  else if (inMin < p->posMin && inMax > p->posMax) return; /* pos is ok */
	  else pos = (inMin + inMax)/2;

	  stateShutAdapt(p, 0.01 * pos);
	  return;
	} 
      p = p->next;
    }

  /* unkown - ignore */
#ifdef DBG
  printf("Command to unknown shutter (M%02i/%i)\n", inModule, inShutNum);
#endif
}


/****************************************************************************/
/******* Sun calculations */

struct sun_s {
  double day,jd;
  double L;
  double g;
  double A;
  double e;
  double a;
  double d;
  double t0,og,tau,AA,h,hr;
} _sun;

#define RAD(X) ((X)*(3.1415926535 / 180.0))
#define DEG(X) ((X)*(180.0 / 3.1415926535))

void stateSunCalc(Void)
{
  time_t t;
  double sn = 49.0 + 9.0/60.0;
  double wo = 9.0 + 17.0/60.0;
  
  /* 946684800 = Sa  1 Jan 2000 00:00:00 UTC */

  t = time(NULL);
  _sun.day = -0.5 + (t - 946684800.0) / (24.0 * 3600.0);
  _sun.L = 280.460 + 0.9856474 * _sun.day;
  _sun.L = fmod(_sun.L, 360.0);
  _sun.g = 357.528 + 0.9856003 * _sun.day;
  _sun.g = fmod(_sun.g, 360.0);
  _sun.A = _sun.L + 1.915 * sin(RAD(_sun.g)) + 0.020 * sin(2.0*RAD(_sun.g));
  _sun.e = 23.439 - 4e-7 * _sun.day;
  _sun.a = DEG(atan2(cos(RAD(_sun.e))*sin(RAD(_sun.A)), cos(RAD(_sun.A))));
  _sun.d = DEG(asin(sin(RAD(_sun.e))*sin(RAD(_sun.A))));

  /*  
  printf("day = %1.6f\n", _sun.day);
  printf("L   = %1.6f\n", _sun.L);
  printf("g   = %1.6f\n", _sun.g);
  printf("A   = %1.6f\n", _sun.A);
  printf("e   = %1.6f\n", _sun.e);
  printf("a   = %1.6f\n", _sun.a);
  printf("d   = %1.6f\n", _sun.d);
  */

  _sun.t0 = (floor(_sun.day + 0.5) - 0.5 ) / 36525.0;
  _sun.og = 6.697376 + 2400.05134 * _sun.t0 + 1.002738 * (_sun.day + 0.5 - floor(_sun.day + 0.5))*24.0;
  _sun.og = 15.0 * fmod(_sun.og, 24.0);
  _sun.tau = _sun.og + wo - _sun.a;
  _sun.AA = DEG(atan2(sin(RAD(_sun.tau)), cos(RAD(_sun.tau))*sin(RAD(sn)) - tan(RAD(_sun.d))*cos(RAD(sn))));
  _sun.h = DEG(asin(cos(RAD(_sun.d))*cos(RAD(_sun.tau))*cos(RAD(sn)) + sin(RAD(_sun.d))*sin(RAD(sn))));
  _sun.hr = _sun.h + (1.02 / tan(RAD(_sun.h + 10.3/(_sun.h + 5.11))))/60.0;

  /*
  printf("t0   = %1.6f\n", _sun.t0);
  printf("og   = %1.6f\n", _sun.og);
  printf("tau  = %1.6f\n", _sun.tau);
  printf("AA   = %1.6f\n", _sun.AA); /+ süden ist 0, westen ist 90 +/
  printf("h    = %1.6f\n", _sun.h);
  printf("hr   = %1.6f\n", _sun.hr);
  */

  {
    unsigned char dir;
    char *dirname[17] = {
      "N","NNO","NO","ONO","O","OSO","SO","SSO",
      "S","SSW","SW","WSW","W","WNW","NW","NNW", "N"
    };
    dir = (int)floor(8.5 + _sun.AA/22.5);
    if (_sun.hr >= -5.0)
      {
	printf("Sonne: von %s (%1.1f Grad) Hoehe %1.1f Grad\n",  
	       dirname[dir], fmod(180.0 + _sun.AA, 360.0),
	       _sun.hr);
      }
    else
      {
	printf("Sonne: untergegangen\n");
      }
  }
}
