#pragma once
#include <stdint.h>

int rtmp_init(const char* url);
int rtmp_send_h264(uint8_t* data, int size, int64_t timestamp_ms, int is_keyframe);
void rtmp_close();
