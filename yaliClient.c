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
#include <time.h>
#include <arpa/inet.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/time.h>
#include <netdb.h>
#include <ctype.h>

#include "yali.h"

unsigned long volatile _tick = 0;

unsigned short _yaliVersionMayor = 1;
unsigned short _yaliVersionMinor = 0;
char *_yaliVersionText = "Yali Server-C V1.0";

unsigned long _yaliTime = 0;

char *_serverName = "localhost";
int   _serverPort = 4711;
int   _serverSock;
int   _beVerbose = 0;

struct pak_s * pakReceive(int inSock);

void yaliScheduleRefresh(int inModule)
{
}

/* obtain current time and write to global variable */
void yaliTimeAdapt(void)
{
  time_t t;

  t = time(NULL);
  _yaliTime = (unsigned long) t;
}

void announce()
{
  _tick++;
}

void timer_start()
{
  struct itimerval val;

    signal(SIGALRM,announce);    /* handler for SIGALRM */     
 
    /* set the timer for 3 seconds with an interval of 5 seconds
        on the system clock
     */
    val.it_value.tv_sec = 1;                                   
    val.it_value.tv_usec = 0;
    val.it_interval.tv_sec = 1;                                
    val.it_interval.tv_usec = 0;
 
    setitimer(ITIMER_REAL,&val,0);    /* start the timer */    
}

void yaliHistory(void)
{
  struct pak_s pk;
  struct pak_s *p;
  struct lights_s *lp;
  int i;
  unsigned long rtm;
  char dbuf[20];
  struct tm *tp;
  time_t tm;

  pk.type = NET_NETHISTGET;
  pk.len = 0;
  pk.data = NULL;
  netPakSend(_serverSock, &pk);

  do {
    p = pakReceive(_serverSock);
    if (_beVerbose)
      {
	printf("<<< ");
	netPakPrint(p);
      }
  } while (p->type != NET_NETHISTREPORT);
  
  printf("History:\n");

  for (i=0; i<p->len; i+=8)
    {
      rtm = (p->data[i] << 24) + (p->data[i+1] << 16) + (p->data[i+2] << 8) + p->data[i+3];
      if (p->data[i+4] == 1)
	{
	  lp = _lights;
	  while (lp)
	    {
	      if (lp->module == p->data[i+5]
		  && lp->output==p->data[i+6]) break;
	      lp = lp->next;
	    }
	  
	  if (lp)
	    {	      
	      tm = rtm;
	      tp = localtime(&tm);
	      strftime(dbuf, sizeof(dbuf), "%g.%m.%d %H:%M:%S", tp);

	      printf("%s Light \"%s\" %i %%\n", dbuf, lp->name, p->data[i+7]);
	    } 
	}

      if (p->data[i+4] == 2)
	{
	  struct shutter_s *pshut;

	  pshut = stateShutPtrGet(p->data[i+5], p->data[i+6]);
	  if (pshut)
	    {	      
	      tm = rtm;
	      tp = localtime(&tm);
	      strftime(dbuf, sizeof(dbuf), "%g.%m.%d %H:%M:%S", tp);

	      printf("%s Shutter \"%s\" %i %%\n", dbuf, pshut->name, p->data[i+7]);
	    } 
	}

    }
}

void yaliMonitor(void)
{
  struct pak_s *p;
  struct lights_s *lp;
  char dbuf[20];
  struct tm *tp;
  time_t tm;

  while (1)
    {
      do {
	p = pakReceive(_serverSock);
	if (_beVerbose)
	  {
	    printf("<<< ");
	    netPakPrint(p);
	  }
      } while (p->type != NET_LIGHTSTATUSREPORT);

      lp = _lights;
      while (lp)
	{
	  if (lp->module==p->data[0]
	      && lp->output==p->data[1]) break;
	  lp = lp->next;
	}

      if (lp)
	{
	  tm = time(0);
	  tp = localtime(&tm);
	  strftime(dbuf, sizeof(dbuf), "%g.%m.%d %H:%M:%S", tp);

	  printf("%s Light \"%s\" %i %%\n", dbuf, lp->name, p->data[2]);
	}
    }
}

