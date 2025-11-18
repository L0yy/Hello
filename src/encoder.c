#include "encoder.h"
#include <x264.h>
#include <stdlib.h>
#include <string.h>

static x264_t* g_enc = NULL;
static x264_picture_t g_pic_in;
static x264_param_t g_param;

int encoder_init(int width, int height, int fps, int bitrate_kbps) {
    x264_param_default_preset(&g_param, "veryfast", "zerolatency");
    g_param.i_csp = X264_CSP_I420;
    g_param.i_width = width;
    g_param.i_height = height;
    g_param.i_fps_num = fps;
    g_param.i_fps_den = 1;
    g_param.b_vfr_input = 0;
    g_param.rc.i_bitrate = bitrate_kbps;
    g_param.i_threads = 1;
    x264_param_apply_profile(&g_param, "baseline");

    g_enc = x264_encoder_open(&g_param);
    if (!g_enc) return 0;

    x264_picture_alloc(&g_pic_in, X264_CSP_I420, width, height);
    return 1;
}

int encoder_encode_frame(const uint8_t* y, const uint8_t* u, const uint8_t* v,
                         int stride_y, int stride_u, int stride_v,
                         uint8_t** out_buf, int* out_size) {
    if (!g_enc) return 0;
    int w = g_param.i_width;
    int h = g_param.i_height;

    // copy planes into x264 picture (x264 uses packed planes with plane pointers)
    for (int i = 0; i < h; i++) {
        memcpy(g_pic_in.img.plane[0] + i * g_pic_in.img.i_stride[0], y + i * stride_y, w);
    }
    for (int i = 0; i < h/2; i++) {
        memcpy(g_pic_in.img.plane[1] + i * g_pic_in.img.i_stride[1], u + i * stride_u, w/2);
        memcpy(g_pic_in.img.plane[2] + i * g_pic_in.img.i_stride[2], v + i * stride_v, w/2);
    }

    x264_nal_t* p_nal;
    int i_nal;
    x264_picture_t pic_out;
    int frame_size = x264_encoder_encode(g_enc, &p_nal, &i_nal, &g_pic_in, &pic_out);
    if (frame_size < 0) return 0;
    if (frame_size == 0) {
        *out_buf = NULL;
        *out_size = 0;
        return 1; // no output this frame
    }

    // allocate buffer and concatenate NALs
    int total = 0;
    for (int i = 0; i < i_nal; i++) total += p_nal[i].i_payload;
    uint8_t* buf = (uint8_t*)malloc(total);
    int off = 0;
    for (int i = 0; i < i_nal; i++) {
        memcpy(buf + off, p_nal[i].p_payload, p_nal[i].i_payload);
        off += p_nal[i].i_payload;
    }
    *out_buf = buf;
    *out_size = total;
    return 1;
}

void encoder_close() {
    if (g_enc) {
        x264_encoder_close(g_enc);
        g_enc = NULL;
    }
    x264_picture_clean(&g_pic_in);
}
