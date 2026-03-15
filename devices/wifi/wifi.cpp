#include "wifi.h"
#include "devices/threads/threads_conf.h"
#include "ui/ui.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <iwlib.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/wait.h>

namespace {

constexpr int WIFI_PORTAL_PORT = 8080;
constexpr size_t WIFI_HTTP_BUFFER_SIZE = 2048;
constexpr const char *WIFI_PORTAL_IFNAME = "wlan0";
constexpr const char *WIFI_PORTAL_CONN_NAME = "PiTerminalHotspot";
constexpr const char *WIFI_PORTAL_AP_SSID = "PiTerminal-Setup";
constexpr const char *WIFI_PORTAL_AP_PASSWORD = "12345678";
constexpr const char *WIFI_STATION_IP = "192.168.5.4";
constexpr const char *WIFI_PORTAL_HOTSPOT_URL = "http://10.42.0.1:8080";

bool g_wifi_portal_hotspot_active = false;
int g_wifi_portal_server_fd = -1;

void url_encode(const char *src, char *dst, size_t dst_size)
{
    static const char hex[] = "0123456789ABCDEF";
    size_t write_index = 0;

    if(dst_size == 0)
    {
        return;
    }

    for(size_t read_index = 0; src && src[read_index] != '\0'; ++read_index)
    {
        unsigned char ch = static_cast<unsigned char>(src[read_index]);
        bool is_safe =
            (ch >= 'a' && ch <= 'z') ||
            (ch >= 'A' && ch <= 'Z') ||
            (ch >= '0' && ch <= '9') ||
            ch == '-' || ch == '_' || ch == '.' || ch == '~';

        if(is_safe)
        {
            if(write_index + 1 >= dst_size)
            {
                break;
            }

            dst[write_index++] = static_cast<char>(ch);
            continue;
        }

        if(write_index + 3 >= dst_size)
        {
            break;
        }

        dst[write_index++] = '%';
        dst[write_index++] = hex[(ch >> 4) & 0x0F];
        dst[write_index++] = hex[ch & 0x0F];
    }

    dst[write_index] = '\0';
}

bool wifi_get_interface_ip(const char *interface_name, char *buffer, size_t buffer_size)
{
    struct ifaddrs *ifaddr = NULL;
    struct ifaddrs *ifa = NULL;

    if(buffer_size == 0)
    {
        return false;
    }

    buffer[0] = '\0';

    if(getifaddrs(&ifaddr) != 0)
    {
        return false;
    }

    for(ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        if(ifa->ifa_addr == NULL || ifa->ifa_addr->sa_family != AF_INET)
        {
            continue;
        }

        if(interface_name != NULL && strcmp(ifa->ifa_name, interface_name) != 0)
        {
            continue;
        }

        if((ifa->ifa_flags & IFF_LOOPBACK) != 0)
        {
            continue;
        }

        struct sockaddr_in *addr = (struct sockaddr_in *)ifa->ifa_addr;
        if(inet_ntop(AF_INET, &addr->sin_addr, buffer, buffer_size) != NULL)
        {
            freeifaddrs(ifaddr);
            return true;
        }
    }

    freeifaddrs(ifaddr);
    return false;
}

std::string shell_escape(const char *value)
{
    std::string escaped = "'";

    for(const char *cursor = value; cursor && *cursor != '\0'; ++cursor)
    {
        if(*cursor == '\'')
        {
            escaped += "'\\''";
        }
        else
        {
            escaped += *cursor;
        }
    }

    escaped += "'";
    return escaped;
}

int run_command_with_logs(const std::string &command, const char *log_tag)
{
    FILE *pipe = popen(command.c_str(), "r");
    char line[256];
    int status;

    if(pipe == NULL)
    {
        printf("[%s] failed to execute command\n", log_tag);
        return -1;
    }

    while(fgets(line, sizeof(line), pipe) != NULL)
    {
        printf("[%s] %s", log_tag, line);
    }

    status = pclose(pipe);
    if(status == 0)
    {
        return 0;
    }

    if(WIFEXITED(status))
    {
        printf("[%s] command failed, exit code=%d\n", log_tag, WEXITSTATUS(status));
    }
    else
    {
        printf("[%s] command failed, status=%d\n", log_tag, status);
    }

    return -1;
}

void trim_trailing_newline(char *text)
{
    size_t length;

    if(text == NULL)
    {
        return;
    }

    length = strlen(text);
    while(length > 0 && (text[length - 1] == '\n' || text[length - 1] == '\r'))
    {
        text[length - 1] = '\0';
        --length;
    }
}

void wifi_update_ui_status_title(void)
{
    char status_text[128];

    wifi_build_status_text(status_text, sizeof(status_text));
    ui_wifi_set_title(status_text);
}

void html_escape(const char *src, char *dst, size_t dst_size)
{
    size_t write_index = 0;

    if(dst_size == 0)
    {
        return;
    }

    for(size_t read_index = 0; src && src[read_index] != '\0'; ++read_index)
    {
        const char *replacement = NULL;

        switch(src[read_index])
        {
            case '&': replacement = "&amp;"; break;
            case '<': replacement = "&lt;"; break;
            case '>': replacement = "&gt;"; break;
            case '"': replacement = "&quot;"; break;
            case '\'': replacement = "&#39;"; break;
            default: break;
        }

        if(replacement != NULL)
        {
            size_t replacement_len = strlen(replacement);
            if(write_index + replacement_len >= dst_size)
            {
                break;
            }

            memcpy(dst + write_index, replacement, replacement_len);
            write_index += replacement_len;
            continue;
        }

        if(write_index + 1 >= dst_size)
        {
            break;
        }

        dst[write_index++] = src[read_index];
    }

    dst[write_index] = '\0';
}

void url_decode(const char *src, char *dst, size_t dst_size)
{
    size_t write_index = 0;

    if(dst_size == 0)
    {
        return;
    }

    for(size_t read_index = 0; src && src[read_index] != '\0'; ++read_index)
    {
        char ch = src[read_index];

        if(ch == '+')
        {
            if(write_index + 1 >= dst_size)
            {
                break;
            }

            dst[write_index++] = ' ';
            continue;
        }

        if(ch == '%' && src[read_index + 1] != '\0' && src[read_index + 2] != '\0')
        {
            char hex[3] = {src[read_index + 1], src[read_index + 2], '\0'};
            if(write_index + 1 >= dst_size)
            {
                break;
            }

            dst[write_index++] = static_cast<char>(strtol(hex, NULL, 16));
            read_index += 2;
            continue;
        }

        if(write_index + 1 >= dst_size)
        {
            break;
        }

        dst[write_index++] = ch;
    }

    dst[write_index] = '\0';
}

bool get_query_value(const char *query, const char *key, char *buffer, size_t buffer_size)
{
    char pattern[64];
    const char *start;
    const char *end;
    size_t raw_len;
    char raw_value[512];

    if(query == NULL || key == NULL || buffer_size == 0)
    {
        return false;
    }

    snprintf(pattern, sizeof(pattern), "%s=", key);
    start = strstr(query, pattern);
    if(start == NULL)
    {
        return false;
    }

    start += strlen(pattern);
    end = strchr(start, '&');
    raw_len = end ? static_cast<size_t>(end - start) : strlen(start);
    if(raw_len >= sizeof(raw_value))
    {
        raw_len = sizeof(raw_value) - 1;
    }

    memcpy(raw_value, start, raw_len);
    raw_value[raw_len] = '\0';
    url_decode(raw_value, buffer, buffer_size);
    return true;
}

void send_http_response(int client_fd, const char *content)
{
    char response[4096];
    int written = snprintf(response,
                           sizeof(response),
                           "HTTP/1.1 200 OK\r\n"
                           "Content-Type: text/html; charset=UTF-8\r\n"
                           "Content-Length: %zu\r\n"
                           "Connection: close\r\n\r\n%s",
                           strlen(content),
                           content);

    if(written > 0)
    {
        send(client_fd, response, static_cast<size_t>(written), 0);
    }
}

void send_phone_form(int client_fd, const char *ssid)
{
    char html[4096];
    char escaped_ssid[512];

    html_escape(ssid, escaped_ssid, sizeof(escaped_ssid));

    snprintf(html,
             sizeof(html),
             "<!doctype html>"
             "<html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>"
             "<title>Wi-Fi Connect</title>"
             "<style>body{font-family:sans-serif;background:#fff4f4;color:#1a1a1a;padding:20px;}"
             ".card{max-width:360px;margin:0 auto;background:#fff;border-radius:16px;padding:20px;box-shadow:0 8px 30px rgba(0,0,0,.08);}"
             "h1{font-size:20px;margin:0 0 8px;color:#cc0000;}p{margin:0 0 16px;word-break:break-word;}"
             "input{width:100%%;box-sizing:border-box;padding:12px 14px;border:1px solid #ddb0b0;border-radius:10px;font-size:16px;}"
             "button{width:100%%;margin-top:14px;padding:12px 14px;border:0;border-radius:10px;background:#e07070;color:#fff;font-size:16px;}"
             "</style></head><body><div class='card'><h1>输入 Wi-Fi 密码</h1><p>%s</p>"
             "<form action='/submit' method='get'>"
             "<input type='hidden' name='ssid' value='%s'>"
             "<input type='password' name='password' placeholder='Password' autofocus required>"
             "<button type='submit'>确定</button>"
             "</form></div></body></html>",
             escaped_ssid,
             escaped_ssid);

    send_http_response(client_fd, html);
}

void send_submit_result(int client_fd, const char *ssid)
{
    char html[2048];
    char escaped_ssid[512];

    html_escape(ssid, escaped_ssid, sizeof(escaped_ssid));

    snprintf(html,
             sizeof(html),
             "<!doctype html>"
             "<html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>"
             "<title>Submitted</title>"
             "<style>body{font-family:sans-serif;background:#fff4f4;color:#1a1a1a;padding:20px;}"
             ".card{max-width:360px;margin:0 auto;background:#fff;border-radius:16px;padding:20px;box-shadow:0 8px 30px rgba(0,0,0,.08);}"
             "h1{font-size:20px;margin:0 0 8px;color:#cc0000;}p{margin:0;word-break:break-word;}"
             "</style></head><body><div class='card'><h1>已提交</h1><p>%s</p><p>设备正在连接，请返回屏幕查看状态。</p></div></body></html>",
             escaped_ssid);

    send_http_response(client_fd, html);
}

void handle_http_request(int client_fd)
{
    char request[WIFI_HTTP_BUFFER_SIZE];
    char method[16];
    char target[1024];
    char ssid[256];
    char password[256];
    const char *query;
    ssize_t read_size;

    memset(request, 0, sizeof(request));
    read_size = recv(client_fd, request, sizeof(request) - 1, 0);
    if(read_size <= 0)
    {
        return;
    }

    method[0] = '\0';
    target[0] = '\0';
    if(sscanf(request, "%15s %1023s", method, target) != 2)
    {
        return;
    }

    if(strcmp(method, "GET") != 0)
    {
        send_http_response(client_fd, "<html><body>Unsupported method</body></html>");
        return;
    }

    ssid[0] = '\0';
    password[0] = '\0';
    query = strchr(target, '?');

    if(query != NULL)
    {
        ++query;
        get_query_value(query, "ssid", ssid, sizeof(ssid));
        get_query_value(query, "password", password, sizeof(password));
    }

    if(strncmp(target, "/submit", 7) == 0)
    {
        if(ssid[0] == '\0')
        {
            snprintf(ssid, sizeof(ssid), "%s", ui_wifi_get_selected_ssid());
        }

        printf("[wifi] received phone password for SSID: %s\n", ssid);
        wifi_get_ssid(ssid);
        wifi_get_pass(password);
        wifi_isconnect();
        ui_wifi_submit_phone_password(password);
        send_submit_result(client_fd, ssid);
        return;
    }

    if(ssid[0] == '\0')
    {
        snprintf(ssid, sizeof(ssid), "%s", ui_wifi_get_selected_ssid());
    }

    send_phone_form(client_fd, ssid);
}

void wifi_qr_escape(const char *src, char *dst, size_t dst_size)
{
    size_t write_index = 0;

    if(dst_size == 0)
    {
        return;
    }

    for(size_t read_index = 0; src && src[read_index] != '\0'; ++read_index)
    {
        char ch = src[read_index];
        bool needs_escape = ch == '\\' || ch == ';' || ch == ',' || ch == ':' || ch == '"';

        if(needs_escape)
        {
            if(write_index + 2 >= dst_size)
            {
                break;
            }

            dst[write_index++] = '\\';
        }
        else if(write_index + 1 >= dst_size)
        {
            break;
        }

        dst[write_index++] = ch;
    }

    dst[write_index] = '\0';
}

void wifi_build_hotspot_join_qr(char *buffer, size_t buffer_size)
{
    char escaped_ssid[128];
    char escaped_password[128];

    if(buffer_size == 0)
    {
        return;
    }

    wifi_qr_escape(WIFI_PORTAL_AP_SSID, escaped_ssid, sizeof(escaped_ssid));
    wifi_qr_escape(WIFI_PORTAL_AP_PASSWORD, escaped_password, sizeof(escaped_password));

    snprintf(buffer,
             buffer_size,
             "WIFI:T:WPA;S:%s;P:%s;;",
             escaped_ssid,
             escaped_password);
}

int wifi_start_phone_portal_hotspot(void)
{
    std::string command;

    if(g_wifi_portal_hotspot_active)
    {
        return 0;
    }

    command = "nmcli --wait 20 device wifi hotspot ifname ";
    command += WIFI_PORTAL_IFNAME;
    command += " con-name ";
    command += shell_escape(WIFI_PORTAL_CONN_NAME);
    command += " ssid ";
    command += shell_escape(WIFI_PORTAL_AP_SSID);
    command += " password ";
    command += shell_escape(WIFI_PORTAL_AP_PASSWORD);
    command += " 2>&1";

    printf("[wifi] starting phone portal hotspot: %s\n", WIFI_PORTAL_AP_SSID);
    if(run_command_with_logs(command, "wifi") != 0)
    {
        return -1;
    }

    g_wifi_portal_hotspot_active = true;
    return 0;
}

void wifi_stop_phone_portal_hotspot_internal(void)
{
    std::string command;

    if(!g_wifi_portal_hotspot_active)
    {
        return;
    }

    command = "nmcli --wait 10 connection down ";
    command += shell_escape(WIFI_PORTAL_CONN_NAME);
    command += " 2>&1";

    printf("[wifi] stopping phone portal hotspot\n");
    if(run_command_with_logs(command, "wifi") != 0)
    {
        command = "nmcli --wait 10 device disconnect ";
        command += WIFI_PORTAL_IFNAME;
        command += " 2>&1";
        run_command_with_logs(command, "wifi");
    }

    g_wifi_portal_hotspot_active = false;
}

}  // namespace

