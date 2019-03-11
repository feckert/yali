/*
  YALI - Yet Another LCN Interface

Copyright (C) 2009 Daniel Dallmann

This program is free software; you can redistribute it and/or modify it under 
the terms of the GNU General Public License as published by the 
Free Software Foundation; either version 3 of the License, 
or (at your option) any later version.

This program is distributed in the hope that it will be useful, but 
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 

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

struct timeQueue_s *_timeQueue = NULL;

void timeQueuePrint(void)
{
  struct timeQueue_s *p;
  int i;
  double currentTime;

  currentTime = _tick * 0.1;

  printf("Time queue:\n");
  p = _timeQueue;
  i = 1;
  while (p != NULL)
    {
      printf(" %2i : %1.3f (%+1.3f) : dst %i cmd %i\n",
	     i, p->time, p->time - currentTime, 
	     p->lcn.dst, p->lcn.cmd);
      p = p->next;
    }
}

void timeQueueAdd(struct timeQueue_s *pNew)
{
  struct timeQueue_s *p;

  if (pNew != NULL)
    {
      if (_timeQueue == NULL)
	{
	  _timeQueue = pNew;
	  pNew->prev = NULL;
	  pNew->next = NULL;
	}
      else
	{
	  p = _timeQueue;
	  while (p != NULL)
	    {
	      if (p->time > pNew->time)
		{
		  /* insert element before current */
		  pNew->next = p;
		  pNew->prev = p->prev;

		  p->prev = pNew;
		  if (pNew->prev == NULL)
		    {
		      _timeQueue = pNew;
		    }
		  else
		    {
		      pNew->prev->next = pNew;
		    }

		  break;
		}

	      if (p->next == NULL)
		{
		  /* add at end of list */
		  p->next = pNew;
		  pNew->prev = p;
		  pNew->next = NULL;
		}

	      p = p->next;
	    }
	}
    }
}

