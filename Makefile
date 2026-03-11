#
# Makefile
#
CC ?= gcc 
CXX ?= g++
LVGL_DIR_NAME ?= lvgl
LVGL_DIR ?= ${shell pwd}
CFLAGS ?= -O3 -g0 -I$(LVGL_DIR)/ -Wall -Wshadow -Wundef -Wmissing-prototypes -Wno-discarded-qualifiers -Wall -Wextra -Wno-unused-function -Wno-error=strict-prototypes -Wpointer-arith -fno-strict-aliasing -Wno-error=cpp -Wuninitialized -Wmaybe-uninitialized -Wno-unused-parameter -Wno-missing-field-initializers -Wtype-limits -Wsizeof-pointer-memaccess -Wno-format-nonliteral -Wno-cast-qual -Wunreachable-code -Wno-switch-default -Wreturn-type -Wmultichar -Wformat-security -Wno-ignored-qualifiers -Wno-error=pedantic -Wno-sign-compare -Wno-error=missing-prototypes -Wdouble-promotion -Wclobbered -Wdeprecated -Wempty-body -Wtype-limits -Wshift-negative-value -Wstack-usage=2048 -Wno-unused-value -Wno-unused-parameter -Wno-missing-field-initializers -Wuninitialized -Wmaybe-uninitialized -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -Wtype-limits -Wsizeof-pointer-memaccess -Wno-format-nonliteral -Wpointer-arith -Wno-cast-qual -Wmissing-prototypes -Wunreachable-code -Wno-switch-default -Wreturn-type -Wmultichar -Wno-discarded-qualifiers -Wformat-security -Wno-ignored-qualifiers -Wno-sign-compare

CXXFLAGS ?= -O3 -g0 -I$(LVGL_DIR)/ $(shell pkg-config --cflags opencv4) -Wall -Wshadow -Wextra \
           -Wpointer-arith -fno-strict-aliasing -Wuninitialized \
           -Wmaybe-uninitialized -Wno-unused-parameter -Wno-missing-field-initializers

LDFLAGS ?= -lm -lwiringPi -lpthread $(shell pkg-config --libs opencv4) -liw

BIN = demo

#Collect the files to compile
MAINSRC = ./main.cpp

include $(LVGL_DIR)/lvgl/lvgl.mk
include $(LVGL_DIR)/lv_drivers/lv_drivers.mk
include $(LVGL_DIR)/ui/ui.mk
include $(LVGL_DIR)/devices/threads/threads.mk
include $(LVGL_DIR)/devices/opencv/cv.mk
include $(LVGL_DIR)/devices/wifi/wifi.mk
include $(LVGL_DIR)/devices/tm7711/tm7711.mk
include $(LVGL_DIR)/devices/power/power.mk
include $(LVGL_DIR)/devices/date/date.mk

#CSRCS +=$(LVGL_DIR)/mouse_cursor_icon.c 

AOBJS = $(patsubst %.S,build/%.o,$(ASRCS))
COBJS = $(patsubst %.c,build/%.o,$(CSRCS))
CXXOBJS = $(patsubst %.cpp,build/%.o,$(CXXSRCS))
MAINOBJ = $(patsubst %.cpp,build/%.o,$(MAINSRC))

SRCS = $(ASRCS) $(CSRCS) $(MAINSRC)
OBJS = $(AOBJS) $(COBJS) $(CXXOBJS) $(MAINOBJ)

all: default

build/%.o: %.c
	@mkdir -p $(@D)
	@$(CC)  $(CFLAGS) -c $< -o $@
	@echo "CC $<"

build/%.o: %.cpp
	@mkdir -p $(@D)
	@$(CXX)  $(CXXFLAGS) -c $< -o $@
	@echo "CXX $<"

default: $(AOBJS) $(COBJS) $(CXXOBJS) $(MAINOBJ)
	$(CXX) -o $(BIN) $(MAINOBJ) $(AOBJS) $(COBJS) $(CXXOBJS) $(LDFLAGS)

clean: 
	rm -f $(BIN)
	rm -rf build
