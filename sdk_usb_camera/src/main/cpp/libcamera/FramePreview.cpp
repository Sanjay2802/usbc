//
// Created by Hsj on 2022/4/13.
//

#include <cstring>
#include <malloc.h>
#include "libyuv.h"
#include "Common.h"
#include "FramePreview.h"

#ifdef __cplusplus
extern "C" {
#endif

// Render by OpenGL ES

GLESPreview::GLESPreview(uint16_t width, uint16_t height, PixelFormat format, ANativeWindow *window) : window(window) {
}

GLESPreview::~GLESPreview() {
}

int GLESPreview::pause() {
    return STATUS_EXE_FAILED;
}

int GLESPreview::renderRGB(uint8_t *data) {
    return STATUS_EXE_FAILED;
}

int GLESPreview::renderNV21(uint8_t *data) {
    return STATUS_EXE_FAILED;
}

int GLESPreview::renderYUY2(uint8_t *data) {
    return STATUS_EXE_FAILED;
}

//==================================================================================================

// Render by ANativeWindow

NativePreview::NativePreview(uint16_t width, uint16_t height, PixelFormat format, ANativeWindow *window) :
    width(width), height(height), stride_argb(width * 4), window(window) {
    if (format == PIXEL_FORMAT_RGB) {
        stride_rgb = width * 3;
    } else if (format == PIXEL_FORMAT_NV21) {
        length_y  = width * height;
        stride_vu = width / 2;
    } else if (format == PIXEL_FORMAT_YUY2) {
        stride_yuy2 = width * 2;
    } else {
        LOGE("PixelFormat error: %d", format);
        assert(false);
    }
    ANativeWindow_setBuffersGeometry(window, width, height, WINDOW_FORMAT_RGBA_8888);
}

NativePreview::~NativePreview() {
    if (window) {
        ANativeWindow_release(window);
        window = nullptr;
    }
}

int NativePreview::pause() {
    ANativeWindow_Buffer buffer;
    if (LIKELY(STATUS_SUCCESS == ANativeWindow_lock(window, &buffer, nullptr))) {
        auto *dst = (uint8_t *) buffer.bits;
        const int32_t size_line = buffer.width * 4;
        const int32_t size_stride = buffer.stride * 4;
        for (int32_t i = 0; i < buffer.height; i++) {
            memset(dst, 0, size_line);
            dst += size_stride;
        }
        ANativeWindow_unlockAndPost(window);
        return STATUS_SUCCESS;
    } else {
        return STATUS_EXE_FAILED;
    }
}

int NativePreview::renderRGB(uint8_t *data) {
    ANativeWindow_Buffer buffer;
    if (LIKELY(STATUS_SUCCESS == ANativeWindow_lock(window, &buffer, nullptr))) {
        libyuv::RGB24ToARGB(data, stride_rgb, (uint8_t *) buffer.bits, stride_argb, width, height);
        ANativeWindow_unlockAndPost(window);
        return STATUS_SUCCESS;
    } else {
        return STATUS_EXE_FAILED;
    }
}

int NativePreview::renderNV21(uint8_t *data) {
    ANativeWindow_Buffer buffer;
    if (LIKELY(STATUS_SUCCESS == ANativeWindow_lock(window, &buffer, nullptr))) {
        uint8_t *src_vu = (data + length_y);
        libyuv::NV21ToABGR(data, width, src_vu, stride_vu, (uint8_t *) buffer.bits, stride_argb, width, height);
        ANativeWindow_unlockAndPost(window);
        return STATUS_SUCCESS;
    } else {
        return STATUS_EXE_FAILED;
    }
}

int NativePreview::renderYUY2(uint8_t *data) {
    ANativeWindow_Buffer buffer;
    if (LIKELY(STATUS_SUCCESS == ANativeWindow_lock(window, &buffer, nullptr))) {
        libyuv::YUY2ToARGB(data, stride_yuy2, (uint8_t *) buffer.bits, stride_argb, width, height);
        ANativeWindow_unlockAndPost(window);
        return STATUS_SUCCESS;
    } else {
        return STATUS_EXE_FAILED;
    }
}

#ifdef __cplusplus
}  // extern "C"
#endif
