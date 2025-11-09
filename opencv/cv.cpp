#include "cv.h"
#include "lvgl/lvgl.h"
#include "ui/src/ui.h"

#define IMG_WIDTH       168
#define IMG_HEIGHT      168

#define FPS_SHOW        0

cv::VideoCapture cap;
cv::Mat frame, hsv, mask, red_mask;
cv::Mat trans_frame;

static lv_img_dsc_t cv_img_dsc = {
    .header = {
        .cf = LV_IMG_CF_TRUE_COLOR,  
        .always_zero = 0,
        .reserved = 0,
        .w = IMG_WIDTH,
        .h = IMG_HEIGHT
    },
    .data_size = IMG_WIDTH * IMG_HEIGHT * 2,
    .data = NULL  // 动态更新这个指针
};
lv_obj_t * cv_img;
lv_obj_t * cv_label;

bool cv_init()
{
    cv_label = lv_label_create(ui_OpenCV);
    lv_label_set_text(cv_label, "Loading Camera...");
    lv_obj_align(cv_label, LV_ALIGN_CENTER, 0, 0);

    cap.open(0);
    if(!cap.isOpened()) {
        lv_label_set_text(cv_label, "Failed to Open Camera!");
        return 0;
    }

#if FPS_SHOW
    // 帧率计算相关变量
    int64 start_time = cv::getTickCount();
    int frame_count = 0;
    double fps = 0;
#endif

    cv_img = lv_img_create(ui_OpenCV);
    lv_obj_align(cv_img, LV_ALIGN_CENTER, 0, 0);  
    lv_img_set_zoom(cv_img, 256);  
    lv_label_set_text(cv_label, " ");

    return 1;
}

void cv_loop()
{
    cap >> frame;  // 获取摄像头画面

    // 调整图像尺寸匹配屏幕分辨率
    cv::resize(frame, frame, cv::Size(IMG_WIDTH, IMG_HEIGHT));

    // 红色识别处理
    // 转换到HSV颜色空间
    cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);

    // 定义红色范围（HSV空间）
    cv::Mat lower_red, upper_red;
    cv::inRange(hsv, cv::Scalar(0, 70, 50), cv::Scalar(10, 255, 255), lower_red);    // 低范围红色
    cv::inRange(hsv, cv::Scalar(160, 70, 50), cv::Scalar(180, 255, 255), upper_red);  // 高范围红色
    
    // 合并红色掩膜
    red_mask = lower_red | upper_red;
    
    // 形态学处理
    cv::morphologyEx(red_mask, red_mask, cv::MORPH_OPEN, cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5,5)));
    
    // 查找轮廓
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(red_mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    
    // 绘制边界框
    for (const auto& contour : contours) {
        if(cv::contourArea(contour) > 500) { // 面积阈值过滤小噪声
            cv::Rect rect = cv::boundingRect(contour);
            cv::rectangle(frame, rect, cv::Scalar(0, 255, 0), 2); // 用绿色框标记
        }
    }

#if FPS_SHOW
    // 计算并显示FPS
    frame_count++;
    if (frame_count >= 10) { // 每10帧计算一次
        double elapsed = (cv::getTickCount() - start_time) / cv::getTickFrequency();
        fps = frame_count / elapsed;
        start_time = cv::getTickCount();
        frame_count = 0;
    }
    
    // 在画面上显示FPS
    std::string fps_str = cv::format("FPS:%.1f", fps);
    cv::putText(frame, fps_str, cv::Point(0, 20), cv::FONT_HERSHEY_SIMPLEX, 
                0.8, cv::Scalar(0, 255, 0), 2);
#endif
            
    // 将BGR转换为RGB格式
    cv::cvtColor(frame, trans_frame, cv::COLOR_BGR2BGR565);

    uint16_t *display_buffer = (uint16_t*)trans_frame.data;
    cv_img_dsc.data = (const uint8_t*)transfor_image(display_buffer);
    lv_img_set_src(cv_img, &cv_img_dsc);
}

void cv_deinit(bool ret)
{
    cap.release();
    cv_img_dsc.data = (const uint8_t*)new uint16_t[IMG_WIDTH * IMG_HEIGHT]{0xFFFF}; 
    lv_img_set_src(cv_img, cv_img_dsc.data); 
    if(!ret) lv_label_set_text(cv_label, "Failed to Open Camera!");
}

