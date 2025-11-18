#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <windows.h>

#include "capture.h"
#include "encoder.h"
#include "rtmp_push.h"

static int64_t now_ms() {
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    unsigned long long t = ((unsigned long long)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
    // FILETIME is in 100-ns intervals since 1601-01-01
    return (int64_t)(t / 10000ULL);
}

// Simple BGRA -> I420 conversion
static void bgra_to_i420(const uint8_t* bgra, int stride, int w, int h,
                         uint8_t* y, uint8_t* u, uint8_t* v, int sy, int su, int sv) {
    for (int j = 0; j < h; j++) {
        const uint8_t* src = bgra + j * stride;
        uint8_t* py = y + j * sy;
        for (int i = 0; i < w; i++) {
            int b = src[i*4 + 0];
            int g = src[i*4 + 1];
            int r = src[i*4 + 2];
            int yy = ( (66*r + 129*g + 25*b + 128) >> 8) + 16;
            py[i] = (uint8_t) (yy < 0 ? 0 : (yy>255?255:yy));
        }
    }
    for (int j = 0; j < h/2; j++) {
        const uint8_t* src0 = bgra + (j*2) * stride;
        const uint8_t* src1 = bgra + (j*2+1) * stride;
        uint8_t* pu = u + j * su;
        uint8_t* pv = v + j * sv;
        for (int i = 0; i < w/2; i++) {
            int idx = i*2;
            int b = (src0[idx*4+0] + src0[(idx+1)*4+0] + src1[idx*4+0] + src1[(idx+1)*4+0]) / 4;
            int g = (src0[idx*4+1] + src0[(idx+1)*4+1] + src1[idx*4+1] + src1[(idx+1)*4+1]) / 4;
            int r = (src0[idx*4+2] + src0[(idx+1)*4+2] + src1[idx*4+2] + src1[(idx+1)*4+2]) / 4;
            int uu = ((-38*r - 74*g + 112*b + 128) >> 8) + 128;
            int vv = ((112*r - 94*g - 18*b + 128) >> 8) + 128;
            pu[i] = (uint8_t)(uu<0?0:(uu>255?255:uu));
            pv[i] = (uint8_t)(vv<0?0:(vv>255?255:vv));
        }
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s rtmp://server/app/streamkey\n", argv[0]);
        return 1;
    }
    const char* url = argv[1];

    int width=0, height=0;
    if (!capture_init(&width, &height)) {
        printf("capture_init failed\n");
        return 1;
    }
    printf("Capture size: %dx%d\n", width, height);

    int fps = 15;
    int bitrate_kbps = 600;
    if (!encoder_init(width, height, fps, bitrate_kbps)) {
        printf("encoder_init failed\n");
        capture_shutdown();
        return 1;
    }

    if (!rtmp_init(url)) {
        printf("rtmp connect failed\n");
        encoder_close();
        capture_shutdown();
        return 1;
    }

    uint8_t* y = malloc(width * height);
    uint8_t* u = malloc(width/2 * height/2);
    uint8_t* v = malloc(width/2 * height/2);

    printf("Starting capture loop\n");
    while (1) {
        uint8_t* bgra;
        int stride;
        if (!capture_next(&bgra, &stride)) {
            Sleep(5);
            continue;
        }

        bgra_to_i420(bgra, stride, width, height, y, u, v, width, width/2, width/2);

        uint8_t* out = NULL;
        int out_size = 0;
        encoder_encode_frame(y, u, v, width, width/2, width/2, &out, &out_size);
        if (out && out_size>0) {
            int key = (out[4] & 0x1f) == 5; // naive: NAL type 5 => IDR
            int64_t ts = now_ms();
            rtmp_send_h264(out, out_size, ts, key);
            free(out);
        }

        capture_release_frame();
        Sleep(1000 / fps);
    }

    rtmp_close();
    encoder_close();
    capture_shutdown();
    free(y); free(u); free(v);
    return 0;
}
