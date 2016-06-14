TEMPLATEROOT = ../

#  compiler flags

CFLAGS = -g -O1 -DUSE_FULL_ASSERT
AFLAGS = -g

# additional source files (none)

CSOURCE =  newlib_stubs.c mmcbb.c
ASMSOURCE =

# additional files to delete at clean (none)

CLEANOTHER=

# hal files to exclude from build

CEXCLUDE = stm32f3xx_hal_i2c.c stm32f3xx_hal_i2c_ex.c
CEXCLUDE += stm32f3xx_hal_flash.c stm32f3xx_hal_flash_ex.c
CEXCLUDE += stm32f3xx_hal_pwr.c stm32f3xx_hal_pwr_ex.c
CEXCLUDE += stm32f3xx_hal_tim.c stm32f3xx_hal_tim_ex.c

# load template make file

-include $(TEMPLATEROOT)/gpdsc.mk

