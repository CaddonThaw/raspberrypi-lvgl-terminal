CV_NAME ?= opencv

override CXXFLAGS := -I$(LVGL_DIR) $(CXXFLAGS)

CXXSRCS += $(wildcard $(LVGL_DIR)/$(CV_NAME)/*.cpp)
