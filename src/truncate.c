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
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>


int main(int argc, char **argv)
{
	int opt;
	int fd;
	off64_t sz=0;
	off64_t cur;
	enum {SET, EXTEND, REDUCE, ATMOST, ATLEAST, ROUNDDN, ROUNDUP} mode=SET;

	while((opt=getopt(argc, argv, "cor:s:hv"))!=-1) {
		char *end;
		switch(opt) {
		case 'c':
		case 'o':
		case 'r':
			fprintf(stderr, "Sorry, but the -c, -o, -r, and -v options are unimplemented\n");
			return 1;
			break;
		case 's':
			if(!isdigit(optarg[0])) {
				switch(optarg[0]) {
				case '+':
					mode=EXTEND;
					break;
				case '-':
					mode=REDUCE;
					break;
				case '<':
					mode=ATMOST;
					break;
				case '>':
					mode=ATLEAST;
					break;
				case '/':
					mode=ROUNDDN;
					break;
				case '%':
					mode=ROUNDUP;
					break;
				default:
					fprintf(stderr, "Unknown flag \"%c\" at the start of size string\n", optarg[0]);
					return 1;
				}
				++optarg;
			}

			sz=strtoull(optarg, &end, 0);
			if(*end!=0) {
				int mul;
				switch(*end) {
				case 'K':
					mul=1;
					break;
				case 'M':
					mul=2;
					break;
				case 'G':
					mul=3;
					break;
				case 'T':
					mul=4;
					break;
				case 'P':
					mul=5;
					break;
				case 'E':
					mul=6;
					break;
				case 'Z':
/*					mul=7; */
/*					break; */
				case 'Y':
/*					mul=8; */
					fprintf(stderr, "I'm sorry Dave, but I just can't do that, the size is simply too big");
					return -1;
					break;
				default:
					fprintf(stderr, "Unknown flag \"%c\" at the end of the size string\n", *end);
					return 1;
				}
				if(end[1]) {
					fprintf(stderr, "Garbage after size option arg: \"%s\"\n", end+1);
					return 1;
				}
				mul=(mul<<3)+(mul<<1);
				sz<<=mul;
			}

			break;
		default:
		case 'h':
			fprintf(stderr, "Sorry, but I'm helpless\n");
			return 0;
		}
	}

	while(optind<argc) {
		if((fd=open(argv[optind], O_CREAT|O_WRONLY|O_LARGEFILE|O_NOCTTY, 0666))<0) {
			perror("open");
			return 1;
		}

		cur=lseek64(fd, 0, SEEK_END);
		switch(mode) {
		case SET:
			cur=sz;
			break;
		case EXTEND:
			cur+=sz;
			break;
		case REDUCE:
			if(cur>sz) cur-=sz;
			else cur=0;
			break;
		case ATMOST:
			if(cur>sz) cur=sz;
			break;
		case ATLEAST:
			if(cur<sz) cur=sz;
			break;
		case ROUNDUP:
			cur+=sz-1;
		case ROUNDDN:
			cur/=sz;
			cur*=sz;
			break;
		}
		ftruncate64(fd, cur);
		++optind;
	}

	return 0;
}

