# LGE Tools

This is a set of tools for causing certain devices produced by LG
Electronics to do certain things not approved by the manufacturer.

## The Tools

### chkdzdev (scripts/chkdzdev)

This is a tool for modifying how a KDZ file is marked.  Instead of saying
it is for device X, now it is marked as being for device Y.  The new
device setting is done in one of two ways.  You can use the "-s" option
to explicitly set the new device name (things like "H900", "H901",
"H960", "H960A", "H961N"  or "H962").  You can use the "-c" option to
copy the device name setting from an existing KDZ file which is known to
be appropriate for your device.

The typical usage would be to download a KDZ file for the H901 (T-Mobile
V10), then run `chkdzdev -c <KDZ file for your device> <H901 KDZ file>`.
At which point LGUP should be willing to install that KDZ file on your
device.

### mergegpt (libs/<architecture>/mergegpt)

This is a tool for merging a GPT ("partition table") into your device's
existing GPT.  This is written to keep many of the data elements the same
as the existing table, but merge in portions from the sample file.  Of
note it keeps the type and id values from the existing table, but will
move the extents to match the table being merged in.  This is valuable if
your device has a /cust and you want to recreate that area (notably this
may be valuable for dual-SIM devices).

A typical invocation might be `mergegpt /dev/block/mmcblk0
PrimaryGPT.image`.  There are 3 merging strategies.  The default is
a simple merge (-m) that adds new slices^Wpartitions after existing
entries in the GPT.  I suspect this is safest.  Alternate strategy one is
a hybrid merge (-M) where the ordering matches the incoming GPT, but
types and ids from the existing GPT are preserved.  I consider this
strategy slightly riskier, but pretty safe.  The last strategy is a
replacement strategy (-r), this pretty much swaps in the new GPT with
**minimal** safety checking.  **Be careful** if using this mode.

## The Stategy

The system images for most variants of the LG V10 are pretty similar.
This is to be expected since redesigning and manufacturing new silicon
for each variant of a device would be prohibitively expensive.  Since the
main processor is identical in all of them, the bootloader is compatible
from device to device.

This approach is only likely to work for GSM-only versions of the V10.
The CDMA versions have the data in the on-board flash laid out
differently enough that I think this strategy is too risky to use.
Notably the CDMA devices have extra "eri" and "operatorlogging" slices
(which I suspect are _required_ for CDMA operation) and their
"raw_resources" slices are on a different range of blocks.

### Step 1, data to save from device before proceding

Several portions of the existing flash need to be saved before proceding.
The conventional approach is to use `dd`: `dd if=/dev/block/mmcblk0p##
of=<someplace safe>`.  The crucial to save areas are
/dev/mmcblk0p1 "modem", /dev/mmcblk0p26 "persist", /dev/mmcblk0p34 "sec",
/dev/mmcblk0p39 "laf", /dev/mmcblk0p40 "boot", /dev/mmcblk0p41 "recovery",
/dev/mmcblk0p51 "system", and optionally /dev/mmcblk0p52 "cust".  For
devices with a _slightly_ differing layout from the H901 (such as having
/cust) you will also need to save a portion of /dev/mmcblk0
`dd if=/dev/mmcblk0 count=32 of=<GPT image>`.  While you're doing this
you may also want to save an image of the first 4-8GB of /dev/mmcblk0
onto an SD card as a backup as per [this posting]
(http://forum.xda-developers.com/tmobile-lg-v10/development/debrick-phone-h901-qualcomm-9008-t3440359).

### Step 2, install a H901 KDZ file

**WARNING this is _risky_.  While I am quite hopeful this will work on
most/all versions of the V10, others may not be so lucky.  You stand a
rather good change of "bricking" your device!**

This will need to be one for which a rooting method is known.  The
locations of KDZ files appropriate for many devices are [documented here]
(http://forum.xda-developers.com/lg-v10/general/kdz-download-lollipop-kdz-10c-lg-servers-t3332348).
The current stock rooting method is [documented here]
(http://forum.xda-developers.com/tmobile-lg-v10/development/rom-v10h901v20estock-customized-t3360240).
Notably versions 20e, 20l, and 20j have been rooted, later versions may well
also get the root treatment.  Then run `chkdzdev -s <your device>
<H901 KDZ>` (or use -c and a known good KDZ for your device).  Finally
use LGUP to install this KDZ file.

### Step 3, root image and unlock bootloader

Pretty often these two steps are combined.  The above links give good
information on the process, beware it is _risky_.

### Step 4, merge/replace GPT with original

This step is **optional**!  If your device has a /cust slice which you
wish to restore you need to follow this step, otherwise you can skip.
I suspect this is most useful for dual-SIM devices and others won't need
this.  Use `adb` to copy the image of the old GPT and `mergegpt` onto the
device.  Then run `mergegpt -M /dev/block/mmcblk0 <GPT image>`.

### Step 5, restore old image

_Something more needs to be written here.  I particularly worry about the
process of restoring "system" since that slice is larger than the memory
on the device.  I'm getting tired of writing too._

You simply need to restore the images you generated in step 1.  "boot",
"cust", and "modem" are the most likely candidates for restoration.  The
reason being these are where I believe most of the dual-SIM functionality
lives.  You may wish to restore the others as well.

### Step 6, there is no step 6

There is no step 6, we've gotten everything done for unlocking, nothing
is left.  Perhaps you want to start a Cyanogenmod repository for your
device.

## Building

Some of the tools need to have executables built from source before use.
For these you need to have the Android NDK installed.  Currently NDK
version 12b is able to build armeabi binaries (most likely target), but
unable to build aarch64 binaries (overkill, but _might_ be better).  I
doubt subsequent versions will have any problems and I imagine earlier
versions will also work.  Simply run `ndk-build` and the executables
should successfully build without warnings.  This will also build some
extra tools meant for testing functionality.

