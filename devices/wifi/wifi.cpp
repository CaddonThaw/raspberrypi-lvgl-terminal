#include "wifi.h"
#include "ui/ui.h"
#include <iwlib.h>
#include <sys/wait.h>

// Scan nearby WiFi networks
int wifi_scan(void)
{
    int sock = iw_sockets_open();  // Create wireless communication socket
    if(sock < 0) 
    {
        ui_wifi_set_scan_results( "无法打开无线socket");
        return -1;
    }

    /****** Scan nearby WiFi ******/
    ui_wifi_set_scan_results("正在检查当前连接...");
    ui_wifi_set_scan_results("正在扫描附近WiFi...");

    char buffer[4096] = {0};         // Buffer to store WiFi list
    wireless_scan_head scan_results; // Scan results linked list head

    // Execute WiFi scan
    if(iw_scan(sock, (char *)"wlan0", 30, &scan_results) < 0) 
    {
        close(sock);
        ui_wifi_set_scan_results("无法连接...");
        return -1;
    }

    int is_first = 1;  // Flag to mark if it's the first SSID
    wireless_scan *ap = scan_results.result;
    
    // Iterate through scan results
    while(ap != NULL) 
    {
        char ssid[IW_ESSID_MAX_SIZE + 1] = {0};
        uint32_t ssid_len = strlen(ap->b.essid);
        
        // printf("[DEBUG] ESSID: '%s' (len=%d)\n", ap->b.essid, ssid_len);

        if(ssid_len > 0) 
        {  
            strncpy(ssid, (const char*)ap->b.essid, ssid_len);
            ssid[ssid_len] = '\0';
            
            // Additional check for empty ESSID
            if(ssid[0] == '\0')strcpy(ssid, "隐藏网络");
        } 
        else strcpy(ssid, "隐藏网络");
             
        // Concatenate SSID to buffer
        if(strlen(buffer) + strlen(ssid) + 2 <= sizeof(buffer))
        {
            if(!is_first)strcat(buffer, "\n");    
            strcat(buffer, ssid);
            is_first = 0;
        }
        
        ap = ap->next;  // Next access point
    }

    // Update UI controls
    ui_wifi_set_scan_results(buffer);
    
    return 0;
}
