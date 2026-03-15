#ifndef WIFI_HH
#define WIFI_HH

#include <unistd.h>
#include <stdio.h>
#include <cstdint>
#include <stddef.h>

int wifi_scan(void);
int wifi_connect(const char *ssid, const char *password);
bool wifi_get_connected_ssid(char *buffer, size_t buffer_size);
void wifi_build_status_text(char *buffer, size_t buffer_size);
bool wifi_build_phone_entry_url(const char *ssid, char *buffer, size_t buffer_size);
int wifi_prepare_phone_portal(const char *ssid,
							  char *qr_payload,
							  size_t qr_payload_size,
							  char *portal_hint,
							  size_t portal_hint_size);
bool wifi_portal_start(void);
void wifi_portal_stop(void);
void wifi_portal_poll(void);
void wifi_stop_phone_portal(void);

#endif // WIFI_HH