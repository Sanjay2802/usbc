//
// Created by Hsj on 2022/4/12.
//

#include <cstring>
#include "Common.h"
#include "libyuv.h"
#include "FrameDecoder.h"

#ifdef __cplusplus
extern "C" {
#endif

FrameDecoder::FrameDecoder(uint16_t width, uint16_t height, FrameFormat format, PixelFormat pixel):
    width(width), height(height) {

    if (format == FRAME_FORMAT_MJPG){
        if (pixel == PIXEL_FORMAT_RGB){
            buffer = (uint8_t *) malloc(width * height * 2);
            assert(buffer != nullptr);
            handle = tjInitDecompress();
        } else if (pixel == PIXEL_FORMAT_NV21 || pixel == PIXEL_FORMAT_YUY2){
            length_y = width * height;
            buffer = (uint8_t *) malloc(length_y * 2);
            assert(buffer != nullptr);
            handle = tjInitDecompress();
            stride_yuy2 = width * 2;
            buffer_u = buffer + length_y;
            buffer_v = buffer_u + length_y / 2;
            stride_uv = stride_u = stride_v = width / 2;
        } else {
            LOGE("PixelFormat error: %d", pixel);
            assert(false);
        }
    } else if (format == FRAME_FORMAT_YUY2){
        if (pixel == PIXEL_FORMAT_RGB || pixel == PIXEL_FORMAT_NV21) {
            length_y    = width * height;
            buffer = (uint8_t *) malloc(length_y * 3 / 2);
            assert(buffer != nullptr);
            stride_uv   = width / 2;
            stride_rgb  = width * 3;
            stride_yuy2 = width * 2;
            buffer_uv = buffer + length_y;
        } else if (pixel == PIXEL_FORMAT_YUY2){
            stride_yuy2 = width * 2;
            length_yuy2 = width * height * 2;
            LOGD("format=%d, pixel=%d.", format, pixel);
        } else {
            LOGE("PixelFormat error: %d", pixel);
            assert(false);
        }
    } else if (format == FRAME_FORMAT_H264){
        if (pixel == PIXEL_FORMAT_RGB){
            LOGE("format=%d, pixel=%d decode not implemented.", format, pixel);
        } else if (pixel == PIXEL_FORMAT_NV21){
            LOGE("format=%d, pixel=%d decode not implemented.", format, pixel);
        } else if (pixel == PIXEL_FORMAT_YUY2){
            LOGE("format=%d, pixel=%d decode not implemented.", format, pixel);
        } else {
            LOGE("PixelFormat error: %d", pixel);
            assert(false);
        }
    } else {
        LOGE("FrameFormat error: %d", format);
        assert(false);
    }
}

FrameDecoder::~FrameDecoder() {
    SAFE_FREE(buffer);
    if (handle) {
        tjDestroy(handle);
        handle = nullptr;
    }
}

//==========================================YUY2====================================================

int FrameDecoder::YUY2ToRGB(uint8_t *src, size_t size, uint8_t *dst){
    int ret = libyuv::YUY2ToNV12(src, stride_yuy2, buffer, width, buffer_uv, stride_uv, width, height);
    if (STATUS_SUCCESS == ret){
        ret = libyuv::NV12ToRGB24(buffer, width, buffer_uv, stride_uv, dst, stride_rgb, width, height);
    }
    return ret;
}

int FrameDecoder::YUY2ToNV21(uint8_t *src, size_t size, uint8_t *dst){
    int ret = libyuv::YUY2ToNV12(src, stride_yuy2, buffer, width, buffer_uv, stride_uv, width, height);
    if (STATUS_SUCCESS == ret){
        uint8_t *dst_uv = dst + length_y;
        ret = libyuv::NV21ToNV12(buffer, width, buffer_uv, stride_uv, 
                dst, width, dst_uv, width, width, height);
    }
    return ret;
}

int FrameDecoder::YUY2ToCopy(uint8_t *src, size_t size, uint8_t *dst){
    memcpy(dst, src, length_yuy2);
    /*for (int h = 0; h < height; ++h) {
        libyuv::CopyPlane(src, stride_yuy2, dst, stride_yuy2, width, 1);
        src += width;
        dst += width;
    }*/
    return STATUS_SUCCESS;
}

//=========================================MJPEG====================================================

int FrameDecoder::MJPGToRGB(uint8_t *src, size_t size, uint8_t *dst){
    int ret = tjDecompressHeader3(handle, src, size, &_width, &_height, &sub_sample, &color_space);
    if (STATUS_SUCCESS == ret) {
        ret = tjDecompress2(handle, src, size, dst, _width, _width * 3, _height, TJPF_RGB, TJFLAG_FASTDCT);
    } else {
        memcpy(dst, src, size);
    }
    return ret;
}

int FrameDecoder::MJPGToNV21(uint8_t *src, size_t size, uint8_t *dst){
    int ret = tjDecompressHeader3(handle, src, size, &_width, &_height, &sub_sample, &color_space);
    if (STATUS_SUCCESS == ret) {
        ret = tjDecompressToYUV2(handle, src, size, buffer, _width, 4, _height, TJFLAG_FASTDCT);
        if (STATUS_SUCCESS == ret) {
            uint8_t *dst_uv = dst + length_y;
            ret = libyuv::I422ToNV21(buffer, width, buffer_u, stride_u, buffer_v, stride_v,
                                     dst, width, dst_uv, width, width, height);
        }
    }
    return ret;
}

int FrameDecoder::MJPGToYUY2(uint8_t *src, size_t size, uint8_t *dst){
    int ret = tjDecompressHeader3(handle, src, size, &_width, &_height, &sub_sample, &color_space);
    if (STATUS_SUCCESS == ret) {
        ret = tjDecompressToYUV2(handle, src, size, buffer, _width, 4, _height, TJFLAG_FASTDCT);
        if (STATUS_SUCCESS == ret) {
            ret = libyuv::I422ToYUY2(buffer, width, buffer_v, stride_v, buffer_u, stride_u,
                                     dst, stride_yuy2, width, height);
        }
    }
    return ret;
}

//===========================================H264===================================================

int FrameDecoder::H264ToRGB(uint8_t *src, size_t size, uint8_t *dst){
    return STATUS_EXE_FAILED;
}

int FrameDecoder::H264ToNV21(uint8_t *src, size_t size, uint8_t *dst){
    return STATUS_EXE_FAILED;
}

int FrameDecoder::H264ToYUY2(uint8_t *src, size_t size, uint8_t *dst){
    return STATUS_EXE_FAILED;
}

//==================================================================================================

#ifdef __cplusplus
}  // extern "C"
#endif