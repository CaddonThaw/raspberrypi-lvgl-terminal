CAMERA_NAME ?= devices/camera

override CXXFLAGS := -I$(LVGL_DIR) $(CXXFLAGS)

CXXSRCS += $(wildcard $(LVGL_DIR)/$(CAMERA_NAME)/*.cpp)