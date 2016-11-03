/* **********************************************************************
* Copyright (C) 2016 Elliott Mitchell <ehem+android@m5p.com>		*
*									*
*	This program is free software: you can redistribute it and/or	*
*	modify it under the terms of the GNU General Public License as	*
*	published by the Free Software Foundation, either version 3 of	*
*	the License, or (at your option) any later version.		*
*									*
*	This program is distributed in the hope that it will be useful,	*
*	but WITHOUT ANY WARRANTY; without even the implied warranty of	*
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the	*
*	GNU General Public License for more details.			*
*									*
*	You should have received a copy of the GNU General Public	*
*	License along with this program.  If not, see			*
*	<http://www.gnu.org/licenses/>.					*
************************************************************************/


#define _LARGEFILE64_SOURCE

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#include "gpt.h"


int verbose=0;


int main(int argc, char **argv)
{
	enum {DEFAULT, REPLACE, MERGE} action=DEFAULT;
	bool testonly=false;
	int opt;
	struct gpt_data *devpri=NULL, *devsec=NULL, *newpri=NULL, *newsec=NULL;
	int dev, new0, new1=-1;

	while((opt=getopt(argc, argv, "hrmtvq"))!=-1) {
		switch(opt) {
		case 'r':
			if(action!=DEFAULT) {
				fprintf(stderr, "Only one of -m and -r can be used!\n");
				exit(128);
			}
			action=REPLACE;
			break;
		case 'm':
			if(action!=DEFAULT) {
				fprintf(stderr, "Only one of -m and -r can be used!\n");
				exit(128);
			}
			action=MERGE;
			break;
		case 't':
			testonly=true;
			break;
		case 'v':
			++verbose;
			break;
		case 'q':
			verbose=-1;
			break;
		default:
			fprintf(stderr,
"Usage: %s [-m | -r] [-t] <device> <replacement-GPT.bin>\n" "\n"
"optional arguments:\n"
"  -h, --help            show this help message and exit\n"
"  -m                    merge GPT copies in\n"
"  -r                    replace GPT on device with GPT images from .bin files"
"\n", argv[0]);
			exit(opt=='h'?0:128);
		}
	}
	switch(argc-optind) {
	case 3:
		if((new1=open(argv[optind+2], O_RDONLY|O_LARGEFILE))<0) {
			perror("open()ing second GPT.bin file failed");
			exit(32);
		}
	case 2:
		if((new0=open(argv[optind+1], O_RDONLY|O_LARGEFILE))<0) {
			perror("open()ing first GPT.bin file failed");
			exit(32);
		}
		if((dev=open(argv[optind], O_RDWR|O_LARGEFILE|O_SYNC))<0) {
			perror("open()ing MMC device failed");
			exit(32);
		}
		break;
	case 1:
	case 0:
		fprintf(stderr, "%s: too few arguments, need target device/image and a GPT image\n", argv[0]);
		exit(64);
		break;
	default:
		fprintf(stderr, "%s: too many arguments, only need device and 2 GPT images\n", argv[0]);
		exit(64);
	}

	if(!(devpri=readgpt(dev, GPT_PRIMARY))) {
		printf("failed to load device primary GPT\n");
	}
	if(!(devsec=readgpt(dev, GPT_BACKUP))) {
		printf("failed to load device backup GPT\n");
	}
	if(!devpri||!devsec) exit(128);

	if(!(newpri=readgpt(new0, GPT_ANY))) {
		printf("failed to open new primary GPT\n");
	}
	if((new1>0)&&!(newsec=readgpt(new1, GPT_ANY))) {
		printf("failed to open new backup GPT\n");
	}

	/* might suggest overwritten with differing size image, a distinct issue*/
	if(!comparegpt(devpri, devsec)) printf("Device primary and backup GPTs differ!\n");

	if(newsec&&!comparegpt(newpri, newsec)) printf("Primary and backup GPTs to install differ!\n");

	if(comparegpt(devpri, newpri)) printf("Device GPTs and GPTs to install are identical!\n");

	if(testonly) printf("foo\n");

/* UEFI specification, write backupGPT first, primaryGPT second */
	return 0;
}

