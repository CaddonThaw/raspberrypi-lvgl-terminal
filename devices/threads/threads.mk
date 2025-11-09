THR_NAME ?= devices/threads

override CXXFLAGS := -I$(LVGL_DIR) $(CXXFLAGS)

CXXSRCS += $(wildcard $(LVGL_DIR)/$(THR_NAME)/*.cpp)