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

#ifndef _CONF_H
#define _CONF_H

/*! \brief structure used for storing configuration */
struct conf_s
{
  unsigned char showLcnTraffic; /*!<\brief 1: print LCN traffic to stdout */
  unsigned char showTcpTraffic; /*!<\brief 1: print TCP traffic to stdout */
  unsigned short tcpPort;       /*!<\brief port number used for the server */
  char *lcnInterface;           /*!<\brief device name of the serial port */
  char *lcnBinLogBasename;      /*!<\brief base path+name for LCN binary logs */
  char *serverConfFile;         /*!<\brief filename of the configuration file */
};

/*! \brief storage for configuration values */
extern struct conf_s _conf;

/*! \brief add module/output to light name association */
extern void confLightAdd(int module, int output, int state, char *name);

#endif
