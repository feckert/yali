##############################################################################
# JALI - Jet Another LCN Interface
#
# Copyright (C) 2009 Daniel Dallmann
#
# This program is free software; you can redistribute it and/or modify it under 
# the terms of the GNU General Public License as published by the 
# Free Software Foundation; either version 3 of the License, 
# or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but 
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
# or FITNESS FOR A PARTICULAR PURPOSE. 
# See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along 
# with this program; if not, see <http://www.gnu.org/licenses/>.
##############################################################################

.PHONY: all clean

OSX_V := $(shell uname -r|cut -d"." -f1)

CC     = gcc
CFLAGS = -g -Wall -D"OSX_V=${OSX_V}"

all: yaliServ yaliClient

OBJ := net_io.o lcn_io.o conf.o lcn_print.o state.o time_queue.o
HFILES := net_io.h lcn_io.h conf.h state.h yali.h

yaliServ: $(OBJ) yaliServ.o $(HFILES) Makefile
	$(CC) $(CFLAGS) $(OBJ) yaliServ.o -o $@ -lm

yaliClient: $(OBJ) yaliClient.o $(HFILES) Makefile
	$(CC) $(CFLAGS) $(OBJ) yaliClient.o -o $@ -lm

lcnDecode: lcn_print.o lcnDecode.o $(HFILES) Makefile
	$(CC) $(CFLAGS) lcn_print.o lcnDecode.o -o $@

%.o:%.c $(HFILES)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	-rm $(OBJ) yaliServ.o yaliClient.o lcnDecode.o yaliServ yaliClient lcnDecode
