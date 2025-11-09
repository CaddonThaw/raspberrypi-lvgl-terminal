CV_NAME ?= devices/opencv

override CXXFLAGS := -I$(LVGL_DIR) $(CXXFLAGS)

CXXSRCS += $(wildcard $(LVGL_DIR)/$(CV_NAME)/*.cpp)
