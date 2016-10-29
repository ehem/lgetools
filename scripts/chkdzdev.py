#!/usr/bin/env python

"""
Copyright (C) 2016 Elliott Mitchell <ehem+android@m5p.com>

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
"""

from __future__ import print_function

import argparse
import getopt

# our tools are in "libexec"
sys.path.append(os.path.join(sys.path[0], "libexec"))

import kdz
import dz


class CHLGEDev:
	pass
#change the device a given KDZ file claims to be for...
#
#H90120j_00_0712.kdz	H90120j_00.dz LG-H901
#LGH901AT-00-V20j-TMO-US-JUL-12-2016-ARB02+0
#
#H961N20g_00_0628.kdz	H961N20g_00.dz LG-H961N
#LGH961NAT-00-V20g-GLOBAL-COM-JUN-28-2016+0
#
#H96220c_00_0701.kdz	H96220c_00.dz LG-H962
#LGH962AT-00-V20c-GLOBAL-COM-JUL-01-2016+0
#
#-c <copy from this file>
#-d <set device>
#-a => modify all fields
#-f => modify factoryversion field?
#-F => do not modify factoryversion field?

#opts, files = getopt.getopt(sys.argv[1:], "hd:c:a", ["help", "devicename=", "copyfrom=", "allfields"])

