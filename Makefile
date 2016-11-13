#########################################################################
# Copyright (C) 2016 Elliott Mitchell <ehem+android@m5p.com>		#
#									#
#	This program is free software: you can redistribute it and/or	#
#	modify it under the terms of the GNU General Public License as	#
#	published by the Free Software Foundation, either version 3 of	#
#	the License, or (at your option) any later version.		#
#									#
#	This program is distributed in the hope that it will be useful,	#
#	but WITHOUT ANY WARRANTY; without even the implied warranty of	#
#	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the	#
#	GNU General Public License for more details.			#
#									#
#	You should have received a copy of the GNU General Public	#
#	License along with this program.  If not, see			#
#	<http://www.gnu.org/licenses/>.					#
#########################################################################


CC = gcc
CFLAGS = -Wall -g -c

LD = $(CC)

SRCS = gpt.c mergegpt.c

LIBS = z uuid

OBJS = $(SRCS:.c=.o)

#ANDROID_NDK ?= <some directory>

-include local.mk

all: gpt mergegpt

# $(ANDROID_NDK)/ndk-build

mergegpt: $(OBJS:%=obj/%)
	$(LD) $(LIBS:%=-l%) $^ -o bin/$@

gpt: src/gpt.c src/gpt.h
	mkdir -p bin
	gcc -Wall -g -DGPT_MAIN src/gpt.c $(LIBS:%=-l%) -o bin/gpt

obj/%.o: src/%.c
	$(CC) $(CFLAGS) $< -o $@

