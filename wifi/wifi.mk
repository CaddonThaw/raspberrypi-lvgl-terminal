WIFI_NAME ?= wifi

override CXXFLAGS := -I$(LVGL_DIR) $(CXXFLAGS)

CXXSRCS += $(wildcard $(LVGL_DIR)/$(WIFI_NAME)/*.cpp)