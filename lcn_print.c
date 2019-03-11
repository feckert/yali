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
#include <assert.h>

#include "yali.h"

int lcnPrintEmpty(unsigned char *p);
int lcnPrintOutStat(unsigned char *p);
int lcnPrintBeep(unsigned char *p);
int lcnPrintKey(unsigned char *p);
int lcnPrintUnlockKey(unsigned char *p);
int lcnPrintRelais(unsigned char *p);
int lcnPrintDelayedKey(unsigned char *p);
int lcnPrintOutput(unsigned char *p);
int lcnPrintCalc(unsigned char *p);
int lcnPrintOStatReq(unsigned char *p);
int lcnPrintOStat(unsigned char *p);
int lcnPrintSw123(unsigned char *p);
int lcnPrintSw12(unsigned char *p);

#define DC 0xFF00

struct pkDecode_s {
  int (*func)(unsigned char *p);
  int len;
  unsigned short pat[15];
};

int lcnHexDump(unsigned char *p)
{
  return 0;
}

struct pkDecode_s _pkDecList[] =
  {
    /* function,  type, total length, pattern for bytes 6,7,8,.. */
    { lcnPrintCalc,          8, { 0x29, 0x00,   DC } }, /* add N */
    { lcnPrintOutput,        8, { 0x04,   DC, 0xFB } }, /* output 1 darker by x */
    { lcnPrintOutput,        8, { 0x05,   DC, 0xFB } }, /* output 2 darker by x */
    { lcnPrintOutput,        8, { 0x03,   DC, 0xFB } }, /* output 3 darker by x */
    { lcnPrintOutput,        8, { 0x04,   DC, 0xFC } }, /* output 1 brighter by x */
    { lcnPrintOutput,        8, { 0x05,   DC, 0xFC } }, /* output 2 brighter by x */
    { lcnPrintOutput,        8, { 0x03,   DC, 0xFC } }, /* output 3 brighter by x */
    { lcnPrintOutput,        8, { 0x04,   DC, 0xFD } }, /* switch output 1 */
    { lcnPrintOutput,        8, { 0x05,   DC, 0xFD } }, /* switch output 2 */
    { lcnPrintOutput,        8, { 0x03,   DC, 0xFD } }, /* switch output 3 */
    { lcnPrintSw123,         8, { 0x01, 0xFA,   DC } }, /* swith off output 1+2+3 */
    { lcnPrintSw123,         8, { 0x01, 0xF8,   DC } }, /* swith on output 1+2+3 */
    { lcnPrintSw123,         8, { 0x01, 0xFB,   DC } }, /* swith output 1+2+3 */
    { lcnPrintSw12,          8, { 0x01, 0xC8, 0xC8 } }, /* switch output 1+2 on fixed ramp */
    { lcnPrintSw12,          8, { 0x01, 0xFD, 0xFD } }, /* switch output 1+2 on without ramp */
    { lcnPrintSw12,          8, { 0x01, 0x00, 0x00 } }, /* switch output 1+2 off fixed ramp */
    { lcnPrintEmpty,         8, { 0x00, 0x00, 0x00 } }, /* no operation */
    { lcnPrintOutput,        8, { 0x04,   DC,   DC } }, /* swith output 1 to x */
    { lcnPrintOutput,        8, { 0x05,   DC,   DC } }, /* swith output 2 to x */
    { lcnPrintOutput,        8, { 0x03,   DC,   DC } }, /* swith output 3 to x */
    { lcnPrintUnlockKey,     8, { 0x19,   DC,   DC } }, /* unlock keys A */
    { lcnPrintUnlockKey,     8, { 0x0C,   DC,   DC } }, /* unlock keys B */
    { lcnPrintUnlockKey,     8, { 0x0D,   DC,   DC } }, /* unlock keys C */
    { lcnPrintUnlockKey,     8, { 0x11,   DC,   DC } }, /* unlock keys D */
    { lcnPrintRelais,        8, { 0x13,   DC,   DC } }, /* rolladen command */
    { lcnPrintDelayedKey,    8, { 0x08,   DC,   DC } }, /* send keys A delayed */
    { lcnPrintDelayedKey,    8, { 0x1B,   DC,   DC } }, /* send keys B delayed */
    { lcnPrintDelayedKey,    8, { 0x1C,   DC,   DC } }, /* send keys C delayed */
    { lcnPrintDelayedKey,    8, { 0x1D,   DC,   DC } }, /* send keys D delayed */
    { lcnPrintBeep,          8, { 0x40,   DC,   DC } }, /* beep */
    { lcnPrintKey,           8, { 0x17,   DC,   DC } }, /* send keys */
    { lcnPrintOutStat,       8, { 0x68, 0x0F10, DC } }, /* output 1 status report */
    { lcnPrintOutStat,       8, { 0x68, 0x0F20, DC } }, /* output 2 status report */
    { lcnPrintOStat,        20, { 0x6E, 0x7B, 0x01, DC,DC,DC,DC,DC,DC,DC,DC,DC,DC,DC,DC } }, /* output report */
    { NULL, 0, {} }
  };

