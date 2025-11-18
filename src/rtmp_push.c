#include "rtmp_push.h"
#include <librtmp/rtmp.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

static RTMP* rtmp = NULL;

int rtmp_init(const char* url) {
    rtmp = RTMP_Alloc();
    RTMP_Init(rtmp);
    if (!RTMP_SetupURL(rtmp, (char*)url)) return 0;
    RTMP_EnableWrite(rtmp);
    if (!RTMP_Connect(rtmp, NULL)) return 0;
    if (!RTMP_ConnectStream(rtmp, 0)) return 0;
    return 1;
}

// This implementation sends raw H.264 NAL data as an FLV video packet
// For production you must send AVC sequence header (SPS/PPS) before frames.
int rtmp_send_h264(uint8_t* data, int size, int64_t timestamp_ms, int is_keyframe) {
    if (!rtmp) return 0;
    RTMPPacket packet;
    RTMPPacket_Alloc(&packet, size + 9);
    RTMPPacket_Reset(&packet);
    packet.m_nChannel = 0x04; // video
    packet.m_nInfoField2 = rtmp->m_stream_id;
    packet.m_headerType = RTMP_PACKET_SIZE_LARGE;

    // Simple FLV VideoTag header for AVC NALU (not full spec compliant)
    uint8_t* body = (uint8_t*)packet.m_body;
    int i = 0;
    body[i++] = (is_keyframe ? 0x17 : 0x27); // FrameType & CodecID
    body[i++] = 0x01; // AVC NALU
    body[i++] = 0x00; // composition time
    body[i++] = 0x00;
    body[i++] = 0x00;

    memcpy(body + i, data, size);
    packet.m_nBodySize = size + i;
    packet.m_hasAbsTimestamp = 0;
    packet.m_nTimeStamp = (uint32_t)timestamp_ms;
    packet.m_packetType = RTMP_PACKET_TYPE_VIDEO;

    int ret = RTMP_SendPacket(rtmp, &packet, 0);
    RTMPPacket_Free(&packet);
    return ret;
}

void rtmp_close() {
    if (rtmp) {
        RTMP_Close(rtmp);
        RTMP_Free(rtmp);
        rtmp = NULL;
    }
}
