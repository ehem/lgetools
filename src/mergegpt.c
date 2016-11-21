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
#include <string.h>
#include <termios.h>
#include <zlib.h>

#include "gpt.h"
#include "forcecrc32.h"


int verbose=0;


static bool mergegpt(int, struct gpt_data **, const struct gpt_data *);
static bool hybridgpt(int, struct gpt_data **, const struct gpt_data *);
static bool replacegpt(int, struct gpt_data **, const struct gpt_data *);

static bool checkmove(const struct gpt_entry *, const struct gpt_entry *);
static bool checkremove(const struct gpt_entry *ent);


int main(int argc, char **argv)
{
	enum {DEFAULT, REPLACE, MERGE, HYBRID} action=DEFAULT;
	bool testonly=false;
	int opt;
	struct gpt_data *devpri=NULL, *devsec=NULL, *newpri=NULL, *newsec=NULL;
	struct gpt_data *devnew;
	struct _gpt_data *new;
	int dev, new0, new1=-1;
	bool (*mergefunc)(int, struct gpt_data **, const struct gpt_data *);
	uint32_t crc;
	int i;

	while((opt=getopt(argc, argv, "hrmMtvq"))!=-1) {
		switch(opt) {
		case 'r':
			if(action!=DEFAULT) {
				fprintf(stderr, "Only one of -m, -M, or -r can be used!\n");
				exit(128);
			}
			action=REPLACE;
			break;
		case 'm':
			if(action!=DEFAULT) {
				fprintf(stderr, "Only one of -m, -M, or -r can be used!\n");
				exit(128);
			}
			action=MERGE;
			break;
		case 'M':
			if(action!=DEFAULT) {
				fprintf(stderr, "Only one of -m, -M, or -r can be used!\n");
				exit(128);
			}
			action=HYBRID;
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
		case 'h':
			fprintf(stderr,
"Usage: %s [-m | -M | -r] [-t] <device> <replacement-GPT.bin> [<backup-GPT.bin>]\n"
"\n"
"optional arguments:\n"
"  -h                    show this help message and exit\n"
"  -t                    test mode, do not write to media\n"
"  -m                    merge GPT copies in\n"
"  -M                    intermediate merge, incoming has more effect\n"
"  -r                    replace GPT on device with GPT images from .bin files"
"\n" "\n"
"Merge mode (-m) is the most conservative and should be safest.\n"
"Intermediate merge mode (-M) is slightly less conservative, but should\n"
"still be pretty safe.  Replacement mode (-r) is most risky and may or may\n"
"not work depending upon how sensitive the firmware is.\n", argv[0]);
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

	if(newsec) {
		if(!comparegpt(newpri, newsec)) {
			printf("Error: Primary and backup GPTs to install differ!\n");
			exit(1);
		}
		if(newpri->head.myLBA==newsec->head.myLBA) {
			fprintf(stderr, "Error: Both primary and backup GPT files appear to be from same media region!\n");
			exit(1);
		}
	}

	/* might suggest overwritten with differing size image, be wary */
	if(!comparegpt(devpri, devsec)) {
		const char *const fmt="Device primary and backup GPTs differ!\n%s";
		if(comparegpt(devsec, newpri)) {
			devnew=devpri;
			printf(fmt, "You appear to be reinstalling a copy of the backup.\n");
		} else if(comparegpt(devpri, newpri)) {
			devnew=devsec;
			printf(fmt, "You appear to be reinstalling a copy of the primary.\n");
		} else {
			char buf[512];
			printf(fmt, "Danger!  GPT to install does not match either device GPT!\n\nAre you SURE you wish to continue? (must type \"yes\")\n");
			fgets(buf, sizeof(buf), stdin);
			if(strcasecmp(buf, "yes\n")) {
				printf("Not confirmed, terminating\n");
				exit(64);
			}
		}

	/* I don't know why you'd want to do this, but doesn't seem harmful...*/
	} else if(comparegpt(devpri, newpri)) {
		printf("Device GPTs and GPTs to install are identical!\n");
		devnew=devpri;
	} else devnew=devpri;

	/* writegpt() will fail in this case, but not give a message */
	if(lseek(dev, -(newpri->head.myLBA+newpri->head.altLBA)*devpri->blocksz, SEEK_END)!=0) {
		fprintf(stderr, "Primary and/or backup GPT LBAs do not match size of media, cannot continue.\n");
		exit(128);
	}

	if(testonly) {
		close(dev);
		dev=-1;
	}

	switch(action) {
	case DEFAULT:
	case MERGE:
		mergefunc=mergegpt;
	break;
	case REPLACE:
		mergefunc=replacegpt;
	break;
	case HYBRID:
		mergefunc=hybridgpt;
	break;
	default:
		fprintf(stderr, "%s: Internal error!", argv[0]);
		exit(-1);
	}

	if(!mergefunc(dev, &devnew, newpri)) exit(2);


	new=(struct _gpt_data *)devnew;
	gpt_entries2raw(new, devnew);

	crc=crc32(0, (Byte *)new->entry, new->head.entrySize*new->head.entryCount);
	crc^=new->head.entryCRC32;
	for(i=new->head.entryCount-1; i>=0; --i) {
		if(!memcmp("\0\0\0\0\0\0", (char *)new->entry[i].name+sizeof(new->entry[i].name)-6, 6)) {
			crc=reverse_crc32(crc,
(char *)(new->entry+new->head.entryCount)-((char *)new->entry[i].name+sizeof(new->entry[i].name)-4));
			memcpy((char *)new->entry[i].name+sizeof(new->entry[i].name)-4, &crc, 4);
			break;
		}
	}

	crc=crc32(0, (Byte *)new->entry, new->head.entrySize*new->head.entryCount);
	if(crc!=new->head.entryCRC32) fprintf(stderr, "Failed to preserve entry CRC32, the GPT modification may be easier to spot!!\n\n");

	new->head.entryCRC32=crc;

	printf("A merged GPT has been successfully created.  This process has "
"a substantial\n" "chance of producing incorrect results, even though care has "
"been taken to make\n" "the process work.\n"
"Are you sure you wish to write the new GPT to the target? (y/N)\n");

	setvbuf(stdin, NULL, _IONBF, 0);
	{
		struct termios termios;
		tcgetattr(fileno(stdin), &termios);
		termios.c_lflag&=~ICANON;
		termios.c_cc[VMIN]=1;
		termios.c_cc[VTIME]=0;
		tcsetattr(fileno(stdin), TCSANOW, &termios);
	}

	opt=getchar();
	if(opt!='y') {
		printf("\nNot confirmed, terminating\n");
		exit(1);
	}
	putchar('\n');

	if(dev<0) {
		printf("...simulate writing to media...\n");
		exit(0);
	}

	if(!_writegpt(dev, new)) {
		fprintf(stderr, "PANIC!!! Failed while writing new GPT\n");
		exit(1);
	}

	return 0;
}


static bool mergegpt(int fd, struct gpt_data **_dev, const struct gpt_data *new)
{
	int i;
	int empty=0;
	struct gpt_data *dev=*_dev;
	char *visited=NULL;
	uint32_t origentries, needentries;

	/* we copy these fields since we could be reinstalling a F600S GPT,
	** which has a slightly different amount of space */
	dev->head.myLBA=new->head.myLBA;
	dev->head.altLBA=new->head.altLBA;
	dev->head.dataStartLBA=new->head.dataStartLBA;
	dev->head.dataEndLBA=new->head.dataEndLBA;
	dev->head.entryStart=new->head.entryStart;

	origentries=dev->head.entryCount;


	/* have to worry if the older one had less space for entries */
	needentries=0;
	for(i=0; i<new->head.entryCount; ++i) if(!uuid_is_null(new->entry[i].type)&&!uuid_is_null(new->entry[i].id)) ++needentries;
	if(origentries<needentries) {
		dev=realloc(dev, sizeof(struct gpt_data)+sizeof(struct gpt_entry)*needentries);
		if(!dev) {
			fprintf(stderr, "Failed allocating memory, sorry\n");
			return false;
		}
		*_dev=dev;

		memset(dev->entry+origentries, 0,
sizeof(struct gpt_entry)*(needentries-origentries));
		dev->head.entryCount=needentries;
	}

	if(!(visited=malloc((new->head.entryCount+7)>>3))) {
		fprintf(stderr, "Failed allocating memory, sorry\n");
		return false;
	}
	memset(visited, 0, (new->head.entryCount+7)>>3);

	for(i=0; i<origentries; ++i) {
		int j;
		const struct gpt_entry *g;

		if(uuid_is_null(dev->entry[i].type)||uuid_is_null(dev->entry[i].id)) continue;

		j=i;
		while(strcmp(dev->entry[i].name, new->entry[j].name)) {
			++j;
			if(j>new->head.entryCount) j=0;

			if(j==i) {
				if(checkremove(dev->entry+i)) break;
				fprintf(stderr, "ERROR: GPT to install lacks slice named \"%s\", cannot continue\n", dev->entry[i].name);
				goto fail;
			}
		}
		g=new->entry+j;

		visited[(g-new->entry)>>3]|=1<<((g-new->entry)&7);

		if(uuid_compare(g->type, dev->entry[i].type)) {
			fprintf(stderr, "ERROR: Type UUID differs for slice named \"%s\", cannot continue\n", dev->entry[i].name);
			goto fail;
		}

		if(g->flags!=dev->entry[i].flags) {
			fprintf(stderr, "ERROR: Flags differ for slice named \"%s\", cannot continue\n", dev->entry[i].name);
			goto fail;
		}

		/* are they sure?  (or is it a known okay?) */
		if(!checkmove(g, dev->entry+i)) return false;

		/* problematic situation, start and/or end differ, be wary */
		dev->entry[i].startLBA=g->startLBA;
		dev->entry[i].endLBA=g->endLBA;
	}

	/* copy in any extra slices needed by replacement image */
	for(i=0; i<new->head.entryCount; ++i) {
		/* original already has such entry */
		if(visited[(unsigned)i>>3]&1<<(i&7)) continue;

		if(uuid_is_null(new->entry[i].type)) continue;

		while(!uuid_is_null(dev->entry[empty].type)&&!uuid_is_null(dev->entry[empty].id))
			if(++empty==dev->head.entryCount) {
				fprintf(stderr, "Ran out of spare entries in GPT, unable to continue\n");
				return false;
			}

		dev->entry[empty]=new->entry[i];
	}

	free(visited);

	return true;

fail:
	if(visited) free(visited);
	return false;
}


static bool hybridgpt(int fd, struct gpt_data **_dev, const struct gpt_data *new)
{
	int i;
	int empty=0;
	struct gpt_data *dev=*_dev;
	uint32_t origentries, needentries;

	/* we copy these fields since we could be reinstalling a F600S GPT,
	** which has a slightly different amount of space */
	dev->head.myLBA=new->head.myLBA;
	dev->head.altLBA=new->head.altLBA;
	dev->head.dataStartLBA=new->head.dataStartLBA;
	dev->head.dataEndLBA=new->head.dataEndLBA;
	dev->head.entryStart=new->head.entryStart;

	origentries=dev->head.entryCount;


	/* have to worry if the older one had less space for entries */
	needentries=0;
	for(i=0; i<new->head.entryCount; ++i) if(!uuid_is_null(new->entry[i].type)&&!uuid_is_null(new->entry[i].id)) ++needentries;
	if(origentries<needentries) {
		dev=realloc(dev, sizeof(struct gpt_data)+sizeof(struct gpt_entry)*needentries);
		if(!dev) {
			fprintf(stderr, "Failed allocating memory, sorry\n");
			return false;
		}
		*_dev=dev;

		memset(dev->entry+origentries, 0,
sizeof(struct gpt_entry)*(needentries-origentries));
		dev->head.entryCount=needentries;
	}


	for(i=new->head.entryCount; i>=0; --i) {
		int j;

		if(uuid_is_null(new->entry[i].type)||uuid_is_null(new->entry[i].id)) continue;

		j=i;
		while(strcmp(new->entry[i].name, dev->entry[j].name)) {
			--j;
			if(j<0) j=origentries;

			if(j==i) { /* absent from original GPT */

				if(!uuid_is_null(dev->entry[i].type)&&!uuid_is_null(dev->entry[i].id)) {
					while(!uuid_is_null(dev->entry[empty].type)&&!uuid_is_null(dev->entry[empty].id))
						if(++empty==dev->head.entryCount) {
							fprintf(stderr, "Ran out of spare entries in GPT, unable to continue\n");
							return false;
						}
					dev->entry[empty]=dev->entry[i];
				}

				dev->entry[i]=new->entry[i];
				break;
			}
		}

		if(j!=i) {
			struct gpt_entry tmp=dev->entry[j];
			dev->entry[i]=dev->entry[j];
			dev->entry[j]=tmp;
		}

		if(uuid_compare(new->entry[i].type, dev->entry[i].type)) {
			fprintf(stderr, "ERROR: Type UUID differs for slice named \"%s\", cannot continue\n", dev->entry[i].name);
			return false;
                }

		if(new->entry[i].flags!=dev->entry[i].flags) {
			fprintf(stderr, "ERROR: Flags differ for slice named \"%s\", cannot continue\n", dev->entry[i].name);
			return false;
                }

		/* are they sure?  (or is it a known okay?) */
		if(!checkmove(new->entry+i, dev->entry+i)) return false;

		/* problematic situation, start and/or end differ, be wary */
		dev->entry[i].startLBA=new->entry[i].startLBA;
		dev->entry[i].endLBA=new->entry[i].endLBA;
	}


	/* check for slices which would be removed (likely BAD) */
	for(i=0; i<dev->head.entryCount; ++i) {
		int j;

		/* dev is potentially scrambled at this point, so scan whole */

		if(uuid_is_null(dev->entry[i].type)||uuid_is_null(dev->entry[i].id)) continue;

		j=i;
		while(strcmp(dev->entry[i].name, new->entry[j].name)) {
			++j;
			if(j>new->head.entryCount) j=0;

			if(j==i) {
				if(checkremove(dev->entry+i)) break;
				fprintf(stderr, "ERROR: GPT to install lacks slice named \"%s\", cannot continue\n", dev->entry[i].name);
				return false;
                        }
		}
	}

	return true;
}


static bool replacegpt(int fd, struct gpt_data **_dev, const struct gpt_data *new)
{
	int i;
	struct gpt_data *dev=*_dev;

	dev->head.major=new->head.major;
	dev->head.minor=new->head.minor;
	dev->head.headerSize=new->head.headerSize;
	/* headerCRC32 is taken care of by writegpt() */
	dev->head.reserved=new->head.reserved;

	/* we copy these fields since we could be reinstalling a F600S GPT,
	** which has a slightly different amount of space */
	dev->head.myLBA=new->head.myLBA;
	dev->head.altLBA=new->head.altLBA;
	dev->head.dataStartLBA=new->head.dataStartLBA;
	dev->head.dataEndLBA=new->head.dataEndLBA;
	uuid_copy(dev->head.diskUuid, new->head.diskUuid);
	dev->head.entryStart=new->head.entryStart;


	/* have to worry if the older one had less space for entries */
	if(dev->head.entryCount<new->head.entryCount) {
		dev=realloc(dev, sizeof(struct gpt_data)+sizeof(struct gpt_entry)*new->head.entryCount);
		if(!dev) {
			fprintf(stderr, "Failed allocating memory, sorry\n");
			return false;
		}
		*_dev=dev;

		memset(dev->entry+dev->head.entryCount, 0,
sizeof(struct gpt_entry)*(new->head.entryCount-dev->head.entryCount));
	}

	for(i=0; i<dev->head.entryCount; ++i) {
		int j;
		if(uuid_is_null(dev->entry[i].type)) continue;

		j=i;
		while(uuid_compare(dev->entry[i].type, new->entry[j].type)||
strcmp(dev->entry[i].name, new->entry[j].name)) {
			++j;
			if(j>dev->head.entryCount) j=0;

			if(j==i) {
				if(checkremove(dev->entry+i)) break;
				fprintf(stderr, "ERROR: GPT to install lacks slice named \"%s\", cannot continue\n", dev->entry[i].name);
				return false;
                        }
                }

		if(!checkmove(dev->entry+i, new->entry+j)) return false;
	}

	dev->head.entryCount=new->head.entryCount;
	dev->head.entrySize=new->head.entrySize;
	dev->head.entryCRC32=new->head.entryCRC32;

	for(i=0; i<new->head.entryCount; ++i) dev->entry[i]=new->entry[i];

	return true;
}


static bool checkmove(const struct gpt_entry *ent, const struct gpt_entry *oth)
{
	char buf[128];

	if((ent->startLBA==oth->startLBA)&&(ent->endLBA==oth->endLBA)) return true;

	/* these 3 are known to move somewhat and reasonably safe to adjust */
	if(ent->name[0]=='c') {
		if(!strcmp(ent->name, "cache")) return true;
		else if(!strcmp(ent->name, "cust")) return true;
	} else if(!strcmp(ent->name, "userdata")) return true;

	if(!strcmp(ent->name+strlen(ent->name)-3, "bak")) {
		fprintf(stderr, "ERROR: Need to move slice \"%s\", which is likely part of bootloader, fail\n", ent->name);
		return false;
	}

	fprintf(stderr, "DANGER: Need to move slice \"%s\", which is UNSAFE, are you sure?\n\n", ent->name);

	fgets(buf, sizeof(buf), stdin);
	if(strcasecmp(buf, "yes\n")) {
		printf("Not confirmed, terminating\n");
		return false;
	}

	return true;
}

static bool checkremove(const struct gpt_entry *ent)
{
	const char *const okay[]={"cust", "eri", "operatorlogging"};
	unsigned int lo, hi, mid;
	char buf[128];

	if(!strcmp(ent->name+strlen(ent->name)-3, "bak")) {
		fprintf(stderr, "ERROR: Need to remove slice \"%s\", which is likely part of bootloader, fail\n", ent->name);
		return false;
	}

	lo=0; hi=sizeof(okay)/sizeof(okay[0]);
	do {
		int cmp;
		mid=((lo+hi)>>1);
		if(!(cmp=strcmp(ent->name, okay[mid]))) {
			fprintf(stderr, "WARNING: Going to remove slice \"%s\", which is suspected safe, but dangerous!\n\n", ent->name);
			return true;
		} else if(cmp<0) hi=mid;
		else lo=mid+1;
	} while(lo!=hi);

	fprintf(stderr, "DANGER: Need to remove slice \"%s\", which is UNSAFE, are you sure?\n\n", ent->name);

	fgets(buf, sizeof(buf), stdin);
	if(strcasecmp(buf, "yes\n")) {
		printf("Not confirmed, terminating\n");
		return false;
	}

	return true;
}