int lcnPkDecode(unsigned char *p, int len)
{
  int idx;
  int i;
  int mask;
  int val;
  int tmp;

  if (len==0) return 1; /* no match */

  i = -1;
  for (idx=0; _pkDecList[idx].func != NULL; idx++)
    {
      /*printf("%i %i\n", _pkDecList[idx].len, len);*/
      if (_pkDecList[idx].len == len)
	{
	  for (i=5; i<len; i++)
	    {
	      mask = (0xFF ^ (_pkDecList[idx].pat[i-5] >> 8)) & 0xFF;
	      val = _pkDecList[idx].pat[i-5] & 0xFF;
	      tmp = p[i] & mask;
	      /*printf("  %i %i\n", tmp, val);*/
	      if ( tmp != val ) break; /* mismatch */
	    }
	  if (i==len) break; /* match */
	  i = -1;
	}
    }

  if (i != -1)
    {
      /*printf("idx=%i\n",idx);*/
      return _pkDecList[idx].func(p);
    }
  else
    {
      return 1; /* no match */
    }
}

float lcnDecodeRamp(int n)
{
  if (n<6) return 0.25 * n;
  if (n<9) return (0.5 * n) - 1.0;
  else return (2.0 * n) - 14.0;
}

void lcnPrintTime(int n)
{
  if (n < 0x3D) printf("%i s", n);
  else if (n < 0x97) printf("%i m", n - 0x3d + 1);
  else if (n < 0xC9) printf("%i h", n - 0x97 + 1);
  else printf("%i d", n - 0xC9 + 1);
}

int lcnPrintEmpty(unsigned char *p)
{
  printf("empty command");
  return 0;
}

int lcnPrintOutStat(unsigned char *p)
{
  if (p[5]==0x68 && (p[6]&0xF0)==0x10)
    {
      printf("Output 1 at %i %%", 2*p[7]);
      return 0;
    }
  else if (p[5]==0x68 && (p[6]&0xF0)==0x20)
    {
      printf("Output 2 at %i %%", 2*p[7]);
      return 0;
    }

  return 1;
}

int lcnPrintBeep(unsigned char *p)
{
  if (p[6]<2)
    {
      printf("beep-%i, %i times\n", p[6], p[7]);
    
    }
  return 0;
}

int lcnPrintKey(unsigned char *p)
{
  char *tas[4] = {"", " %c-short", " %c-long", " %c-release"};
  int i;

  printf("key");
  for (i=0; i<8; i++)
    {
      if (p[7] & (1<<i)) printf(" %i", i+1);
    }

  i = p[6] & 0x03;
  printf(tas[i], 'A');

  i = (p[6]>>2) & 0x03;
  printf(tas[i], 'B');

  i = (p[6]>>4) & 0x03;
  printf(tas[i], 'C');

  i = (p[6]>>6) & 0x03;
  printf(tas[i], 'D');

  return 0;
}

