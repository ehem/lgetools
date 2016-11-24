# LGE Tools

This is a set of tools for causing certain devices produced by LG
Electronics to do certain things not approved by the manufacturer.

## The Tools

### chkdzdev

This is a tool for modifying how a KDZ file is marked.  Instead of saying
it is for device X, now it is marked as being for device Y.  The new
device setting is done in one of two ways.  You can use the "-s" option
to explicitly set the new device name (things like "H960", "H960A",
"H961N", "H962", "RS987" or "VS990").  You can use the "-c" option to
copy the device name setting from another KDZ file.

The typical usage would be to download a KDZ file for the H901 (T-Mobile
V10), then run `chkdzdev -c <KDZ file for your device> <H901 KDZ file>`.
At which point LGUP should be willing to install that KDZ file on your
device.

### mergegpt

This is a tool for merging a GPT ("partition table") into your device's
existing GPT.  This is written to keep many of the data elements the same
as the existing table, but merge in portions from the sample file.  Of
note it keeps the type and id values from the existing table, but will
move the extents to match the table being merged in.  This is valuable if
your device has a /cust and you want to recreate that area (notably this
may be valuable for dual-SIM devices).

A typical invocation might be `mergegpt /dev/block/mmcblk0
H962.PrimaryGPT.image`.  There are 3 merging strategies.  The default is
a simple merge (-m) that adds new slices^Wpartitions after existing
entries in the GPT.  I suspect this is safest.  Alternate strategy one is
a hybrid merge (-M) where the ordering matches the incoming GPT, but
types and ids from the existing GPT are preserved.  I consider this
strategy slightly riskier, but pretty safe.  The last strategy is a
replacement strategy (-r), this pretty much swaps in the new GPT with
**minimal** safety checking.  **Be careful** if using this mode.

## The Stategy

Something here about the approach these are designed for.  Some overt
instructions.

