//
// Created by Hsj on 2022/4/13.
//

#include <malloc.h>
#include "Common.h"
#include "FrameProcess.h"

#ifdef __cplusplus
extern "C" {
#endif

//==================================================================================================

FrameProcess::FrameProcess(jint width, jint height, jint format, jint pixel) :
        width(width), height(height), pixel(PixelFormat(pixel)), buffer_size(0),
        decode(nullptr), render(nullptr), preview(nullptr),
        jvm(nullptr), obj(nullptr), mid(nullptr){
    if (format == FRAME_FORMAT_YUY2) {
        if (pixel == PIXEL_FORMAT_RGB) {
            buffer_size = width * height * 3 ;
            buffer = (uint8_t *) malloc(buffer_size);
            assert(buffer!= nullptr);
            decode = &FrameDecoder::YUY2ToRGB;
            decoder = new FrameDecoder(width, height, FRAME_FORMAT_YUY2, PIXEL_FORMAT_RGB);
        } else if (pixel == PIXEL_FORMAT_NV21) {
            buffer_size = width * height * 3 / 2;
            buffer = (uint8_t *) malloc(buffer_size);
            assert(buffer!= nullptr);
            decode = &FrameDecoder::YUY2ToNV21;
            decoder = new FrameDecoder(width, height, FRAME_FORMAT_YUY2, PIXEL_FORMAT_NV21);
        } else if (pixel == PIXEL_FORMAT_YUY2){
            buffer_size = width * height * 2;
            buffer = (uint8_t *) malloc(buffer_size);
            assert(buffer!= nullptr);
            decode = &FrameDecoder::YUY2ToCopy;
            decoder = new FrameDecoder(width, height, FRAME_FORMAT_YUY2, PIXEL_FORMAT_YUY2);
        } else {
            LOGE("PixelFormat error: %d", pixel);
            assert(false);
        }
    } else if (format == FRAME_FORMAT_MJPG) {
        if (pixel == PIXEL_FORMAT_RGB) {
            buffer_size = width * height * 3 ;
            buffer = (uint8_t *) malloc(buffer_size);
            assert(buffer!= nullptr);
            decode = &FrameDecoder::MJPGToRGB;
            decoder = new FrameDecoder(width, height, FRAME_FORMAT_MJPG, PIXEL_FORMAT_RGB);
        } else if (pixel == PIXEL_FORMAT_NV21) {
            buffer_size = width * height * 3 / 2;
            buffer = (uint8_t *) malloc(buffer_size);
            assert(buffer!= nullptr);
            decode = &FrameDecoder::MJPGToNV21;
            decoder = new FrameDecoder(width, height, FRAME_FORMAT_MJPG, PIXEL_FORMAT_NV21);
        } else if (pixel == PIXEL_FORMAT_YUY2){
            buffer_size = width * height * 2;
            buffer = (uint8_t *) malloc(buffer_size);
            assert(buffer!= nullptr);
            decode = &FrameDecoder::MJPGToYUY2;
            decoder = new FrameDecoder(width, height, FRAME_FORMAT_MJPG, PIXEL_FORMAT_YUY2);
        } else {
            LOGE("PixelFormat error: %d", pixel);
            assert(false);
        }
    } else if (format == FRAME_FORMAT_H264) {
        if (pixel == PIXEL_FORMAT_RGB) {
            buffer_size = width * height * 3 ;
            buffer = (uint8_t *) malloc(buffer_size);
            assert(buffer!= nullptr);
            decode = &FrameDecoder::H264ToRGB;
            decoder = new FrameDecoder(width, height, FRAME_FORMAT_H264, PIXEL_FORMAT_RGB);
        } else if (pixel == PIXEL_FORMAT_NV21) {
            buffer_size = width * height * 3 / 2;
            buffer = (uint8_t *) malloc(buffer_size);
            assert(buffer!= nullptr);
            decode = &FrameDecoder::H264ToNV21;
            decoder = new FrameDecoder(width, height, FRAME_FORMAT_H264, PIXEL_FORMAT_NV21);
        } else if (pixel == PIXEL_FORMAT_YUY2){
            buffer_size = width * height * 2;
            buffer = (uint8_t *) malloc(buffer_size);
            assert(buffer!= nullptr);
            decode = &FrameDecoder::H264ToYUY2;
            decoder = new FrameDecoder(width, height, FRAME_FORMAT_H264, PIXEL_FORMAT_YUY2);
        } else {
            LOGE("PixelFormat error: %d", pixel);
            assert(false);
        }
    } else {
        LOGE("Frame format error: %d", format);
        assert(false);
    }
}

FrameProcess::~FrameProcess() {
    decode = nullptr;
    render = nullptr;
    SAFE_FREE(buffer);
    SAFE_DELETE(decoder);
    SAFE_DELETE(preview);
    if (jvm != nullptr && obj != nullptr) {
        JNIEnv *env;
        if (JNI_OK == jvm->GetEnv((void **) &env, JNI_VERSION_1_6)) {
            env->DeleteGlobalRef(obj);
        }
        obj = nullptr;
        mid = nullptr;
    }
}

//==================================================================================================

int FrameProcess::renderFrame(uint8_t *src) {
    if (preview != nullptr) {
        return (preview->*render)(src);
    } else {
        //LOGW("renderFrame failed.");
        return STATUS_EXE_FAILED;
    }
}

int FrameProcess::setPreview(ANativeWindow *window) {
    int ret = STATUS_SUCCESS;
    SAFE_DELETE(preview);
    if (window == nullptr) {
        LOGI("setPreview: window is nullptr");
    } else if (pixel == PIXEL_FORMAT_RGB) {
        render = &IPreview::renderRGB;
        this->preview = new NativePreview(width, height, pixel, window);
    } else if (pixel == PIXEL_FORMAT_NV21) {
        render = &IPreview::renderNV21;
        this->preview = new NativePreview(width, height, pixel, window);
    } else if (pixel == PIXEL_FORMAT_YUY2) {
        render = &IPreview::renderYUY2;
        this->preview = new NativePreview(width, height, pixel, window);
    } else {
        LOGE("PixelFormat error: %d", pixel);
        ret = STATUS_EXE_FAILED;
    }
    return ret;
}

int FrameProcess::decodeFrame(uint8_t *src, size_t size, uint8_t *dst) {
    if (buffer_size != 0) {
        return (decoder->*decode)(src, size, dst);
    } else {
        LOGW("decodeFrame failed.");
        return STATUS_NONE_INIT;
    }
}

int FrameProcess::setJavaObj(JavaVM *_jvm, jobject _obj, jmethodID _mid) {
    if (this->jvm != nullptr) {
        JNIEnv *env;
        if (JNI_OK == this->jvm->GetEnv((void **) &env, JNI_VERSION_1_6)) {
            env->DeleteGlobalRef(this->obj);
            this->obj = nullptr;
            this->mid = nullptr;
        }
    }
    this->jvm = _jvm;
    this->obj = _obj;
    this->mid = _mid;
    return STATUS_SUCCESS;
}

#ifdef __cplusplus
}  // extern "C"
#endif