int lcnPrintUnlockKey(unsigned char *p)
{
  char *tas[4] = {"", " %i-short", " %i-long", " %i-release"};
  char key;
  int i;
  int tmp;

  switch (p[5])
    {
    case 0x19: key = 'A'; break;
    case 0x0C: key = 'B'; break;
    case 0x0D: key = 'C'; break;
    case 0x11: key = 'D'; break;
    default: return 1;
    }

  printf("unlock key %c", key);
  for (i=0; i<8; i++)
    {
      tmp = ((p[6]>>i) & 1) + 2*((p[7]>>i) & 1);
      printf(tas[tmp], i+1);
    }

  return 0;
}

int lcnPrintRelais(unsigned char *p)
{
  int i;
  int tmp;

  printf("rolladen");
  for (i=0; i<4; i++)
    {
      tmp = (((p[6]>>(i*2))&3)<<4) + ((p[7]>>(i*2))&3);
      if (tmp==0)
	{
	}
      else if (tmp==0x11)
	{
	  printf(" %i-stop", i+1);
	}
      else if (tmp==0x32)
	{
	  printf(" %i-up", i+1);
	}
      else if (tmp==0x30)
	{
	  printf(" %i-down", i+1);
	}
      else printf(" %i-%02X", i+1, tmp);
    }

  return 0;
}

int lcnPrintDelayedKey(unsigned char *p)
{
  char key;
  int i;

  switch (p[5])
    {
    case 0x08: key = 'A'; break;
    case 0x1B: key = 'B'; break;
    case 0x1C: key = 'C'; break;
    case 0x1D: key = 'D'; break;
    default: return 1;
    }

  printf(" key");
  for (i=0; i<8; i++) if (p[7] & (1<<i)) printf(" %c%i", key, i+1);
  printf(" in ");
  lcnPrintTime(p[6]);

  return 0;
}

int lcnPrintOutput(unsigned char *p)
{
  char outp;
  int flag = 1;

  switch (p[5])
    {
    case 0x04: outp = '1'; break;
    case 0x05: outp = '2'; break;
    case 0x03: outp = '3'; break;
    default: return 1;
    }
  
  printf("Output %c ", outp);

  if (p[6]==0xFD)
    {
      printf("switch, ramp %1.1f s",
	     lcnDecodeRamp(p[7]));
      flag = 0;
    }
  else if (p[6]<0xFD && p[7]<=0xFA)
    {
      printf("to %i %%, ramp %1.1f s",
	     2*p[6], lcnDecodeRamp(p[7]));
      flag = 0;
    }
  else if (p[7]==0xFC)
    {
      printf("%1.1f %% brighter", 0.5*p[6]);
      flag = 0;
    }
  else if (p[7]==0xFB)
    {
      printf("%1.1f %% darker", 0.5*p[6]);
      flag = 0;
    }

  return flag;
}

int lcnPrintCalc(unsigned char *p)
{
  if (p[6]==0)
    {
      printf("add %i", p[7]);
      return 0;
    }

  return 1;
}

int lcnPrintOStatReq(unsigned char *p)
{
  printf("request output status");
  return 0;
}

int lcnPrintOStat(unsigned char *p)
{
  printf(" O1 = %1.1f%% (%1.1f%%, %1.1fs)"
	 "  O2 = %1.1f%% (%1.1f%%, %1.1fs)"
	 "  O3 = %1.1f%% (%1.1f%%, %1.1fs)",
	 0.5*p[8], 0.5*p[9], lcnDecodeRamp(p[10]),
	 0.5*p[11], 0.5*p[12], lcnDecodeRamp(p[13]),
	 0.5*p[14], 0.5*p[15], lcnDecodeRamp(p[16])
	 );
  return 0;
}

int lcnPrintSw123(unsigned char *p)
{
  char *op;
  
  switch (p[6])
    {
    case 0xFA: op = "off"; break;
    case 0xF8: op = "on"; break;
    case 0xFB: op = ""; break;
    default: return 1;
    }

  printf("swutch outputs 1,2,3 %s", op);
  return 0;
}

int lcnPrintSw12(unsigned char *p)
{
  char *op;
  
  switch (p[6])
    {
    case 0xC8: op = "on, fixed ramp"; break;
    case 0xFD: op = "on, no ramp"; break;
    case 0x00: op = "off, fixed ramp"; break;
    default: return 1;
    }

  printf("swutch outputs 1,2 %s", op);
  return 0;
}

