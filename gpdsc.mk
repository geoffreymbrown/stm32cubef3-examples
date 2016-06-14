V = 0

# name of executable

ELF=$(notdir $(CURDIR)).elf                    
BIN=$(notdir $(CURDIR)).bin
MKFRAG=$(notdir $(CURDIR)).mk
GPDSC=$(notdir $(CURDIR)).gpdsc

# Tool path

TOOLROOT=/l/arm2/devtools/bin/

# Tools

GPDSC2MK=python $(TEMPLATEROOT)/gpdsc2make.py

CC_0 = @echo -n "Compiling $< "; $(CC_1)
CC_1 = $(TOOLROOT)/arm-none-eabi-gcc
CC = $(CC_$(V))

GAS_0 = @echo -n "Assembling $< "; $(GAS_1)
GAS_1 = $(TOOLROOT)/arm-none-eabi-gcc
GAS = $(GAS_$(V))


DD_0 = @echo " "; $(DD_1)
DD_1 = $(TOOLROOT)/arm-none-eabi-gcc
DD = $(DD_$(V))

LD_0 = @echo "Linking $@"; $(LD_1)
LD_1=$(TOOLROOT)/arm-none-eabi-gcc
LD = $(LD_$(V))

DL_0 = @echo -n "Downloading"; $(DL_1)
DL_1=st-flash 
DL = $(DL_$(V))

OC_0 = @echo "Creating Bin Downloadable File"; $(OC_1)
OC_1=$(TOOLROOT)/arm-none-eabi-objcopy
OC = $(OC_$(V))

GP_0 = @echo " "; $(GP_1)
GP_1=grep
GP = $(GP_$(V))

RM_0 = @echo "Cleaning Directory"; $(RM_1)
RM_1=rm
RM = $(RM_$(V))

AR=$(TOOLROOT)/arm-none-eabi-ar
AS=$(TOOLROOT)/arm-none-eabi-as
OBJCOPY=$(TOOLROOT)/arm-none-eabi-objcopy

# Code Paths

vpath %.c $(TEMPLATEROOT)/Src
CFLAGS += -I$(TEMPLATEROOT)/Inc

#  Processor specific

LDSCRIPT = $(TEMPLATEROOT)/stm32f3xx.ld

# Compilation Flags

CFLAGS += -DSTM32F303xC

# LDLIBS = --specs=nosys.specs -lm 

LDLIBS = -lm 
LDFLAGS+= -T$(LDSCRIPT) -mthumb -mcpu=cortex-m4
CFLAGS+= -mcpu=cortex-m4 -mthumb 
CFLAGS+= -Wno-multichar

# Generate project specific makefile

all:

$(MKFRAG): $(GPDSC)
	$(GPDSC2MK) $< > $@

-include $(MKFRAG)

CSOURCE := $(filter-out $(CEXCLUDE),$(CSOURCE))
OBJS += $(addprefix build/, $(CSOURCE:.c=.o) $(ASMSOURCE:.s=.o))
CFLAGS += $(INC)

# Build executable 

$(ELF) : $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

# compile and generate dependency info

build:
	@mkdir -p $@

build/%.o: %.c | build
	$(CC) -c $(CFLAGS) $< -o $@
	$(DD) -MM $(CFLAGS) $< > build/$*.d

build/%.o: %.s | build
	$(GAS) -c $(CFLAGS) $< -o $@
	$(DD) -MM $(CFLAGS) $< > build/$*.d

%.bin: %.elf
	$(OC) -O binary $< $@

clean:
	$(RM) -rf build  $(ELF) $(CLEANOTHER) $(BIN) $(MKFRAG) st-flash.log

debug: $(ELF)
	arm-none-eabi-gdb $(ELF)

download: $(BIN)
	$(DL) write $(BIN) 0x8000000 > st-flash.log 2>&1
	$(GP) -o "jolly" st-flash.log | sed 's/jolly/Success/'
	$(GP) -o "Couldn" st-flash.log | sed 's/Couldn/Fail/'

all: $(ELF)

# pull in dependencies

-include $(OBJS:.o=.d)





