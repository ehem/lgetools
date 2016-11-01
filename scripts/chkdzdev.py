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

import struct
import argparse
import io
import sys


KDZEntry = struct.Struct("<256sQQ")
KDZMagic = b"\x28\x05\x00\x00"b"\x24\x38\x22\x25"

DZHead = struct.Struct("<4sIII32s144s")
DZMagic = b"\x32\x96\x18\x74"

verbose = 0


def parseArgs():
	# Parse and return arguments
	parser = argparse.ArgumentParser(description="LG KDZ device value changer $Id$")
	group = parser.add_mutually_exclusive_group(required=True)
	group.add_argument("-s", "--set-device", help='Device name to set (often "H###")', action="store", dest="device")
	group.add_argument("-c", "--copyfrom", help="KDZ file to copy device name from", action="store", dest="deviceFile")
	group = parser.add_mutually_exclusive_group()
	group.add_argument("-f", "--set-factoryversion", help="Modify DZ factoryversion field (default)", action="store_const", const=True, dest="setFactory", default=True)
	group.add_argument("-F", "--ignore-factoryversion", help="Do not modify DZ factoryversion field", action="store_const", const=False, dest="setFactory", default=True)
	parser.add_argument("-v", "--verbose", action="count", help="Increase verbosity", dest="verbose", default=0)
	parser.add_argument("files", action="store", help="KDZ file(s) to modify", nargs='+')
	return parser.parse_args()


def getDevice(name):
	# Very simple partial KDZ/DZ parser, this is minimally capable and
	# assumes the DZ portion will be first (error otherwise)
	try:
		file = io.open(name, "rb")
	except IOError as err:
		print('Failed opening "{:s}": {:s}'.format(name, err[1]))
		sys.exit(1)
	try:
		buf = file.read(len(KDZMagic))
		if buf != KDZMagic:
			raise IOError()

		buf = file.read(KDZEntry.size)
		if len(buf) != KDZEntry.size:
			raise IOError()

	except IOError as err:
		print('File "{:s}" doesn\'t appear to be a KDZ file.'.format(name), file=sys.stderr)
		sys.exit(1)


	try:
		entry = KDZEntry.unpack(buf)
		ename = entry[0].rstrip(b'\x00')

		if not ename.endswith(b".dz"):
			raise IOError(None, "Failed to find DZ entry at start of KDZ file.")

		if file.seek(entry[2], io.SEEK_SET) != entry[2]:
			raise IOError(None, 'Failed while attempting to read inner DZ file.')

		buf = file.read(DZHead.size)
		if len(buf) != DZHead.size:
			raise IOError(None, 'Failed while attempting to read inner DZ file.')

	except IOError as err:
		print(err[1], file=sys.stderr)
		sys.exit(1)


	entry = DZHead.unpack(buf)
	if entry[0] != DZMagic:
		print("Bad magic number on inner DZ file.", file=sys.stderr)
		sys.exit(1)

	if entry[1] != 2:
		print("Cannot handle DZ files besides version 2, inner DZ file is version {:d}".format(entry[1]), file=sys.stderr)
		sys.exit(1)

	if entry[2] != 1:
		print("Warning: Tool is writen for DZ file format v2.1, inner DZ file is v2.{:d}".format(entry[2]), file=sys.stderr)

	device = entry[4].rstrip(b'\x00')

	if not device.startswith(b"LG-"):
		print("Error: Device name inside inner DZ file doesn't begin with \"LG-\", failed.", file=sys.stderr)
		sys.exit(1)

	# Last bit of decoding needed
	device = device[3:].decode("utf8")

	if verbose >= 1:
		print('File "{:s}": found device name "{:s}"'.format(name, device))

	return device


def setDevice(name, device, setFactory):
	# This is a very simple KDZ/DZ parser and modifier, we assume the inner
	# DZ file will be first (it has in all real world cases observed so
	# far).

	# Turn it into a "binary" string
	device = device.encode("utf8")

	try:
		file = io.open(name, "r+b")
	except IOError as err:
		print('Failed opening "{:s}": {:s}'.format(name, err[1]))
		return 1
	try:
		buf = file.read(len(KDZMagic))
		if buf != KDZMagic:
			raise IOError()

		kdz_offset = file.seek(0, io.SEEK_CUR)
		buf = file.read(KDZEntry.size)
		if len(buf) != KDZEntry.size:
			raise IOError()

	except IOError as err:
		print('File "{:s}" doesn\'t appear to be a KDZ file.'.format(name), file=sys.stderr)
		return 1


	try:
		kdz_entry = KDZEntry.unpack(buf)
		ename = kdz_entry[0].rstrip(b'\x00')

		if not ename.endswith(b".dz"):
			raise IOError(None, "Failed to find DZ entry at start of KDZ file.")

		dz_offset = kdz_entry[2]

		if file.seek(kdz_entry[2], io.SEEK_SET) != dz_offset:
			raise IOError(None, 'Failed while attempting to read inner DZ file.')

		buf = file.read(DZHead.size)
		if len(buf) != DZHead.size:
			raise IOError(None, 'Failed while attempting to read inner DZ file.')

	except IOError as err:
		print(err[1], file=sys.stderr)
		return 1


	dz_entry = DZHead.unpack(buf)
	if dz_entry[0] != DZMagic:
		print("Bad magic number on inner DZ file.", file=sys.stderr)
		return 1

	if dz_entry[1] != 2:
		print("Cannot handle DZ files besides version 2, inner DZ file is version {:d}".format(dz_entry[1]), file=sys.stderr)
		return 1

	if dz_entry[2] != 1:
		print("Warning: Tool is writen for DZ file format v2.1, inner DZ file is v2.{:d}".format(dz_entry[2]), file=sys.stderr)

	old_device = dz_entry[4].rstrip(b'\x00')

	if not old_device.startswith(b"LG-"):
		print("Error: Device name inside \"{:s}\" inner DZ file doesn't begin with \"LG-\", failed.".format(name), file=sys.stderr)
		return 1

	old_device = old_device[3:]

	nename = device + ename[len(old_device):]
	if verbose >= 1:
		print('File "{:s}": old device is "{:s}"'.format(name, old_device.decode("utf")))
		print('File "{:s}": old kdz name="{:s}", new kdz name="{:s}"'.format(name, ename, nename))

	nename = nename.ljust(256, b'\x00')

	file.seek(kdz_offset, io.SEEK_SET)
	file.write(nename)

	entry = [x for x in dz_entry]

	entry[4] = ("LG-" + device).ljust(32, b'\x00')

	old_factory = entry[5].rstrip(b'\x00')
	new_factory = "LG" + device + old_factory[len(old_device)+2:]

	if setFactory:
		print('File "{:s}": old factory version="{:s}", new factory version="{:s}"'.format(name, old_factory.decode("utf8"), new_factory.decode("utf8")))
		entry[5] = new_factory.ljust(144, b'\x00')

	file.seek(dz_offset, io.SEEK_SET)
	file.write(DZHead.pack(*entry))

	return 0



if __name__ == "__main__":
	args = parseArgs()

	if args.deviceFile:
		pass

	if args.deviceFile:
		args.device=getDevice(args.deviceFile)

	print('Device name to use: "{:s}"'.format(args.device))

	verbose = args.verbose

	err = 0

	for f in args.files:
		print('Modify file "{:s}"'.format(f))
		e = setDevice(f, args.device, args.setFactory)
		if e > err:
			err = e

	sys.exit(err)

