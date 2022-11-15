//
// Created by Hsj on 2022/4/13.
//

#ifndef ANDROID_USB_CAMERA_FRAMEPREVIEW_H
#define ANDROID_USB_CAMERA_FRAMEPREVIEW_H

#include <android/native_window_jni.h>
#include "FrameDecoder.h"

typedef uint16_t DepthPixel;

#ifdef __cplusplus
extern "C" {
#endif

class IPreview {
public:
    virtual ~IPreview() = default;
    virtual int pause() = 0;
    virtual int renderRGB(uint8_t *data) = 0;
    virtual int renderNV21(uint8_t *data) = 0;
    virtual int renderYUY2(uint8_t *data) = 0;
};

class GLESPreview : public IPreview {
private:
    ANativeWindow *window;
public:
    GLESPreview(uint16_t width, uint16_t height, PixelFormat format, ANativeWindow *window);
    ~GLESPreview();
    int pause();
    int renderRGB(uint8_t *data);
    int renderNV21(uint8_t *data);
    int renderYUY2(uint8_t *data);
};

class NativePreview : public IPreview{
private:
    uint16_t width;
    uint16_t height;
    uint32_t length_y;
    uint16_t stride_vu;
    uint32_t stride_rgb;
    uint32_t stride_argb;
    uint32_t stride_yuy2;
    ANativeWindow *window;
public:
    NativePreview(uint16_t width, uint16_t height, PixelFormat format, ANativeWindow *window);
    ~NativePreview();
    int pause();
    int renderRGB(uint8_t *data);
    int renderNV21(uint8_t *data);
    int renderYUY2(uint8_t *data);
};

#ifdef __cplusplus
}  // extern "C"
#endif

#endif //ANDROID_USB_CAMERA_FRAMEPREVIEW_H