void lcnPrint2(unsigned char *p, int len)
{
  unsigned int crc;
  int i;
  unsigned int source;
  unsigned int info;
  unsigned int dest;
  unsigned int destSeg;
  unsigned int cmd;
  unsigned int p1;
  unsigned int p2;

  if (len<6)
    {
      for (i=0; i<len; i++)
	{
	  printf(" %02X", p[i]);
	  printf("\n");
	}
      return;
    }

  crc = lcnCrcCalc(p, len);

  source = 0;
  for (i=1; i<256; i*=2)
    {
      source *= 2;
      if ( (i & p[0])!=0 )
	{
	  source += 1;
	}
    }

  info = p[1];
  destSeg = p[3];
  dest = p[4];
  cmd = p[5];
  p1 = p[6];
  p2 = p[7];

  printf("M%02i->", source);

  if ( info==7 ) printf("%i/G%02i", destSeg, dest);
  else if ( (info & 0xFE)==4 ) printf("%i/M%02i", destSeg, dest);
  else if ( info==6 ) printf("%i/%03i", destSeg, dest);
  else if ( info==0) printf("%i/M%02i", destSeg, dest);
  else printf("%i/%03i", destSeg, dest);

  if (crc != p[2]) printf(" (CRC error)");

  printf(" ");
  i = lcnPkDecode(p, len);
  if (i==1)
    {
      for (i=5; i<len; i++)
	{
	  printf(" %02X", p[i]);
	}
    }

  printf("\n");
}