void usageCli(char *name)
{
  printf("%s: [-s server] [-p port] [-m] [-H] [light... brightness] [light...]\n", name);
  printf("  Without specifying the name of a light + brightness,\n"
	 "  the status of all active lights is reported.\n"
	 "  If brighness is not specified the current brightness is returned.\n"
	 "  The default server is localhost, if not overwrtten by the\n"
	 "  environment variable YALI_SERVER.\n"
	 "  The default port is 4711, if not overwritten by the environment\n"
	 "  variable YALI_PORT.\n"
	 "  When -m is specified the client starts in monitor mode.\n"
	 "  When -H is specified the client obtains the history from the server.\n"
	 );
}

struct pak_s * pakReceive(int inSock)
{
  unsigned char buf[3];
  int i,n;
  struct pak_s *p;

  i = 0;
  while (i<3)
    {
      n = recv(inSock, &buf[i], 3-i, 0);
      if (n < 0) return NULL;
      i += n;
    }

  p = (struct pak_s*) malloc(sizeof(struct pak_s));
  p->type = buf[0];
  p->len = 256*buf[1] + buf[2];
  p->data = (unsigned char*) malloc(p->len);
  
  i = 0;
  while (i < p->len)
    {
      n = recv(inSock, &p->data[i], p->len - i, 0);
      if (n < 0) return NULL;
      i += n;
    }

  if (p->type==NET_ERRORREPORT)
    {
      fprintf(stderr, "server error %i: \"%s\"\n", p->data[0], p->data+1);
      exit(1);
    }

  return p;
}

void yaliListAllActiveLights(int inNum)
{
  struct lights_s *lp;

  printf("List of active lights:\n");
  lp = _lights;
  while (lp)
    {
      if (lp->state > 0)
	{
	  printf("Light \"%s\" on at %i %%\n", lp->name, lp->state);
	}
      lp = lp->next;
    }
}

void yaliLightStatPrint(char **cp, int n)
{
  int i;
  struct lights_s *lp;
  struct pak_s pk;
  struct pak_s *p;
  unsigned char buf[8];
  signed char val;

  pk.data = buf;

  for (i=0; i<n; i++)
    {
      lp = _lights;
      while (lp)
	{
	  if ( strcmp(lp->name, cp[i])==0 )
	    {
	      break;
	    }
	  lp = lp->next;
	}

      pk.type = NET_LIGHTSTATUSGET;
      pk.len = 2;
      pk.data[0] = lp->module;
      pk.data[1] = lp->output;
      
      netPakSend(_serverSock, &pk);
      
      if (_beVerbose)
	{
	  printf(">>> ");
	  netPakPrint(&pk);
	}
      
      do {
	p = pakReceive(_serverSock);
	
	if (_beVerbose)
	  {
	    printf("<<< ");
	    netPakPrint(p);
	  }
      } while (p->type != NET_LIGHTSTATUSREPORT
	       || p->data[0] != lp->module
	       || p->data[1] != lp->output);
      
      val = p->data[2];
      if (val >= 0)
	{
	  printf("Light \"%s\" = %i %%\n", lp->name, val);
	}
      else
	{
	  printf("Light \"%s\" = ? %%\n", lp->name);
	}
    }
}

void yaliLightStatSet(char **cp, int n, int val)
{
  int i;
  struct lights_s *lp;
  struct pak_s pk;
  unsigned char buf[8];

  pk.data = buf;

  for (i=0; i<n; i++)
    {
      lp = _lights;
      while (lp)
	{
	  if ( strcmp(lp->name, cp[i])==0 )
	    {
	      printf("Switching light \"%s\" to %i %%\n", cp[i], val);

	      pk.type = NET_LIGHTSTATUSSET;
	      pk.len = 3;
	      pk.data[0] = lp->module;
	      pk.data[1] = lp->output;
	      pk.data[2] = val;

	      netPakSend(_serverSock, &pk);

	      if (_beVerbose)
		{
		  printf(">>> ");
		  netPakPrint(&pk);
		}
	      break;
	    }
	  lp = lp->next;
	}
    }
}

void yaliShutList(void)
{
  struct shutter_s *sp;

  sp = _stateShutRoot;
  while (sp != NULL)
    {
      printf("Shutter \"%s\" is between %1.1f%% and %1.1f%%\n",
	     sp->name, sp->posMin, sp->posMax);
      sp = sp->next;
    }
}

