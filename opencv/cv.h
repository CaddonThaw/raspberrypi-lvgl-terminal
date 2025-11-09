#ifndef CV_H
#define CV_H

#include <opencv2/opencv.hpp>
#include <unistd.h>
#include <stdbool.h> 

bool cv_init();
void cv_loop();
void cv_deinit(bool ret);
uint8_t* transfor_image(uint16_t *data);

#endif // CV_H