void lcnPrint(unsigned char *p, int len)
{
  unsigned int crc;
  int i;
  unsigned int source;
  unsigned int info;
  unsigned int dest;
  unsigned int destSeg;
  unsigned int cmd;
  unsigned int flag;
  unsigned int tmp;
  unsigned int p1;
  unsigned int p2;

  if (len<6)
    {
      printf("len=%i (illegal)\n", len);
      return;
    }

  crc = lcnCrcCalc(p, len);

  source = 0;
  for (i=1; i<256; i*=2)
    {
      source *= 2;
      if ( (i & p[0])!=0 )
	{
	  source += 1;
	}
    }

  info = p[1];
  destSeg = p[3];
  dest = p[4];
  cmd = p[5];
  p1 = p[6];
  p2 = p[7];

  if (crc==p[2])
    {
      flag = 0;

      switch (info)
	{
	case 0:
	    printf("Leerkommando von %i -", source);
	    for (i=3; i<len; i++)
	      {
		printf(" 0x%02X", p[i]);
	      }
	  printf("\n");
	  flag = 1;
	  break;
	case 6:
	  printf("status von M%02i an M%02i :", source, dest);
	  if (p[5]==0x68 && (p[6]&0xF0)==0x10)
	    {
	      printf(" Ausgang 1 auf %i %%", 2*p[7]);
	    }
	  else if (p[5]==0x68 && (p[6]&0xF0)==0x20)
	    {
	      printf(" Ausgang 2 auf %i %%", 2*p[7]);
	    }
	  for (i=5; i<len; i++)
	    {
	      printf(" 0x%02X", p[i]);
	    }
	  printf("\n");
	  flag = 1;
	  break;
	case 4:
	case 5:
	case 7:
	  printf("TC von M%02i an %s%02i%s :", 
		 source, (info==7) ? "Gruppe ":"M", dest,
		 (info & 1)==0 ? "":" ACK");
	  
	  switch (cmd)
	    {
	    case 0x40:
	      if (p1<2)
		{
		  printf(" ton %i, %i mal\n", p1, p2);
		}
	      flag = 1;
	      break;
	    case 0x17:
	      printf(" tasten");
	      for (i=0; i<8; i++)
		{
		  if (p2 & (1<<i)) printf(" %i", i+1);
		}
	      {
		char *tas[4] = {"", " %c-kurz", " %c-lang", " %c-los"};
		i = p1 & 0x03;
		printf(tas[i], 'A');
		i = (p1>>2) & 0x03;
		printf(tas[i], 'B');
		i = (p1>>4) & 0x03;
		printf(tas[i], 'C');
		i = (p1>>6) & 0x03;
		printf(tas[i], 'D');
		printf("\n");
	      }
	      flag = 1;
	      break;
	    case 0x0C:
	      printf(" entsperre taste B");
	      {
		char *tas[4] = {"", " %i-kurz", " %i-lang", " %i-los"};
		for (i=0; i<8; i++)
		  {
		    tmp = ((p1>>i) & 1) + 2*((p2>>i) & 1);
		    printf(tas[tmp], i+1);
		  }
	      }
	      printf("\n");
	      flag = 1;
	      break;
	    case 0x0D:
	      printf(" entsperre taste C");
	      {
		char *tas[4] = {"", " %i-kurz", " %i-lang", " %i-los"};
		for (i=0; i<8; i++)
		  {
		    tmp = ((p1>>i) & 1) + 2*((p2>>i) & 1);
		    printf(tas[tmp], i+1);
		  }
	      }
	      printf("\n");
	      flag = 1;
	      break;
	    case 0x11:
	      printf(" entsperre taste D");
	      {
		char *tas[4] = {"", " %i-kurz", " %i-lang", " %i-los"};
		for (i=0; i<8; i++)
		  {
		    tmp = ((p1>>i) & 1) + 2*((p2>>i) & 1);
		    printf(tas[tmp], i+1);
		  }
	      }
	      printf("\n");
	      flag = 1;
	      break;
	    case 0x19:
	      printf(" entsperre taste A");
	      {
		char *tas[4] = {"", " %i-kurz", " %i-lang", " %i-los"};
		for (i=0; i<8; i++)
		  {
		    tmp = ((p1>>i) & 1) + 2*((p2>>i) & 1);
		    printf(tas[tmp], i+1);
		  }
	      }
	      printf("\n");
	      flag = 1;
	      break;
	    case 0x13:
	      printf(" rolladen");
	      for (i=0; i<4; i++)
		{
		  tmp = (((p1>>(i*2))&3)<<4) + ((p2>>(i*2))&3);
		  if (tmp==0)
		    {
		    }
		  else if (tmp==0x11)
		    {
		      printf(" %i-stop", i+1);
		    }
		  else if (tmp==0x32)
		    {
		      printf(" %i-hoch", i+1);
		    }
		  else if (tmp==0x30)
		    {
		      printf(" %i-runter", i+1);
		    }
		  else printf(" %i-%02X", i+1, tmp);
		}
	      printf("\n");
	      flag = 1;
	      break;
	    case 0x08:
	      printf(" sende taste");
	      for (i=0; i<8; i++) if (p2 & (1<<i)) printf(" A%i", i+1);
	      printf(" in ");
	      lcnPrintTime(p1);
	      printf("\n");
	      flag = 1;
	      break;
	    case 0x1B:
	      printf(" sende taste");
	      for (i=0; i<8; i++) if (p2 & (1<<i)) printf(" B%i", i+1);
	      printf(" in ");
	      lcnPrintTime(p1);
	      printf("\n");
	      flag = 1;
	      break;
	    case 0x1C:
	      printf(" sende taste");
	      for (i=0; i<8; i++) if (p2 & (1<<i)) printf(" C%i", i+1);
	      printf(" in ");
	      lcnPrintTime(p1);
	      printf("\n");
	      flag = 1;
	      break;
	    case 0x1D:
	      printf(" sende taste");
	      for (i=0; i<8; i++) if (p2 & (1<<i)) printf(" D%i", i+1);
	      printf(" in ");
	      lcnPrintTime(p1);
	      printf("\n");
	      flag = 1;
	      break;
	    case 0x04:
	      if (p1==0xFD)
		{
		  printf(" Ausgang 1 UM rampe %1.1f s\n",
			 lcnDecodeRamp(p2));
		  flag = 1;
		}
	      else if (p1<0xFD && p2<=0xFA)
		{
		  printf(" Ausgang 1 auf %i %% rampe %1.1f s\n",
			 2*p1, lcnDecodeRamp(p2));
		  flag = 1;
		}
	      else if (p2==0xFC)
		{
		  printf(" Ausgang 1 %1.1f %%-Punkte heller\n", 0.5*p1);
		  flag = 1;
		}
	      else if (p2==0xFB)
		{
		  printf(" Ausgang 1 %1.1f %%-Punkte dunkler\n", 0.5*p1);
		  flag = 1;
		}
	      break;
	    case 0x05:
	      if (p1==0xFD)
		{
		  printf(" Ausgang 2 UM rampe %1.1f s\n",
			 lcnDecodeRamp(p2));
		  flag = 1;
		}
	      else if (p1<0xFD && p2<=0xFA)
		{
		  printf(" Ausgang 2 auf %i %% rampe %1.1f s\n",
			 2*p1, lcnDecodeRamp(p2));
		  flag = 1;
		}
	      else if (p2==0xFC)
		{
		  printf(" Ausgang 2 %1.1f %%-Punkte heller\n", 0.5*p1);
		  flag = 1;
		}
	      else if (p2==0xFB)
		{
		  printf(" Ausgang 2 %1.1f %%-Punkte dunkler\n", 0.5*p1);
		  flag = 1;
		}
	      break;
	    case 0x03:
	      if (p1==0xFD)
		{
		  printf(" Ausgang 3 UM rampe %1.1f s\n",
			 lcnDecodeRamp(p2));
		  flag = 1;
		}
	      else if (p1<0xFD && p2<=0xFA)
		{
		  printf(" Ausgang 3 auf %i %% rampe %1.1f s\n",
			 2*p1, lcnDecodeRamp(p2));
		  flag = 1;
		}
	      else if (p2==0xFC)
		{
		  printf(" Ausgang 3 %1.1f %%-Punkte heller\n", 0.5*p1);
		  flag = 1;
		}
	      else if (p2==0xFB)
		{
		  printf(" Ausgang 3 %1.1f %%-Punkte dunkler\n", 0.5*p1);
		  flag = 1;
		}
	      break;
	    case 0x29:
	      if (p1==0)
		{
		  printf(" addiere %i\n", p2);
		  flag = 1;
		}
	      break;
	    case 0x6E:
	      if (p1==0xFB && p2==0x01)
		{
		  printf(" abfrage status ausgang\n");
		  flag = 1;
		}
	      else
		{
		  printf(" ?? cmd=0x%02X P1=0x%02X P2=0x%02X",
			 cmd, p1, p2);
		  for (i=8; i<len; i++)
		    {
		      printf(" 0x%02X", p[i]);
		    }
		  printf("\n");
		  flag = 1;
		}
	      break;
	    default:
	      printf(" cmd=0x%02X P1=0x%02X P2=0x%02X",
		     cmd, p1, p2);
	      for (i=8; i<len; i++)
		{
		  printf(" 0x%02X", p[i]);
		}
	      printf("\n");
	      flag = 1;
	    }

	  break;

	case 12:
	  if (cmd==0x6E && p1==0x7B && p2==0x01)
	    {
	      printf("TM von M%02i an M%02i :"
		     " A1 = %1.1f%% (%1.1f%%, %1.1fs)"
		     "  A2 = %1.1f%% (%1.1f%%, %1.1fs)"
		     "  A3 = %1.1f%% (%1.1f%%, %1.1fs)", source, dest,
		     0.5*p[8], 0.5*p[9], lcnDecodeRamp(p[10]),
		     0.5*p[11], 0.5*p[12], lcnDecodeRamp(p[13]),
		     0.5*p[14], 0.5*p[15], lcnDecodeRamp(p[16])
		     );
	      /*
	      for (i=8; i<len; i++) printf(" 0x%02X", p[i]);
	      */
	      printf("\n");
	      flag = 1;
	    }
	  break;
	}

      if (flag) return;
    }

  printf("?? Src=M%02i Info=%i Dest=M%02i%s", source, info, dest, crc!=p[2] ? " CRC":"");

  /*
  printf("Src=%i Info=0x%02X %s Dest=%i/%02i Cmd=0x%02X",
	 source, info, (crc==p[2]) ? "Ok ":"CRC", cmd, p1, p2);
  */
  
  for (i=5; i<len; i++)
    {
      printf(" 0x%02X", p[i]);
    }

  printf("\n");
}
