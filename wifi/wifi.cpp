#include "wifi.h"
#include "ui/src/ui.h"

/*wifi扫描*/
#include <iwlib.h>
/*wifi获取IP*/
#include <sys/ioctl.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
/*wifi连接*/
#include <sys/wait.h>

// 扫描附近wifi
int wifi_scan(void)
{
    int sock = iw_sockets_open();  // 创建无线通信socket
    if(sock < 0) 
    {
        lv_label_set_text(ui_IPAddrLabel, "无法打开无线socket");
        lv_roller_set_options(ui_WiFiScanRoller, "无法打开无线socket", LV_ROLLER_MODE_NORMAL);
        return -1;
    }

    /******扫描附近WiFi******/
    lv_label_set_text(ui_IPAddrLabel, "正在检查当前连接...");
    lv_roller_set_options(ui_WiFiScanRoller, "正在扫描附近WiFi...", LV_ROLLER_MODE_NORMAL);

    char buffer[4096] = {0};         // 存储WiFi列表的缓冲区
    wireless_scan_head scan_results; // 扫描结果链表头

    // 执行WiFi扫描
    if(iw_scan(sock, (char *)"wlan0", 30, &scan_results) < 0) 
    {
        close(sock);
        lv_label_set_text(ui_IPAddrLabel, "无法连接...");
        lv_roller_set_options(ui_WiFiScanRoller, "WiFi扫描失败", LV_ROLLER_MODE_NORMAL);
        return -1;
    }

    int is_first = 1;  // 标记是否为第一个SSID
    wireless_scan *ap = scan_results.result;
    
    // 遍历扫描结果
    while(ap != NULL) 
    {
        char ssid[IW_ESSID_MAX_SIZE + 1] = {0};
        uint32_t ssid_len = strlen(ap->b.essid);
        
        // printf("[DEBUG] ESSID: '%s' (len=%d)\n", ap->b.essid, ssid_len);

        if(ssid_len > 0) 
        {  
            strncpy(ssid, (const char*)ap->b.essid, ssid_len);
            ssid[ssid_len] = '\0';
            
            // 额外检查空ESSID
            if(ssid[0] == '\0')strcpy(ssid, "隐藏网络");
        } 
        else strcpy(ssid, "隐藏网络");
             
        // 拼接SSID到缓冲区
        if(strlen(buffer) + strlen(ssid) + 2 <= sizeof(buffer))
        {
            if(!is_first)strcat(buffer, "\n");    
            strcat(buffer, ssid);
            is_first = 0;
        }
        
        ap = ap->next;  // 下一个接入点
    }

    // 更新UI控件
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

    /******获取当前IP地址******/
    struct wireless_config config;

    if(iw_get_basic_config(sock, (const char *)"wlan0", &config) < 0) 
    {
        lv_label_set_text(ui_IPAddrLabel, "无连接...");
        close(sock);
        return -1;
    }

    close(sock);

    // 检查SSID是否有效
    if(config.essid[0] == '\0')
    {
        lv_label_set_text(ui_IPAddrLabel, "无法获取IP地址...");
        return -1;
    }
    
    // 获取IP地址
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
            // printf("WiFi名称: %s, IP地址: %s\n", config.essid, ipstr);
            lv_label_set_text_fmt(ui_IPAddrLabel, "已连接:%s IP:%s", config.essid, ipstr);
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
    // 转义SSID和密码中的特殊字符
    char safe_ssid[256], safe_pass[256];
    escape_shell_chars(ssid, safe_ssid, sizeof(safe_ssid));
    escape_shell_chars(pass, safe_pass, sizeof(safe_pass));

    // printf("%s,%s\r\n",safe_ssid,safe_pass);

    // 构建nmcli命令
    char cmd[512];
    snprintf(cmd, sizeof(cmd), 
        "sudo nmcli --terse dev wifi connect %s password %s 2>&1",
        safe_ssid, safe_pass);

    // 执行命令并捕获输出
    FILE *fp = popen(cmd, "r");
    if(!fp) 
    {
        lv_label_set_text(ui_IPAddrLabel, "连接失败");
        _ui_flag_modify(ui_WiFiConnectWaitSpinner, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_ADD);
        return -1;
    }

    // 读取命令输出
    char output[512] = {0};
    char line[128];
    while(fgets(line, sizeof(line), fp)) 
    {
        strncat(output, line, sizeof(output)-1);
    }
    _ui_flag_modify(ui_WiFiConnectWaitSpinner, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_ADD);

    // 获取执行状态
    int status = pclose(fp);
    int exit_code = WEXITSTATUS(status);

    // 结果处理
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

// 辅助函数：转义特殊字符
void escape_shell_chars(const char *input, char *output, uint16_t out_size) 
{
    uint16_t j = 0;
    output[j++] = '\''; // 开始单引号
    for(uint16_t i = 0; input[i] != '\0' && j < out_size-2; ++i) 
    {
        if(input[i] == '\'') 
        {
            // 单引号转义为 '\''
            if(j + 4 < out_size) 
            {
                output[j++] = '\''; // 结束当前引号
                output[j++] = '\\'; // 反斜杠
                output[j++] = '\''; // 新引号
                output[j++] = '\''; // 重新开始引号
            }
        } 
        else 
        {
            output[j++] = input[i];
        }
    }
    output[j++] = '\''; // 结束单引号
    output[j] = '\0';
}
