#include "capture.h"
#include <windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl.h>
#include <stdint.h>
#include <stdio.h>

using Microsoft::WRL::ComPtr;

static ComPtr<ID3D11Device> g_device;
static ComPtr<ID3D11DeviceContext> g_context;
static ComPtr<IDXGIOutputDuplication> g_duplication;
static int g_width = 0, g_height = 0;
static uint8_t* g_last_frame = nullptr;
static int g_last_stride = 0;

int capture_init(int* width, int* height) {
    HRESULT hr = S_OK;
    D3D_FEATURE_LEVEL featureLevel;
    hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
                           nullptr, 0, D3D11_SDK_VERSION, &g_device, &featureLevel, &g_context);
    if (FAILED(hr)) return 0;

    ComPtr<IDXGIDevice> dxgiDevice;
    hr = g_device.As(&dxgiDevice);
    if (FAILED(hr)) return 0;

    ComPtr<IDXGIAdapter> adapter;
    hr = dxgiDevice->GetAdapter(&adapter);
    if (FAILED(hr)) return 0;

    ComPtr<IDXGIOutput> output;
    hr = adapter->EnumOutputs(0, &output);
    if (FAILED(hr)) return 0;

    ComPtr<IDXGIOutput1> output1;
    hr = output.As(&output1);
    if (FAILED(hr)) return 0;

    DXGI_OUTDUPL_DESC desc;
    ComPtr<IDXGIOutputDuplication> duplication;

    hr = output1->DuplicateOutput(g_device.Get(), &g_duplication);
    if (FAILED(hr)) return 0;

    // get desktop coordinates
    DXGI_OUTPUT_DESC outDesc;
    output->GetDesc(&outDesc);
    RECT r = outDesc.DesktopCoordinates;
    g_width = r.right - r.left;
    g_height = r.bottom - r.top;

    *width = g_width;
    *height = g_height;
    return 1;
}

int capture_next(uint8_t** out_bgra, int* out_stride) {
    if (!g_duplication) return 0;
    DXGI_OUTDUPL_FRAME_INFO frameInfo;
    ComPtr<IDXGIResource> desktopResource;
    HRESULT hr = g_duplication->AcquireNextFrame(100, &frameInfo, &desktopResource);
    if (FAILED(hr)) return 0;

    ComPtr<ID3D11Texture2D> acquiredTex;
    hr = desktopResource.As(&acquiredTex);
    if (FAILED(hr)) {
        g_duplication->ReleaseFrame();
        return 0;
    }

    // copy to a staging texture we can map
    D3D11_TEXTURE2D_DESC desc;
    acquiredTex->GetDesc(&desc);
    desc.Usage = D3D11_USAGE_STAGING;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.BindFlags = 0;
    desc.MiscFlags = 0;

    ComPtr<ID3D11Texture2D> staging;
    HRESULT hr2 = g_device->CreateTexture2D(&desc, nullptr, &staging);
    if (FAILED(hr2)) {
        g_duplication->ReleaseFrame();
        return 0;
    }

    g_context->CopyResource(staging.Get(), acquiredTex.Get());

    D3D11_MAPPED_SUBRESOURCE mapped;
    hr2 = g_context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped);
    if (FAILED(hr2)) {
        g_duplication->ReleaseFrame();
        return 0;
    }

    int stride = mapped.RowPitch;
    size_t size = stride * g_height;
    if (!g_last_frame) g_last_frame = (uint8_t*)malloc(size);
    else g_last_frame = (uint8_t*)realloc(g_last_frame, size);

    memcpy(g_last_frame, mapped.pData, size);
    g_last_stride = stride;

    g_context->Unmap(staging.Get(), 0);
    g_duplication->ReleaseFrame();

    *out_bgra = g_last_frame;
    *out_stride = g_last_stride;
    return 1;
}

void capture_release_frame() {
    // nothing for now; buffer remains until next frame
}

void capture_shutdown() {
    if (g_last_frame) { free(g_last_frame); g_last_frame = nullptr; }
    g_duplication.Reset();
    g_context.Reset();
    g_device.Reset();
}
