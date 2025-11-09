POWER_NAME ?= devices/power

override CXXFLAGS := -I$(LVGL_DIR) $(CXXFLAGS)

CXXSRCS += $(wildcard $(LVGL_DIR)/$(POWER_NAME)/*.cpp)
