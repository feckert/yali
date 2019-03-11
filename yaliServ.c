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

#include "yali.h"

unsigned long volatile _tick = 0;

unsigned short _yaliVersionMayor = 1;
unsigned short _yaliVersionMinor = 0;

char *_yaliVersionText = "Yali Server-C V1.0";

unsigned long _yaliTime = 0;

/* obtain current time and write to global variable */
void yaliTimeAdapt(void)
{
  time_t t;

  t = time(NULL);
  _yaliTime = (unsigned long) t;
}

/* specified module need refresh soon */
void yaliScheduleRefresh(int inModule)
{
  struct lights_s *lp;

  for (lp = _lights; lp!=NULL; lp=lp->next)
    {
      if (lp->module == inModule)
	{
	  lp->time = _yaliTime - 55;
	}
    }
}

/* do refresh */
void yaliRefresh(void)
{
  signed long tmp;
  signed long max;
  int module;
  struct lights_s *lp;
  struct lights_s *lp2;
  static unsigned long ltime = 0;

  yaliTimeAdapt();

  /* timed sending */
  {
    struct timeQueue_s *p = _timeQueue;

    if (p != NULL && p->time <= _tick)
      {
	lcnQueueCmdAdd(&p->lcn, 8);
	_timeQueue = p->next;

#ifdef DBG
	printf("TIMED:: ");
	lcnPrint((unsigned char*) &p->lcn, 8);
#endif

	free(p);
	return;
      }
  }

  if (ltime==_yaliTime) return;

  module = 1;
  
  max = 0;
  for (lp = _lights; lp!=NULL; lp=lp->next)
    {
      tmp = _yaliTime - (lp->time + 600);
      if (tmp > max)
	{
	  lp2 = lp;
	  max = tmp;
	  module = lp->module;
	}
    }  

  if (max > 0)
    {
      /* request status for module */
      lcnQueueCommandSend(_lcnSerFd, module, 0x6E, 0xFB, 0x01);
      lp2->time = _yaliTime;
      ltime = _yaliTime;
    }
}

void handleSigPipe()
{
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
    val.it_interval.tv_sec = 0; 
    val.it_interval.tv_usec = 100000;
 
    setitimer(ITIMER_REAL,&val,0);    /* start the timer */    
}

void usage(char *appname)
{
  printf("%s: [-hv] [-p <port>] [-i <interface>] [-b <binlog_prefix>] [-c <config>]\n", appname);
}

int parse_cmdline(int argc, char **argv)
{
    int i, y;

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
                            usage(argv[0]);
                            exit(0);
                        }

                    case 'v':
                        {
			  _conf.showLcnTraffic = 1;
			  _conf.showTcpTraffic = 1;
			  break;
                        }

                    case 'p':
                        {
                            i++;
			    _conf.tcpPort = atoi(argv[i]);
                            y = 0;
                            break;
                        }

                    case 'i':
                        {
                            i++;
                            _conf.lcnInterface = argv[i];
                            y = 0;
                            break;
                        }

                    case 'c':
                        {
                            i++;
                            _conf.serverConfFile = argv[i];
                            y = 0;
                            break;
                        }

                    case 'b':
                        {
                            i++;
                            _conf.lcnBinLogBasename = argv[i];
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
                    usage(argv[0]);
                    return 1;
                }
                if (y == 0)
                    break;
                y++;
            }
        }
        else
        {
	  usage(argv[0]);
	  return 1;
        }
        i++;
    }

    return 0;
}

int main(int argc, char **argv)
{
  int srvSock;
  int tmp;
  int i;
  fd_set readfs;
  fd_set errorfs;
  int maxfd;
  char *cp;

  /*stateSunCalc();*/

  cp = getenv("YALI_PORT");
  if (cp!=NULL)
    {
      _conf.tcpPort = atoi(cp);
    }

  i = parse_cmdline(argc, argv);
  if (i!=0) exit(1);

  i = confLoad(_conf.serverConfFile);
  if (i!=0)
    {
      fprintf(stderr, "error loading configuration file \"%s\"\n", _conf.serverConfFile);
      exit(1);
    }

  i = open_lcnport();
  if (i<0)
    {
      fprintf(stderr, "error %s not available\n", _conf.lcnInterface);
      exit(1);
    }

  timer_start();

  srvSock = netServerOpen();

  /*
  printf("%s (Version %i.%i)\n",
	 _yaliVersionText, _yaliVersionMayor, _yaliVersionMinor);
  */

  signal(SIGPIPE, handleSigPipe);    /* handler for SIGPIPE */

  while (1)
    {
      netClientAccept(srvSock);

      if (_conf.lcnInterface)
	{
	  static unsigned long ltime = 0;
	  /* this should be fixed somewhen */
	  if (ltime != _tick)
	    {
	      ltime = _tick;
	      lcnSendNext(_lcnSerFd);
	      yaliRefresh();
	    }
	}

      maxfd = _lcnSerFd;

      FD_ZERO(&readfs);
      FD_ZERO(&errorfs);

      if (_conf.lcnInterface) FD_SET(_lcnSerFd, &readfs);

      for (i=0; i<CLI_NUM; i++)
	{
	  if (_cli[i].sf != -1)
	    {
	      FD_SET(_cli[i].sf, &readfs);
	      FD_SET(_cli[i].sf, &errorfs);
	      if (_cli[i].sf > maxfd) maxfd = _cli[i].sf;
	    }
	}

      maxfd += 1;

      tmp = select(maxfd, &readfs, NULL, &errorfs, NULL);
      if (tmp == -1)
	{
	  stateShutCheck();
	  continue;
	}

      if (_conf.lcnInterface && FD_ISSET(_lcnSerFd, &readfs))
	{
	  lcnSerDataGet(_lcnSerFd);
	}

      for (i=0; i<CLI_NUM; i++)
	{
	  if (_cli[i].sf != -1)
	    {
	      if (FD_ISSET(_cli[i].sf, &readfs))
		{
		  /*printf("input on client %i\n", i);*/
		  netSockDataGet(i);
		}
	      else if (FD_ISSET(_cli[i].sf, &errorfs))
		{
		  fprintf(stderr, "error on client %i\n", i);
		  netSockTerm(i);
		}
	    }
	}
    }

  close(srvSock);

  return 0;
}
