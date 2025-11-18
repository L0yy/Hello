#pragma once
#include <stdint.h>

// Simple x264 wrapper
int encoder_init(int width, int height, int fps, int bitrate_kbps);
// encode I420 frame: pointers for Y, U, V planes and strides
int encoder_encode_frame(const uint8_t* y, const uint8_t* u, const uint8_t* v,
                         int stride_y, int stride_u, int stride_v,
                         uint8_t** out_buf, int* out_size);
void encoder_close();
