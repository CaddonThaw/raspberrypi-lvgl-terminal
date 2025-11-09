LV_UI_NAME ?= ui

override CFLAGS := -I$(LVGL_DIR) $(CFLAGS)
override CXXFLAGS := -I$(LVGL_DIR) $(CXXFLAGS)

CSRCS += $(wildcard $(LVGL_DIR)/$(LV_UI_NAME)/*.c)
CSRCS += $(wildcard $(LVGL_DIR)/$(LV_UI_NAME)/src/*.c)
CXXSRCS += $(wildcard $(LVGL_DIR)/$(LV_UI_NAME)/src/*.cpp)
# CSRCS += $(wildcard $(LVGL_DIR)/$(LV_UI_NAME)/src/components/*.c)
# CSRCS += $(wildcard $(LVGL_DIR)/$(LV_UI_NAME)/src/fonts/*.c)
# CSRCS += $(wildcard $(LVGL_DIR)/$(LV_UI_NAME)/src/images/*.c)
# CSRCS += $(wildcard $(LVGL_DIR)/$(LV_UI_NAME)/src/screens/*.c)