bool wifi_portal_start(void)
{
    int opt = 1;
    struct sockaddr_in addr;

    if(g_wifi_portal_server_fd >= 0)
    {
        return true;
    }

    g_wifi_portal_server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(g_wifi_portal_server_fd < 0)
    {
        printf("[wifi] portal socket create failed: %s\n", strerror(errno));
        return false;
    }

    setsockopt(g_wifi_portal_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    fcntl(g_wifi_portal_server_fd, F_SETFL, O_NONBLOCK);

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(WIFI_PORTAL_PORT);

    if(bind(g_wifi_portal_server_fd, (struct sockaddr *)&addr, sizeof(addr)) != 0)
    {
        printf("[wifi] portal bind failed: %s\n", strerror(errno));
        close(g_wifi_portal_server_fd);
        g_wifi_portal_server_fd = -1;
        return false;
    }

    if(listen(g_wifi_portal_server_fd, 4) != 0)
    {
        printf("[wifi] portal listen failed: %s\n", strerror(errno));
        close(g_wifi_portal_server_fd);
        g_wifi_portal_server_fd = -1;
        return false;
    }

    printf("[wifi] phone portal started on port %d\n", WIFI_PORTAL_PORT);
    return true;
}

void wifi_portal_stop(void)
{
    if(g_wifi_portal_server_fd >= 0)
    {
        close(g_wifi_portal_server_fd);
        g_wifi_portal_server_fd = -1;
        printf("[wifi] phone portal stopped\n");
    }
}

void wifi_portal_poll(void)
{
    fd_set read_set;
    struct timeval timeout;
    int ready;
    int client_fd;

    if(g_wifi_portal_server_fd < 0)
    {
        return;
    }

    FD_ZERO(&read_set);
    FD_SET(g_wifi_portal_server_fd, &read_set);
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    ready = select(g_wifi_portal_server_fd + 1, &read_set, NULL, NULL, &timeout);
    if(ready <= 0 || !FD_ISSET(g_wifi_portal_server_fd, &read_set))
    {
        return;
    }

    client_fd = accept(g_wifi_portal_server_fd, NULL, NULL);
    if(client_fd < 0)
    {
        return;
    }

    handle_http_request(client_fd);
    close(client_fd);
}

// Scan nearby WiFi networks
int wifi_scan(void)
{
    int sock = iw_sockets_open();  // Create wireless communication socket
    wifi_update_ui_status_title();

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
            if(ssid_len >= sizeof(ssid))
            {
                ssid_len = sizeof(ssid) - 1;
            }

            memcpy(ssid, (const char*)ap->b.essid, ssid_len);
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
    if(buffer[0] == '\0')
    {
        ui_wifi_set_scan_results("No Wi-Fi Found");
    }
    else
    {
        ui_wifi_set_scan_results(buffer);
    }
    
    return 0;
}

bool wifi_get_connected_ssid(char *buffer, size_t buffer_size)
{
    FILE *pipe;
    char line[256];
    const char *prefix;

    if(buffer == NULL || buffer_size == 0)
    {
        return false;
    }

    buffer[0] = '\0';

    pipe = popen("wpa_cli -i wlan0 status 2>/dev/null", "r");
    if(pipe == NULL)
    {
        return false;
    }

    while(fgets(line, sizeof(line), pipe) != NULL)
    {
        prefix = "ssid=";
        if(strncmp(line, prefix, strlen(prefix)) == 0)
        {
            snprintf(buffer, buffer_size, "%s", line + strlen(prefix));
            trim_trailing_newline(buffer);
            pclose(pipe);
            return buffer[0] != '\0';
        }
    }

    pclose(pipe);
    pipe = popen("nmcli -t -f ACTIVE,SSID dev wifi 2>/dev/null", "r");
    if(pipe != NULL)
    {
        while(fgets(line, sizeof(line), pipe) != NULL)
        {
            if(strncmp(line, "yes:", 4) == 0)
            {
                snprintf(buffer, buffer_size, "%s", line + 4);
                trim_trailing_newline(buffer);
                pclose(pipe);
                return buffer[0] != '\0';
            }
        }

        pclose(pipe);
    }

    pipe = popen("iwgetid wlan0 -r 2>/dev/null", "r");
    if(pipe == NULL)
    {
        return false;
    }

    if(fgets(buffer, (int)buffer_size, pipe) == NULL)
    {
        pclose(pipe);
        buffer[0] = '\0';
        return false;
    }

    pclose(pipe);
    trim_trailing_newline(buffer);
    return buffer[0] != '\0';
}

void wifi_build_status_text(char *buffer, size_t buffer_size)
{
    char connected_ssid[128];

    if(buffer == NULL || buffer_size == 0)
    {
        return;
    }

    if(wifi_get_connected_ssid(connected_ssid, sizeof(connected_ssid)))
    {
        snprintf(buffer, buffer_size, "Connecting %s", connected_ssid);
        return;
    }

    snprintf(buffer, buffer_size, "Disconnecting");
}

void wifi_build_phone_entry_url(const char *ssid, char *buffer, size_t buffer_size)
{
    char encoded_ssid[256];
    char ip_address[64];
    std::string portal_url;

    if(buffer_size == 0)
    {
        return;
    }

    url_encode(ssid ? ssid : "", encoded_ssid, sizeof(encoded_ssid));
    if(!wifi_get_interface_ip(WIFI_PORTAL_IFNAME, ip_address, sizeof(ip_address)) || ip_address[0] == '\0')
    {
        snprintf(ip_address, sizeof(ip_address), "%s", WIFI_STATION_IP);
    }

    portal_url = std::string("http://") + ip_address + ":" + std::to_string(WIFI_PORTAL_PORT) + "/?ssid=" + encoded_ssid;

    snprintf(buffer,
             buffer_size,
             "%s",
             portal_url.c_str());
}

bool wifi_is_station_connected(void)
{
    char ip_address[64];

    if(!wifi_get_interface_ip(WIFI_PORTAL_IFNAME, ip_address, sizeof(ip_address)))
    {
        return false;
    }

    return strcmp(ip_address, "10.42.0.1") != 0;
}

int wifi_prepare_phone_portal(const char *ssid,
                              char *qr_payload,
                              size_t qr_payload_size,
                              char *portal_hint,
                              size_t portal_hint_size)
{
    char portal_url[256];
    char current_ip[64];

    if(qr_payload_size > 0)
    {
        qr_payload[0] = '\0';
    }

    if(portal_hint_size > 0)
    {
        portal_hint[0] = '\0';
    }

    printf("[wifi] preparing phone portal for SSID: %s\n", ssid ? ssid : "");
    wifi_update_ui_status_title();

    if(wifi_is_station_connected())
    {
        wifi_build_phone_entry_url(ssid, portal_url, sizeof(portal_url));
        if(!wifi_get_interface_ip(WIFI_PORTAL_IFNAME, current_ip, sizeof(current_ip)) || current_ip[0] == '\0')
        {
            snprintf(current_ip, sizeof(current_ip), "%s", WIFI_STATION_IP);
        }
        snprintf(qr_payload, qr_payload_size, "%s", portal_url);
        snprintf(portal_hint,
                 portal_hint_size,
                 "Connect phone to %s and open %s",
                 current_ip,
                 portal_url);
        printf("[wifi] using station portal: %s\n", portal_url);
        return 1;
    }

    printf("[wifi] station network unavailable, trying hotspot portal\n");

    if(wifi_start_phone_portal_hotspot() == 0)
    {
        wifi_build_hotspot_join_qr(qr_payload, qr_payload_size);
        snprintf(portal_hint,
                 portal_hint_size,
                 "Connect %s and open %s",
                 WIFI_PORTAL_AP_SSID,
                 WIFI_PORTAL_HOTSPOT_URL);

        printf("[wifi] hotspot ready, portal=%s\n", WIFI_PORTAL_HOTSPOT_URL);
        return 0;
    }

    snprintf(qr_payload, qr_payload_size, "%s", WIFI_PORTAL_HOTSPOT_URL);
    snprintf(portal_hint,
             portal_hint_size,
             "Hotspot failed, open %s manually",
             WIFI_PORTAL_HOTSPOT_URL);
    printf("[wifi] hotspot unavailable, manual portal=%s\n", WIFI_PORTAL_HOTSPOT_URL);
    return -1;
}

void wifi_stop_phone_portal(void)
{
    wifi_stop_phone_portal_hotspot_internal();
}

int wifi_connect(const char *ssid, const char *password)
{
    std::string command;

    if(ssid == NULL || ssid[0] == '\0' || password == NULL)
    {
        printf("[wifi] invalid connect request, ssid or password missing\n");
        return -1;
    }

    wifi_stop_phone_portal_hotspot_internal();

    command = "nmcli --wait 20 dev wifi connect ";
    command += shell_escape(ssid);
    command += " password ";
    command += shell_escape(password);
    command += " ifname ";
    command += WIFI_PORTAL_IFNAME;
    command += " 2>&1";

    printf("[wifi] connecting to SSID: %s\n", ssid);
    if(run_command_with_logs(command, "wifi") == 0)
    {
        printf("[wifi] connect success: %s\n", ssid);
        wifi_update_ui_status_title();
        return 0;
    }

    printf("[wifi] connect failed: %s\n", ssid);
    wifi_update_ui_status_title();

    return -1;
}