uint8_t* transfor_image(uint16_t *data) 
{
    // 创建转换缓冲区（仅需转换一次）
    static uint8_t converted_buffer[IMG_WIDTH * IMG_HEIGHT * 2]; 
    volatile uint8_t *dst = converted_buffer;
    volatile uint16_t *src = data;
    
    for(uint32_t i = 0; i < IMG_WIDTH * IMG_HEIGHT; i += 64) { 
        // 批量处理64个像素
        uint16_t p1 = *src++;uint16_t p2 = *src++;
        uint16_t p3 = *src++;uint16_t p4 = *src++;
        uint16_t p5 = *src++;uint16_t p6 = *src++;
        uint16_t p7 = *src++;uint16_t p8 = *src++;
        uint16_t p9 = *src++;uint16_t p10 = *src++;
        uint16_t p11 = *src++;uint16_t p12 = *src++;
        uint16_t p13 = *src++;uint16_t p14 = *src++;
        uint16_t p15 = *src++;uint16_t p16 = *src++;
        uint16_t p17 = *src++;uint16_t p18 = *src++;
        uint16_t p19 = *src++;uint16_t p20 = *src++;
        uint16_t p21 = *src++;uint16_t p22 = *src++;
        uint16_t p23 = *src++;uint16_t p24 = *src++;
        uint16_t p25 = *src++;uint16_t p26 = *src++;
        uint16_t p27 = *src++;uint16_t p28 = *src++;
        uint16_t p29 = *src++;uint16_t p30 = *src++;
        uint16_t p31 = *src++;uint16_t p32 = *src++;
        uint16_t p33 = *src++;uint16_t p34 = *src++;
        uint16_t p35 = *src++;uint16_t p36 = *src++;
        uint16_t p37 = *src++;uint16_t p38 = *src++;
        uint16_t p39 = *src++;uint16_t p40 = *src++;
        uint16_t p41 = *src++;uint16_t p42 = *src++;
        uint16_t p43 = *src++;uint16_t p44 = *src++;
        uint16_t p45 = *src++;uint16_t p46 = *src++;
        uint16_t p47 = *src++;uint16_t p48 = *src++;    
        uint16_t p49 = *src++;uint16_t p50 = *src++;
        uint16_t p51 = *src++;uint16_t p52 = *src++;
        uint16_t p53 = *src++;uint16_t p54 = *src++;
        uint16_t p55 = *src++;uint16_t p56 = *src++;
        uint16_t p57 = *src++;uint16_t p58 = *src++;
        uint16_t p59 = *src++;uint16_t p60 = *src++;
        uint16_t p61 = *src++;uint16_t p62 = *src++;
        uint16_t p63 = *src++;uint16_t p64 = *src++;

        *dst++ = p1 >> 8; *dst++ = p1;
        *dst++ = p2 >> 8; *dst++ = p2;
        *dst++ = p3 >> 8; *dst++ = p3;
        *dst++ = p4 >> 8; *dst++ = p4;   
        *dst++ = p5 >> 8; *dst++ = p5;
        *dst++ = p6 >> 8; *dst++ = p6;
        *dst++ = p7 >> 8; *dst++ = p7;
        *dst++ = p8 >> 8; *dst++ = p8;
        *dst++ = p9 >> 8; *dst++ = p9;
        *dst++ = p10 >> 8; *dst++ = p10;
        *dst++ = p11 >> 8; *dst++ = p11;
        *dst++ = p12 >> 8; *dst++ = p12;
        *dst++ = p13 >> 8; *dst++ = p13;
        *dst++ = p14 >> 8; *dst++ = p14;
        *dst++ = p15 >> 8; *dst++ = p15;
        *dst++ = p16 >> 8; *dst++ = p16;
        *dst++ = p17 >> 8; *dst++ = p17;
        *dst++ = p18 >> 8; *dst++ = p18;
        *dst++ = p19 >> 8; *dst++ = p19;
        *dst++ = p20 >> 8; *dst++ = p20;
        *dst++ = p21 >> 8; *dst++ = p21;
        *dst++ = p22 >> 8; *dst++ = p22;
        *dst++ = p23 >> 8; *dst++ = p23;
        *dst++ = p24 >> 8; *dst++ = p24;
        *dst++ = p25 >> 8; *dst++ = p25;
        *dst++ = p26 >> 8; *dst++ = p26;
        *dst++ = p27 >> 8; *dst++ = p27;
        *dst++ = p28 >> 8; *dst++ = p28;
        *dst++ = p29 >> 8; *dst++ = p29;
        *dst++ = p30 >> 8; *dst++ = p30;
        *dst++ = p31 >> 8; *dst++ = p31;
        *dst++ = p32 >> 8; *dst++ = p32;
        *dst++ = p33 >> 8; *dst++ = p33;
        *dst++ = p34 >> 8; *dst++ = p34;
        *dst++ = p35 >> 8; *dst++ = p35;
        *dst++ = p36 >> 8; *dst++ = p36;
        *dst++ = p37 >> 8; *dst++ = p37;
        *dst++ = p38 >> 8; *dst++ = p38;
        *dst++ = p39 >> 8; *dst++ = p39;
        *dst++ = p40 >> 8; *dst++ = p40;
        *dst++ = p41 >> 8; *dst++ = p41;
        *dst++ = p42 >> 8; *dst++ = p42;
        *dst++ = p43 >> 8; *dst++ = p43;
        *dst++ = p44 >> 8; *dst++ = p44;
        *dst++ = p45 >> 8; *dst++ = p45;
        *dst++ = p46 >> 8; *dst++ = p46;
        *dst++ = p47 >> 8; *dst++ = p47;
        *dst++ = p48 >> 8; *dst++ = p48;
        *dst++ = p49 >> 8; *dst++ = p49;
        *dst++ = p50 >> 8; *dst++ = p50;
        *dst++ = p51 >> 8; *dst++ = p51;
        *dst++ = p52 >> 8; *dst++ = p52;
        *dst++ = p53 >> 8; *dst++ = p53;
        *dst++ = p54 >> 8; *dst++ = p54;
        *dst++ = p55 >> 8; *dst++ = p55;
        *dst++ = p56 >> 8; *dst++ = p56;
        *dst++ = p57 >> 8; *dst++ = p57;
        *dst++ = p58 >> 8; *dst++ = p58;
        *dst++ = p59 >> 8; *dst++ = p59;
        *dst++ = p60 >> 8; *dst++ = p60;
        *dst++ = p61 >> 8; *dst++ = p61;
        *dst++ = p62 >> 8; *dst++ = p62;
        *dst++ = p63 >> 8; *dst++ = p63;
        *dst++ = p64 >> 8; *dst++ = p64;
    }

    return converted_buffer;
}