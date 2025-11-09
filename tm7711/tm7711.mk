TM7711_NAME ?= tm7711

override CXXFLAGS := -I$(LVGL_DIR) $(CXXFLAGS)

CXXSRCS += $(wildcard $(LVGL_DIR)/$(TM7711_NAME)/*.cpp)
