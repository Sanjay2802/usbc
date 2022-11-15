//
// Created by Hsj on 2022/4/12.
//

#ifndef ANDROID_USB_CAMERA_FRAMEDECODER_H
#define ANDROID_USB_CAMERA_FRAMEDECODER_H

#include <cassert>
#include <unistd.h>
#include <turbojpeg.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    FRAME_FORMAT_UNKNOWN    = 0x0,  //UNKNOWN
    FRAME_FORMAT_YUY2       = 0x04, //YUY2.
    FRAME_FORMAT_MJPG       = 0x06, //MJPEG.
    FRAME_FORMAT_H264       = 0x10, //H264.
} FrameFormat;

typedef enum {
    PIXEL_FORMAT_UNKNOWN    = 0x00, //UNKNOWN
    PIXEL_FORMAT_YUY2       = 0x04, //YUY2: Y U Y V.
    PIXEL_FORMAT_NV21       = 0x0A, //YUV420P: Y U V
    PIXEL_FORMAT_RGB        = 0x0B, //RGB24: R8G8B8
} PixelFormat;

class FrameDecoder {
private:
    uint16_t width;
    uint16_t height;
    uint8_t *buffer;

    //YUY2
    uint16_t stride_uv;
    uint32_t stride_rgb;
    uint32_t stride_yuy2;
    uint32_t length_y;
    uint32_t length_yuy2;
    uint8_t *buffer_uv;

    //MJPEG
    int _width;
    int _height;
    int sub_sample;
    int color_space;
    tjhandle handle;
    uint16_t stride_u;
    uint16_t stride_v;
    uint8_t *buffer_u;
    uint8_t *buffer_v;

    //H264...

public:
    FrameDecoder(uint16_t width, uint16_t height, FrameFormat format, PixelFormat pixel);
    ~FrameDecoder();

    int YUY2ToRGB(uint8_t *src, size_t size, uint8_t *dst);
    int YUY2ToNV21(uint8_t *src, size_t size, uint8_t *dst);
    int YUY2ToCopy(uint8_t *src, size_t size, uint8_t *dst);

    int MJPGToRGB(uint8_t *src, size_t size, uint8_t *dst);
    int MJPGToNV21(uint8_t *src, size_t size, uint8_t *dst);
    int MJPGToYUY2(uint8_t *src, size_t size, uint8_t *dst);

    int H264ToRGB(uint8_t *src, size_t size, uint8_t *dst);
    int H264ToNV21(uint8_t *src, size_t size, uint8_t *dst);
    int H264ToYUY2(uint8_t *src, size_t size, uint8_t *dst);
};

#ifdef __cplusplus
}  // extern "C"
#endif

#endif //ANDROID_USB_CAMERA_FRAMEDECODER_H
