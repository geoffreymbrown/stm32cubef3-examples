This is an example of a simple USB mounted ramdisk.  The files comprising the
disk are drawn from the directory testfiles/.   The disk image (test.img) is
built using mtools.  This is then translated into a C file by the host tool
mkimage (sources includeded).

In order to enable disk ejection, it was necessary to modify one of the
library files usbd_msc_scsi.c.  The modified file is in ../Src to ensure
that regeneration of the project through stm32cubemx does not overwrite the
minor, but key, change.