void yaliShutStatSet(char **cp, int n, int valMin, int valMax)
{
  int i;
  struct pak_s pk;
  unsigned char buf[8];

  struct shutter_s *sp;

  for (i=0; i<n; i++)
    {

  pk.data = buf;
  sp = _stateShutRoot;

  while (sp != NULL)
    {
      if (strcmp(cp[i], sp->name) == 0)
	{
	  /* found suitable shutter */
	  printf("Moving Shutter \"%s\" to %i .. %i %%\n", cp[i], valMin, valMax);
	  
	  pk.type = NET_SHUTSTATUSSET;
	  pk.len = 4;
	  pk.data[0] = sp->module;
	  pk.data[1] = sp->rnum;
	  pk.data[2] = valMin;
	  pk.data[3] = valMax;
	  
	  netPakSend(_serverSock, &pk);
	  
	  if (_beVerbose)
	    {
	      printf(">>> ");
	      netPakPrint(&pk);
	    }
	  break;
	  
	}
      sp = sp->next;
    }
    }
}


int main(int argc, char **argv)
{
  int i;
  int y;
  struct sockaddr_in srv;
  struct hostent *ad;
  struct pak_s pk;
  struct pak_s *p;
  struct lights_s *lp;
  int parn = 0;
  char *par[20];
  int doMonitor = 0;
  char *cp;
  int doHist = 0;
  int doShutter = 0;
  int startPar;
  unsigned char buf[8];

  pk.data = buf;

  cp = getenv("YALI_SERVER");
  if (cp!=NULL)
    {
      _serverName = cp;
    }

  cp = getenv("YALI_PORT");
  if (cp!=NULL)
    {
      _serverPort = atoi(cp);
    }

  i = 1;
  while (i < argc)
    {
      if (argv[i][0] == '-')
        {
	  y = 1;
	  while (argv[i][y] != 0)
            {
	      switch (argv[i][y])
                {
		case 'h':
		  {
		    usageCli(argv[0]);
		    exit(0);
		  }
		  
		case 'v':
		  {
		    _beVerbose = 1;
		    break;
		  }
		  
		case 'm':
		  {
		    doMonitor = 1;
		    break;
		  }
		  
		case 'H':
		  {
		    doHist = 1;
		    break;
		  }
		  
		case 'S':
		  {
		    doShutter = 1;
		    break;
		  }
		  
		case 'p':
		  {
		    i++;
		    _serverPort = atoi(argv[i]);
		    y = 0;
		    break;
		  }
		  
		case 's':
		  {
		    i++;
		    _serverName = argv[i];
		    y = 0;
		    break;
		  }
		  
		default:
		  printf("%s: unknown option -%c\n",
			 argv[0], argv[i][y]);
		  return 1;
                }
	      if (i == argc)
                {
		  usageCli(argv[0]);
		  return 1;
                }
	      if (y == 0)
		break;
	      y++;
            }
        }
      else
        {
	  if (parn >= 20)
	    {
	      usageCli(argv[0]);
	      return 1;
	    }
	  par[parn++] = argv[i];
        }
      i++;
    }

  /*timer_start();*/

  /* determine IP address and connect to yali server */

  ad = gethostbyname(_serverName);
  if (ad==NULL)
    {
      perror("lookup failure");
      exit(1);
    }

  _serverSock = socket(AF_INET, SOCK_STREAM, 0);
  if (_serverSock == -1)
    {
      perror("socket failure");
      exit(1);
    }

  memset(&srv,0,sizeof(srv));

  srv.sin_family = AF_INET;
  srv.sin_addr.s_addr = ((struct in_addr *)(ad->h_addr))->s_addr;
  srv.sin_port = htons(_serverPort);
  
  if (_beVerbose) 
    {
      printf("Trying to connect to %s port %i\n", _serverName, _serverPort);
    }

  if (connect(_serverSock, (void *)&srv, sizeof(srv)) == -1)
    {
      perror("connect");
      exit(1);
    }
  
  /*
  printf("%s (Version %i.%i)\n",
	 _yaliVersionText, _yaliVersionMayor, _yaliVersionMinor);
  */

  /* obtain server version (TODO: compare it to local version) */

  pk.type = NET_VERSIONGET;
  pk.len = 0;

  netPakSend(_serverSock, &pk);

  if (_beVerbose)
    {
      printf(">>> ");
      netPakPrint(&pk);
    }

  do {
    p = pakReceive(_serverSock);

    if (_beVerbose)
      {
	printf("<<< ");
	netPakPrint(p);
      }
  } while (p->type != NET_VERSIONREPORT);

  /* obtain database of all lights from server */

  pk.type = NET_LIGHTDBGET;
  pk.len = 0;

  netPakSend(_serverSock, &pk);

  if (_beVerbose)
    {
      printf(">>> ");
      netPakPrint(&pk);
    }

  do {
    p = pakReceive(_serverSock);

    if (_beVerbose)
      {
	printf("<<< ");
	netPakPrint(p);
      }
  } while (p->type != NET_LIGHTDBREPORT);

  y = 0;
  i = 0;
  while (i < p->len)
    {
      y++;

      confLightAdd(p->data[i], p->data[i+1], p->data[i+2], (char*) &p->data[i+3]);
      i += 3;

      while (i < p->len && p->data[i]) i++;
      i++;
    }

  /* obtain database of all shutters from server */

  pk.type = NET_SHUTTERDBGET;
  pk.len = 0;
  
  netPakSend(_serverSock, &pk);
  
  if (_beVerbose)
    {
      printf(">>> ");
      netPakPrint(&pk);
    }
  
  do {
    p = pakReceive(_serverSock);
    
    if (_beVerbose)
      {
	printf("<<< ");
	netPakPrint(p);
      }
  } while (p->type != NET_SHUTTERDBREPORT);
  
  y = 0;
  i = 0;
  while (i < p->len)
    {
      y++;
      
      stateShutCreate(p->data[i], p->data[i+1], (char*) &p->data[i+4], 0.0, 0.0);

      _stateShutRoot->posMin = p->data[i+2];
      _stateShutRoot->posMax = p->data[i+3];

      i += 4;
      
      while (i < p->len && p->data[i]) i++;
      i++;
    }
  
  /* perform actions defined by command line parameters */
  
  if (doHist == 1)
    {
      if (parn != 0)
	{
	  usageCli(argv[0]);
	  exit(1);
	}

      yaliHistory();
      return 0;
    }

  if (doMonitor == 1)
    {
      if (parn != 0)
	{
	  usageCli(argv[0]);
	  exit(1);
	}

      yaliMonitor();
      return 0;
    }

  if (doShutter == 1)
    {
      if (parn != 0)
	{
	  yaliShutStatSet(par, 1, atoi(par[1]), atoi(par[2]));
	  exit(0);

	  usageCli(argv[0]);
	  exit(1);
	}

      yaliShutList();
      return 0;
    }

  if (parn == 0)
    {
      /* without parameters, report all lights that are ON */
      yaliListAllActiveLights(y);
      return 0;
    }

  startPar = 0;
  while (startPar < parn)
    {
      y = startPar;
      while (y < parn)
	{
	  lp = _lights;
	  while (lp)
	    {
	      if ( strcmp(lp->name, par[y])==0 ) break;
	      lp = lp->next;
	    }
	  if (lp == NULL) break;
	  y++;
	}

      if (lp != NULL)
	{
	  yaliLightStatPrint(&par[startPar], y - startPar);
	}
      else
	{
	  i = strlen(par[y]) - 1;
	  while (i >= 0 && isdigit(par[y][i])) i--;

	  if (i >= 0)
	    {
	      printf("Ligtht \"%s\" is unknown, choose one of ...\n", par[y]);
	      
	      lp = _lights;
	      while (lp)
		{
		  printf("  %s\n", lp->name);
		  lp = lp->next;
		}
	      return 1;
	    }

	  i = atoi(par[y]);
	  if (i < 0) i = 0;
	  if (i > 100) i = 100;

	  yaliLightStatSet(&par[startPar], y - startPar, i);
	  y++;
	}

      startPar = y;
    }

  close(_serverSock);
  return 0;
}
