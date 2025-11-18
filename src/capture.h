#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// C interface for screen capture (Desktop Duplication)
// init returns 1 on success, fills width/height
int capture_init(int* width, int* height);

// capture_next returns 1 when a frame is available.
// It fills `out_bgra` with pointer to a BGRA buffer owned by the capture module
// and `out_stride` with bytes per row. The buffer is valid until capture_release_frame()
int capture_next(uint8_t** out_bgra, int* out_stride);

// release frame after processing
void capture_release_frame();

// shutdown and free resources
void capture_shutdown();

#ifdef __cplusplus
}
#endif
