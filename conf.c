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
#include <ctype.h>
#include <string.h>

#include "yali.h"

/* initial empty list of lights */
struct lights_s *_lights = NULL;

/* initial (default) configuration values */
struct conf_s _conf =
  {
    0,    /* show LCN traffic */
    0,    /* show TCP traffic */
    4711, /* default server TCP port */
    "/dev/tty.usbserial", /* default name of serial device for LCN-PK */
    NULL,  /* basename of LCN binary log files */
    "myconf.yali" /* name of server config file */
  };


/*! \brief Add light to database
 *  \param module module ID
 *  \param output output number (1,2,3)
 *  \param state state of the output (0=off, 100=on,  -1=unknown)
 *  \param name string associated with the module/output
 *  \return N/A
 */
void confLightAdd(int module, int output, int state, char *name)
{
  static struct lights_s *lp = NULL;
  struct lights_s *tmp;

  tmp = (struct lights_s*) malloc(sizeof(struct lights_s));
  if (!tmp)
    {
      printf("out of memory");
      exit(1);
    }
  
  tmp->module = module;
  tmp->output = output;
  tmp->name = strdup(name);
  tmp->state = state;
  tmp->time = 0;
  tmp->next = NULL;
  
  /*printf("%i: m%i/%i \"%s\"\n", line, m, o, cbuf+i);*/
  
  if (lp)
    {
      lp->next = tmp;
    }
  else
    {
      _lights = tmp;
    }

  lp = tmp;
}


/*! \brief Load configuration from file (list of lights)
 *  \param filename name of configuration file
 *  \return 0:OK, 1:ERROR
 */
int confLoad(char *filename)
{
  FILE *fp;
  char cbuf[1024];
  int i;
  int y;
  int m,o;
  int line;
  char type;
  float p1, p2;
 
  fp = fopen(filename, "r");
  if (!fp)
    {
      perror("opening config file");
      return 1;
    }

  line = 0;
  while (fgets(cbuf, sizeof(cbuf), fp))
    {
      line++;

      i = 0;
      while (isspace(cbuf[i])) i++;
      if (cbuf[i] == '#') continue;
      if (cbuf[i] < 32) continue;

      type = cbuf[i];
      i++;
      while (isspace(cbuf[i])) i++;

      if (sscanf(cbuf+i, "%i %i", &m, &o) != 2)
	{
	  printf("%s:%i:error expect two numbers followed by string\n", filename, line);
	  fclose(fp);
	  return 1;
	}
      
      while (cbuf[i]!='\"' && cbuf[i]) i++;
      if (cbuf[i])
	{
	  i++;
	  y = i;
	  while (cbuf[y]!='\"' && cbuf[y]) y++;
	  cbuf[y] = 0;
	  
	  if (type == 'L')
	    {
	      confLightAdd(m, o, -1, cbuf+i);
	    }

	  if (type == 'S')
	    {
	      if (sscanf(cbuf+y+1, " %f %f", &p1, &p2) != 2)
		{
		  printf("%s:%i:error expect two numbers after the string\n", filename, line);
		  fclose(fp);
		  return 1;
		}
	      stateShutCreate(m, o, cbuf+i, p1, p2);
	    }
	}
    }
 
  fclose(fp);
  return 0;
}
