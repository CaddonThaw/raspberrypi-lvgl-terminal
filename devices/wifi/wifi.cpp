#include "wifi.h"
#include "ui/src/ui.h"

/* WiFi scanning */
#include <iwlib.h>
/* WiFi getting IP */
#include <sys/ioctl.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
/* WiFi connection */
#include <sys/wait.h>

// Scan nearby WiFi networks
int wifi_scan(void)
{
    int sock = iw_sockets_open();  // Create wireless communication socket
    if(sock < 0) 
    {
        lv_label_set_text(ui_IPAddrLabel, "无法打开无线socket");
        lv_roller_set_options(ui_WiFiScanRoller, "无法打开无线socket", LV_ROLLER_MODE_NORMAL);
        return -1;
    }

    /****** Scan nearby WiFi ******/
    lv_label_set_text(ui_IPAddrLabel, "正在检查当前连接...");
    lv_roller_set_options(ui_WiFiScanRoller, "正在扫描附近WiFi...", LV_ROLLER_MODE_NORMAL);

    char buffer[4096] = {0};         // Buffer to store WiFi list
    wireless_scan_head scan_results; // Scan results linked list head

    // Execute WiFi scan
    if(iw_scan(sock, (char *)"wlan0", 30, &scan_results) < 0) 
    {
        close(sock);
        lv_label_set_text(ui_IPAddrLabel, "无法连接...");
        lv_roller_set_options(ui_WiFiScanRoller, "WiFi扫描失败", LV_ROLLER_MODE_NORMAL);
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
    lv_roller_set_options(ui_WiFiScanRoller, buffer, LV_ROLLER_MODE_NORMAL);
    if(wifi_IP() == 0)return 0;
    else return -1;
}

int wifi_IP(void)
{
    int sock = iw_sockets_open();
    if(sock < 0) 
    {
        lv_label_set_text(ui_IPAddrLabel, "无法打开无线socket");
        return -1;
    }

    /****** Get current IP address ******/
    struct wireless_config config;

    if(iw_get_basic_config(sock, (const char *)"wlan0", &config) < 0) 
    {
        lv_label_set_text(ui_IPAddrLabel, "无连接...");
        close(sock);
        return -1;
    }

    close(sock);

    // Check if SSID is valid
    if(config.essid[0] == '\0')
    {
        lv_label_set_text(ui_IPAddrLabel, "无法获取IP地址...");
        return -1;
    }
    
    // Get IP address
    struct ifaddrs *ifaddr, *ifa;
    char ipstr[INET_ADDRSTRLEN];

    if(getifaddrs(&ifaddr) == -1) 
    {
        lv_label_set_text(ui_IPAddrLabel, "无法获取IP地址...");
        return -1;
    }

    for(ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) 
    {
        if(ifa->ifa_addr == NULL || strcmp(ifa->ifa_name, (const char *)"wlan0") != 0) continue;
        
        if(ifa->ifa_addr->sa_family == AF_INET) 
        {   
            // IPv4
            struct sockaddr_in *addr = (struct sockaddr_in *)ifa->ifa_addr;
            inet_ntop(AF_INET, &addr->sin_addr, ipstr, sizeof(ipstr));
            // printf("WiFi Name: %s, IP Address: %s\n", config.essid, ipstr);
            lv_label_set_text_fmt(ui_IPAddrLabel, "Connected:%s IP:%s", config.essid, ipstr);
            freeifaddrs(ifaddr);
            return 0;
        }
    }

    freeifaddrs(ifaddr);
    lv_label_set_text(ui_IPAddrLabel, "没有找到IPv4地址...");
    return -1; 
}

int wifi_connect(const char* ssid, const char* pass)
{
    // Escape special characters in SSID and password
    char safe_ssid[256], safe_pass[256];
    escape_shell_chars(ssid, safe_ssid, sizeof(safe_ssid));
    escape_shell_chars(pass, safe_pass, sizeof(safe_pass));

    // printf("%s,%s\r\n",safe_ssid,safe_pass);

    // Build nmcli command
    char cmd[512];
    snprintf(cmd, sizeof(cmd), 
        "sudo nmcli --terse dev wifi connect %s password %s 2>&1",
        safe_ssid, safe_pass);

    // Execute command and capture output
    FILE *fp = popen(cmd, "r");
    if(!fp) 
    {
        lv_label_set_text(ui_IPAddrLabel, "连接失败");
        _ui_flag_modify(ui_WiFiConnectWaitSpinner, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_ADD);
        return -1;
    }

    // Read command output
    char output[512] = {0};
    char line[128];
    while(fgets(line, sizeof(line), fp)) 
    {
        strncat(output, line, sizeof(output)-1);
    }
    _ui_flag_modify(ui_WiFiConnectWaitSpinner, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_ADD);

    // Get execution status
    int status = pclose(fp);
    int exit_code = WEXITSTATUS(status);

    // Result processing
    if(exit_code == 0) 
    {
        wifi_IP(); // 更新IP显示
        return 0;
    }
    else 
    {
        lv_label_set_text(ui_IPAddrLabel, "连接失败");
        return -1;
    }
}

// Helper function: escape special characters
void escape_shell_chars(const char *input, char *output, uint16_t out_size) 
{
    uint16_t j = 0;
    output[j++] = '\''; // Start single quote
    for(uint16_t i = 0; input[i] != '\0' && j < out_size-2; ++i) 
    {
        if(input[i] == '\'') 
        {
            // Single quote escaped as '\'''
            if(j + 4 < out_size) 
            {
                output[j++] = '\''; // End current quote
                output[j++] = '\\'; // Backslash
                output[j++] = '\''; // New quote
                output[j++] = '\''; // Restart quote
            }
        } 
        else 
        {
            output[j++] = input[i];
        }
    }
    output[j++] = '\''; // End single quote
    output[j] = '\0';
}
