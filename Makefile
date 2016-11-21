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


TARGETS := gpt mergegpt

CC ?= gcc
CFLAGS := -Wall -g -MMD -c

LD ?= $(CC)

mergegpt_SRCS := gpt.c mergegpt.c forcecrc32.c

gpt_SRCS := gpt.c display_gpt.c

LIBS := z uuid

DIRS := bin obj

$(foreach targ,$(TARGETS),$(eval $(targ)_OBJS := $$($(targ)_SRCS:.c=.o)))

#ANDROID_NDK ?= <some directory>

-include local.mk

all: $(TARGETS:%=bin/%)

# $(ANDROID_NDK)/ndk-build

$(foreach targ,$(TARGETS),$(eval bin/$(targ): $$($(targ)_OBJS:%=obj/%)))

$(TARGETS:%=bin/%): %:
	$(LD) $(LIBS:%=-l%) $^ -o $@

obj/%.o: src/%.c $(DIRS:%=%/.timestamp)
	$(CC) $(CFLAGS) $< -o $@

$(DIRS:%=%/.timestamp):
	mkdir -p $(dir $@)
	touch $@

-include obj/*.d

