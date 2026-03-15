#ifndef CAMERA_H
#define CAMERA_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

const char *camera_get_scripts_path(void);
int camera_scan_scripts(char *options, size_t buffer_size);
void camera_set_selected_script(const char *script_name);
const char *camera_get_selected_script(void);
void camera_clear_selected_script(void);
bool camera_start_script_process(const char *script_name, char *error, size_t error_size);
void camera_poll_script_process(void);
void camera_stop_script_process(void);
bool camera_is_script_running(void);


#endif 