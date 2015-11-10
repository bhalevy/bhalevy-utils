/* 
 *  
 *  TONEGEN
 *  
 *  Plays a sine wave via the dsp or standard out.
 *  
 *  Copyright (C) 2000 Timothy Pozar pozar@lns.com
 *  
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *  
 *  Version.Revision
 *  ----------------
 *  1.5 - Added some ifdefs to compile better on Linux.
 *
 *  1.4 - Counts number of cycles now to be accurate with STDOUT timed 
 *        playing.
 *
 *  1.3 - Buffered the output (0.1 sec) to not hiccup on a buzy system.
 *
 *  1.2 - Tones stop at a zero crossing when you define a time to run
 *        reduce nasty pops.
 *
 *  1.1 - Found a bug in my sine code where I was adding a duplicate
 *        sample.  The tone should be "more" pure now.
 *
 *  1.0 - First release to a friend.  
 *  
 *  This program needs the math lib.  Compile with something like...  
 *  
 *                   cc -lm -o tonegen tonegen.c
 *  
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <math.h>

#ifdef LINUX
#include <linux/soundcard.h>
#define DSP "/dev/dsp"
#else
#include <machine/soundcard.h>
#define DSP "/dev/dspW"
#endif

#define TRUE 1
#define FALSE 0
#define BUF_SIZE 4096

char verrev[BUF_SIZE] = "$Id: tonegen.c,v 1.4 2000/07/04 06:15:39 pozar Exp pozar $";

char device[BUF_SIZE] = DSP;
int stereo = FALSE;
int rate = 44100;
int freq = 400;
int time_sec = 0;
int elapst_sec = 0;
int db_down = 0;
int devfh;
int stdout_flg = FALSE;
int main(argc,argv)
int argc;
char *argv[];
{
char buf[BUF_SIZE];
int gotmask, setmask, i, len, test;
int cyclesamples;
double ratio = 1;
short int dspnum;
unsigned int temp1, temp2;
float f = 0;
float sinnum;
char *p;

   /* scan command line arguments, and look for files to work on. */
   for (i = 1; i < argc; i++) {
      switch (argv[i][1]){   /* be case indepenent... */
         case 'D':   /* dsp file name... */
         case 'd':
            i++;
            if('-' == argv[i][0]){
               stdout_flg = TRUE;
               strcpy(device,"/dev/tty");
            } else{ 
               strncpy(device,argv[i],BUF_SIZE);
            }
            break;
         case 'F':   /* tone freqency... */
         case 'f':
            i++;
            freq = atoi(argv[i]);
            break;
         case 'A':   /* Attenuate in dB... */
         case 'a':
            i++;
            db_down = atoi(argv[i]);
            ratio = pow(10,(db_down/20.0));
            if(ratio == 0){
               ratio = 1;
               printf("ratio = 0, reseting to 1\n");
            }
            break;
         case 'H':   /* Help... */
         case 'h':
            banner();
            exit(0);
         case 'R':   /* Sample rate... */
         case 'r':
            i++;
            rate = atoi(argv[i]);
            break;
         case 'S':   /* Run in stereo... */
         case 's':
            stereo = TRUE;
            break;  
         case 'T':   /* Seconds to run... */
         case 't':
            i++;
            time_sec = atoi(argv[i]);
            break;
         default:
            printf("I don't know the meaning of the command line argument: \"%s\".\n",argv[i]);
            banner();
            exit(1);
      }
   }

   /* Lets do some santiy checking on what we are to do... */
   if(freq >= (rate/2)){
      printf("Tone frequency of %i cannot be reproduced with a sample rate of %i. \nTry a tone below %i.\nExiting...\n",freq,rate,rate/2);
      exit(1);
   }
   if(rate >= 44101){
      printf("This sample rate of %i can not be over 44100 Hz.\nExiting...\n",rate);
      exit(1);
   }

   if(!stdout_flg){
      printf("The \"sample rate\" is %i.  The tone is %i Hz at %i dB down",rate, freq, db_down);
      if(time_sec == 0)
         printf(".\n");
      else
         printf(" for %i seconds.\n",time_sec);
   }

   p=malloc(rate*2*.15);	/* This will be used for the buffer 
				    to write to the dsp.  The buffer
				    size is rate * 2 for 16 bit
				    audio and 15% of it */

   if(!stdout_flg){
      if((devfh = open(device, O_RDWR)) == -1){
         perror("opening device");
         exit(1);
      }

      /* What formats does this device support? */
      if(ioctl(devfh, SNDCTL_DSP_GETFMTS, &gotmask) == -1){
         perror("get dsp mask");
         exit(1);
      }

      /* Set the number or channels (ie mono vs. stereo)...
         Always set stereo/mono first before sample rate. 
         See http://www.4front-tech.com/pguide/audio.html for details. */
      test = stereo;
      if(ioctl(devfh, SNDCTL_DSP_STEREO, &stereo) == -1){
         perror("Tried to set dsp to mono or stereo");
         exit(1);
      }
      if (stereo != test){
         if(stereo){
            perror("Tried to set dsp to mono but it only supports stereo.\n");
         } else {
            perror("Tried to set dsp to stereo but it only supports mono.\n");
         }
         exit(1);
      }

      /* Set the sample rate... */
      test = rate;
      if(ioctl( devfh, SNDCTL_DSP_SPEED, &test) == -1){
         perror("set sample rate");
         exit(1);
      }
      if(rate != test){
         printf("Could not set the sample rate to: \"%i\". \"%i\" was returned\n",
            rate, test);
      }

   } /* end of dsp ioctls settings */

   cyclesamples = rate / freq;

   i = 0;

   memset(p,0x7f,(rate*2*.15));
   while (1){
      sinnum = sin(f) / ratio;
      dspnum = 0x7fff * sinnum;

      memmove(p+i,&dspnum,sizeof(dspnum));
      i = i + sizeof(dspnum);

      if(i > (rate * 2 * 0.1)){ /* Lets only go to 10% of one second to not
                                 fill up the 15% buffer */
         elapst_sec = elapst_sec + 1;
         if(stdout_flg){
            if(write(1, p, i) == -1){
               perror("Trouble writing to STDOUT.");
               exit(1);
            }
         } else {
            write(devfh, p, i);
         }
         i = 0;
      }

      f = f + ((2*M_PI)/cyclesamples);
      if(f >= (2*M_PI)){     /* We have come to the end of a full cycle. */
         if (time_sec > 0){	/* If it is time to stop then do it at
	                           this nice zero crossing. */
            if (elapst_sec >= time_sec * 10){
               exit(1);
            }
         }
         f = 0;
         f = f + ((2*M_PI)/cyclesamples);
      }
   }

   return 0;
}

banner()
{
   printf("tonegen: Generates a sine wave on the sound card or standard out.\n");
   printf("   -a dB       Sets attenuation from \"all ones\" in dB.  Default is \"%i db\".\n",db_down);
   printf("   -d device   Sets device name.  Default is \"%s\".\n",device);
   printf("               If \"device\" is \"-\" then it uses STDOUT\n");
   printf("   -f Hz       Sets tone in Hertz.  Default is \"%i Hz\".\n",freq);
   printf("   -r rate     Sets device sample rate in Hertz.  Default is \"%i Hz\".\n",rate);
   /* printf("   -s          Sets device to run in stereo.  Default is mono.\n"); */
   printf("   -t seconds  Sets time to run.  Default is infinite.\n");
   printf("               The length of the tone will run over slightly until full\n");
   printf("               cycle stops at a \"zero crossing\" to prevent clicks.\n");
   return;
}

