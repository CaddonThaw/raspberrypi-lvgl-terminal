LV_UI_NAME ?= ui

override CFLAGS := -I$(LVGL_DIR) $(CFLAGS)
override CXXFLAGS := -I$(LVGL_DIR) $(CXXFLAGS)

CSRCS += $(wildcard $(LVGL_DIR)/$(LV_UI_NAME)/*.c)
CSRCS += $(wildcard $(LVGL_DIR)/$(LV_UI_NAME)/fonts/*.c)
CXXSRCS += $(wildcard $(LVGL_DIR)/$(LV_UI_NAME)/*.cpp)
