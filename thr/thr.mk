THR_NAME ?= thr

override CXXFLAGS := -I$(LVGL_DIR) $(CXXFLAGS)

CXXSRCS += $(wildcard $(LVGL_DIR)/$(THR_NAME)/*.cpp)