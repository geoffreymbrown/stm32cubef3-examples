TEMPLATEROOT = ../

#  compiler flags

CFLAGS = -g -O1 -DUSE_FULL_ASSERT
AFLAGS = -g

# additional source files (none)

CSOURCE =  newlib_stubs.c DiskImg.c
ASMSOURCE =

# additional files to delete at clean (none)

CLEANOTHER= mkdisk Src/DiskImg.c

# hal files to exclude from build

CEXCLUDE = stm32f3xx_hal_i2c.c stm32f3xx_hal_i2c_ex.c
CEXCLUDE += stm32f3xx_hal_flash.c stm32f3xx_hal_flash_ex.c
CEXCLUDE += stm32f3xx_hal_pwr.c stm32f3xx_hal_pwr_ex.c
CEXCLUDE += stm32f3xx_hal_tim.c stm32f3xx_hal_tim_ex.c
CEXCLUDE += stm32f3xx_hal_dma.c


# load template make file

-include $(TEMPLATEROOT)/gpdsc.mk

all: 

mkdisk: mkdisk.c
	gcc -o mkdisk mkdisk.c

test.img : testfiles
	mformat -C -h 1 -n 32 -t 1 -v "RamDisk" -i test.img ::
	mcopy -i test.img testfiles/* ::

build/DiskImg.o : Src/DiskImg.c

Src/DiskImg.c: test.img mkdisk
	./mkdisk test.img > Src/DiskImg.c
	
