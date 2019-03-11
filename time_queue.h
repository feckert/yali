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

#ifndef _TIME_QUEUE_H
#define _TIME_QUEUE_H

/*! \brief structure used for time LCN commands */
struct timeQueue_s
{
  double time;
  struct lcnPak_s lcn;
  struct timeQueue_s *prev;
  struct timeQueue_s *next;
};

extern struct timeQueue_s *_timeQueue;
extern void timeQueueAdd(struct timeQueue_s *pNew);
extern void timeQueuePrint(void);

#endif /* _TIME_QUEUE_H */
