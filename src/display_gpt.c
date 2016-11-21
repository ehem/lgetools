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

#include <iconv.h>
#include <stdbool.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "gpt.h"


int main(int argc, char **argv)
{
	struct gpt_data *data, *alt;
	int f;
	char buf0[37], buf1[37];
	int i;

	if(argc!=2) {
		fprintf(stderr, "%s <GPT file>\n", argv[0]);
		exit(128);
	}
	if((f=open(argv[1], O_RDONLY))<0) {
		fprintf(stderr, "%s: open() failed: %s\n", argv[0], strerror(errno));
		exit(4);
	}
	if(!(data=readgpt(f, GPT_ANY))) {
		printf("No GPT found in \"%s\"\n", argv[1]);
		return 1;
	}

	if((data->head.myLBA==1)&&(alt=readgpt(f, GPT_BACKUP))) {
		if(comparegpt(data, alt)) printf("Found identical primary and backup GPTs\n");
		else printf("Primary and backup GPTs differ!\n");
		free(alt);
	} else printf("Second GPT is absent!\n");

	printf("Found v%hu.%hu %s GPT in \"%s\" (%zd sector size)\n",
data->head.major, data->head.minor,
data->head.myLBA==1?"primary":"backup", argv[1], data->blocksz);
	uuid_unparse(data->head.diskUuid, buf0);
	printf("device=%s\nmyLBA=%llu altLBA=%llu dataStart=%llu "
"dataEnd=%llu\n\n", buf0, (unsigned long long)data->head.myLBA,
(unsigned long long)data->head.altLBA,
(unsigned long long)data->head.dataStartLBA,
(unsigned long long)data->head.dataEndLBA);

	for(i=0; i<data->head.entryCount; ++i) {
		if(uuid_is_null(data->entry[i].id)) {
			printf("Name: <empty entry>\n");
		} else {
			uuid_unparse(data->entry[i].type, buf0);
			uuid_unparse(data->entry[i].id, buf1);
			printf("Name: \"%s\" start=%llu end=%llu count=%llu\n"
"typ=%s id=%s\n", data->entry[i].name,
(unsigned long long)data->entry[i].startLBA,
(unsigned long long)data->entry[i].endLBA,
(unsigned long long)(data->entry[i].endLBA-data->entry[i].startLBA), buf0,
buf1);
		}
	}

	free(data);
	return 0;
}

