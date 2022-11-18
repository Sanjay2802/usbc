//
// Created by Hsj on 2022/1/14.
//

#include "Common.h"
#include "UsbCamera.h"
#include "libuvc/libuvc_internal.h"

#ifdef __cplusplus
extern "C" {
#endif

UsbCamera::UsbCamera() : fd(0), process(nullptr), status(STATUS_INIT_NONE) {
    LOGD("UsbCamera()");
}

UsbCamera::~UsbCamera() {
    LOGD("~UsbCamera()");
}

//==================================================================================================

bool is_frame = true;    
    
inline const StatusInfo UsbCamera::getStatus() const { return status; }

static void frame_callback(uvc_frame_t *frame, void *args) {
    if (is_frame) {
        is_frame = false;
        LOGD("frame_callback.");
    }
    auto *process = reinterpret_cast<FrameProcess *>(args);
    //LOGD("frame_callback: %p", process);
    //saveFile("/sdcard/A.jpg", frame->data, frame->data_bytes);

    //1. Decoder: function pointer
    process->decodeFrame((uint8_t *)frame->data, frame->data_bytes, process->buffer);
    //2. Render: function pointer
    process->renderFrame(process->buffer);
    //3. Send to Java
    JNIEnv *env;
    JavaVM *jvm = process->jvm;
    if (jvm != nullptr && JNI_OK == jvm->AttachCurrentThread(&env, nullptr)){
        env->CallVoidMethod(process->obj, process->mid);
        jvm->DetachCurrentThread();
    } else {
        LOGE("frameCallBack AttachCurrentThread failed.");
    }

}

//==================================================================================================

int UsbCamera::connectDevice(int _fd) {
    LOGD("connectDevice()");
    int ret = STATUS_SUCCESS;
    uvc_error_t uvc_ret;
    //1. init uvc
    if (getStatus() == STATUS_INIT_NONE) {
        uvc_ret = uvc_init_fd(&uvc_context, nullptr);
        LOGE("uvc_init(): %d", uvc_ret);
        if (uvc_ret == UVC_SUCCESS) {
            status = STATUS_INIT_OK;
        } else {
            LOGE("uvc_init() failed: %d", uvc_ret);
        }
    }
    //2. connect device
    if (getStatus() == STATUS_INIT_OK) {
        fd = dup(_fd);
        uvc_ret = uvc_find_device_opened(uvc_context, uvc_device, &uvc_device_handle, fd);
        if (uvc_ret == UVC_SUCCESS) {
            status = STATUS_OPEN;
            LOGE("uvc_init(): %d", status);


        } else {
            ::close(fd);
            ret = STATUS_EXE_FAILED;
            LOGE("connect: %d", uvc_ret);
        }
    } else {
        ret = STATUS_ERROR_STEP;
        LOGE("connect: status=%d", getStatus());
    }
    return ret;
}

int UsbCamera::openDevice(int vendorId, int productId, int bus_num, int dev_num) {
    LOGD("openDevice()");
    int ret = STATUS_SUCCESS;
    uvc_error_t uvc_ret;
    //1. init uvc
    if (getStatus() == STATUS_INIT_NONE) {
        uvc_ret = uvc_init(&uvc_context, nullptr);
        if (uvc_ret == UVC_SUCCESS) {
            status = STATUS_INIT_OK;
        } else {
            LOGE("uvc_init() failed: %d", uvc_ret);
        }
    }
    //2. find device
    if (getStatus() == STATUS_INIT_OK) {
        LOGE("bus=%d, dev=%d", bus_num, dev_num);
        uvc_ret = uvc_find_device2(uvc_context, &uvc_device, vendorId, productId, bus_num, dev_num);
        if (uvc_ret == UVC_SUCCESS) {
            //3. open device
            uvc_ret = uvc_open(uvc_device, &uvc_device_handle);
            if (uvc_ret == UVC_SUCCESS) {
                status = STATUS_OPEN;
            } else {
                LOGE("uvc_open: %d", uvc_ret);
                ret = STATUS_EXE_FAILED;
            }
        } else {
            LOGE("uvc_find_device() failed: %d", uvc_ret);
            ret = STATUS_EXE_FAILED;
        }
    } else {
        ret = STATUS_ERROR_STEP;
        LOGE("open: status=%d", getStatus());
    }
    return ret;
}

int UsbCamera::getSupportInfo(std::vector<SupportInfo> &supportInfo) {
    LOGD("getSupportInfo()");
    int ret = STATUS_SUCCESS;
    if (getStatus() == STATUS_OPEN) {
        if (uvc_device_handle) {
            uvc_device_info *device_info = uvc_device_handle->info;
            if (device_info) {
                uvc_streaming_interface_t *stream_info = device_info->stream_ifs;
                if (stream_info){
                    int stream_id = 0;
                    uvc_streaming_interface_t *info;
                    DL_FOREACH(stream_info, info) {
                        ++stream_id;
                        uvc_format_desc_t *fmt_desc;
                        uvc_frame_desc_t *frame_desc;
                        DL_FOREACH(info->format_descs, fmt_desc){
                            DL_FOREACH(fmt_desc->frame_descs, frame_desc){
                                uint32_t fps = 10000000 / frame_desc->dwDefaultFrameInterval;
                                SupportInfo si(fmt_desc->bDescriptorSubtype,frame_desc->wWidth,frame_desc->wHeight,fps);
                                supportInfo.emplace_back(si);
                            }
                        }
                    }
                } else {
                    ret = STATUS_EMPTY_DATA;
                    LOGE("getSupportSize: uvc_streaming_interface_t is null");
                }
            } else {
                ret = STATUS_EMPTY_DATA;
                LOGE("getSupportSize: uvc_device_info is null");
            }
        } else {
            ret = STATUS_EMPTY_OBJ;
            LOGE("getSupportSize: uvc_device_handle is null");
        }
    }else {
        ret = STATUS_ERROR_STEP;
        LOGE("getSupportFormat: status=%d", getStatus());
    }
    return ret;
}

int UsbCamera::setSupportInfo(SupportInfo &configInfo) {
    LOGD("setSupportInfo()");
    int ret = STATUS_SUCCESS;
    int _status = getStatus();
    if (_status == STATUS_OPEN || _status == STATUS_CONFIGURE) {
        if (uvc_device_handle) {
            // Config info
            enum uvc_frame_format frame_format = UVC_FRAME_FORMAT_YUYV;
            if (configInfo.format == UVC_VS_FORMAT_MJPEG) {
                frame_format = UVC_FRAME_FORMAT_MJPEG;
            } else if (configInfo.format == UVC_VS_FORMAT_FRAME_BASED) {
                frame_format = UVC_FRAME_FORMAT_H264;
            }
            uvc_error_t uvc_ret = uvc_get_stream_ctrl_format_size(uvc_device_handle, &ctrl,
                    frame_format, configInfo.width, configInfo.height, configInfo.fps);
            if (uvc_ret == UVC_SUCCESS) {
                status = STATUS_CONFIGURE;
            } else {
                ret = STATUS_EXE_FAILED;
                LOGE("uvc_get_stream_ctrl_format_size() failed: %d", uvc_ret);
            }
        } else {
            ret = STATUS_EMPTY_OBJ;
            LOGE("setSupportInfo: uvc_device_handle is null");
        }
    }else {
        ret = STATUS_ERROR_STEP;
        LOGE("setSupportInfo: status:%d", getStatus());
    }
    return ret;
}

int UsbCamera::setFrameProcess(FrameProcess *_process) {
    LOGD("setFrameProcess()");
    int ret = STATUS_SUCCESS;
    if(getStatus() == STATUS_CONFIGURE) {
        SAFE_DELETE(this->process);
        this->process = _process;
    } else {
        ret = STATUS_ERROR_STEP;
        LOGW("setCallback: status:%d", getStatus());
    }
    return ret;
}

int UsbCamera::setPreview(ANativeWindow *window) {
    LOGD("setPreview()");
    int ret = STATUS_SUCCESS;
    if(getStatus() == STATUS_CONFIGURE) {
        if (process != nullptr) {
            LOGD("startStream()..");
            process->setPreview(window);
        } else {
            ret = STATUS_ERROR_STEP;
        }
    } else {
        ret = STATUS_ERROR_STEP;
        LOGW("setPreview: status:%d", getStatus());
    }
    return ret;
}

int UsbCamera::startStream() {
    LOGD("startStream()");
    int ret = STATUS_SUCCESS;
    if(getStatus() == STATUS_CONFIGURE) {
        uvc_error_t uvc_ret = uvc_start_streaming(uvc_device_handle,
                &ctrl, frame_callback, process, 0x0);
        if (uvc_ret == UVC_SUCCESS){
            status = STATUS_RUN;
        } else {
            ret = STATUS_EXE_FAILED;
            LOGE("startStream: %d", uvc_ret);
        }
    } else {
        ret = STATUS_ERROR_STEP;
        LOGW("startStream: status:%d", getStatus());
    }
    return ret;
}

int UsbCamera::stopStream() {
    LOGD("stopStream()");
    if(getStatus() == STATUS_RUN){
        LOGD("stopStream()..");
        uvc_stop_streaming(uvc_device_handle);
        status = STATUS_CONFIGURE;
    } else {
        LOGW("stopStream: status:%d", getStatus());
    }
    return STATUS_SUCCESS;
}

int UsbCamera::closeDevice() {
    LOGD("closeDevice()");
    int ret = STATUS_SUCCESS;
    int _status = getStatus();
    if (_status == STATUS_OPEN || _status == STATUS_CONFIGURE){
        LOGD("closeDevice()..");
        if (uvc_device_handle != nullptr) {
            uvc_close(uvc_device_handle);
            uvc_device_handle = nullptr;
        }
        if (fd > 0) {
            close(fd);
            fd = 0;
        } else if (uvc_device != nullptr){
            uvc_unref_device(uvc_device);
            uvc_device = nullptr;
        }
        SAFE_DELETE(this->process);
        status = STATUS_INIT_OK;
    } else {
        LOGW("closeDevice: status:%d", getStatus());
    }
    return ret;
}

void UsbCamera::destroy() {
    LOGD("destroy()");
    if (getStatus() == STATUS_INIT_OK){
        LOGD("destroy()..");
        if (uvc_context != nullptr) {
            uvc_exit(uvc_context);
            uvc_context = nullptr;
        }
        status = STATUS_INIT_NONE;
    } else {
        LOGW("destroy: status:%d", getStatus());
    }
}

#ifdef __cplusplus
}  // extern "C"
#endif
