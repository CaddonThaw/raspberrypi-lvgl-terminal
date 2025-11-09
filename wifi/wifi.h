#ifndef WIFI_HH
#define WIFI_HH

#include <unistd.h>
#include <stdbool.h> 
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

int wifi_scan(void);
int wifi_IP(void);
int wifi_connect(const char* ssid, const char* pass);
void escape_shell_chars(const char *input, char *output, uint16_t out_size);

#endif // WIFI_HH