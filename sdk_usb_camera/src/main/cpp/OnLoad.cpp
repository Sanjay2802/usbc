//
// Created by Hsj on 2022/1/13.
//

#include "Common.h"
#include "libcamera/UsbCamera.h"

#define OBJECT_ID "nativeObj"
#define USB_CAMERA  "com/hsj/camera/UsbCamera"

typedef jlong CAMERA_ID;

static void setFieldLong(JNIEnv *env, jobject obj, const char *fieldName, jlong value) {
    jclass clazz = env->GetObjectClass(obj);
    jfieldID field = env->GetFieldID(clazz, fieldName, "J");
    if (LIKELY(field)) {
        env->SetLongField(obj, field, value);
    } else {
        LOGE("Failed to found %s", fieldName);
    }
    env->DeleteLocalRef(clazz);
}

//=======================================UsbCamera==================================================

static CAMERA_ID nativeInit(JNIEnv *env, jobject thiz) {
    auto *camera = new UsbCamera();
    auto cameraId = reinterpret_cast<CAMERA_ID>(camera);
    setFieldLong(env, thiz, OBJECT_ID, cameraId);
    return cameraId;
}

static jint nativeConnect(JNIEnv *env, jobject thiz, CAMERA_ID cameraId, jint fd) {
    auto *camera = reinterpret_cast<UsbCamera *>(cameraId);
    jint ret = STATUS_NONE_INIT;
    if (camera) {
        ret = camera->connectDevice(fd);
    } else {
        LOGE("open: UsbCamera had been release.");
    }
    return ret;
}

static jint nativeOpen(JNIEnv *env, jobject thiz, CAMERA_ID cameraId, jint vid, jint pid, jint bus, jint dev) {
    auto *camera = reinterpret_cast<UsbCamera *>(cameraId);
    jint ret = STATUS_NONE_INIT;
    if (camera) {
        ret = camera->openDevice(vid, pid, bus, dev);
    } else {
        LOGE("open: UsbCamera had been release.");
    }
    return ret;
}

static jint nativeGetSupportInfo(JNIEnv *env, jobject thiz, CAMERA_ID cameraId, jobject supportInfos) {
    jint ret = STATUS_NONE_INIT;
    auto *camera = reinterpret_cast<UsbCamera *>(cameraId);
    if (camera) {
        if(supportInfos){
            std::vector<SupportInfo> infos;
            ret = camera->getSupportInfo(infos);
            if (ret == STATUS_SUCCESS) {
                if (!infos.empty()) {
                    jclass cls = env->GetObjectClass(supportInfos);
                    jmethodID mid = env->GetMethodID(cls,"add","(Ljava/lang/Object;)Z");
                    jclass cls2 = env->FindClass("com/hsj/camera/UsbCamera$SupportInfo");
                    jmethodID mid2 = env->GetMethodID(cls2, "<init>","(IIII)V");
                    for(auto & info : infos) {
                        jobject obj = env->NewObject(cls2, mid2, (int)info.format, info.width, info.height, info.fps);
                        env->CallBooleanMethod(supportInfos, mid, obj);
                        env->DeleteLocalRef(obj);
                    }
                } else{
                    ret = STATUS_EMPTY_DATA;
                    LOGE("getSupportInfo: supportInfo empty");
                }
            } else {
                ret = STATUS_EXE_FAILED;
                LOGE("getSupportInfo: failed: %d.", ret);
            }
        } else {
            ret = STATUS_EMPTY_PARAM;
            LOGE("getSupportInfo: supportInfos is null.");
        }
    } else {
        LOGE("getSupportSize: UsbCamera had been release.");
    }
    return ret;
}

static jint nativeSetSupportInfo(JNIEnv *env, jobject thiz, CAMERA_ID cameraId, jobject supportInfo) {
    jint ret = STATUS_NONE_INIT;
    auto *camera = reinterpret_cast<UsbCamera *>(cameraId);
    if (camera) {
        if (supportInfo){
            jclass cls = env->GetObjectClass(supportInfo);
            jfieldID fId1 = env->GetFieldID(cls, "format","I");
            jfieldID fId2 = env->GetFieldID(cls, "width", "I");
            jfieldID fId3 = env->GetFieldID(cls, "height","I");
            jfieldID fId4 = env->GetFieldID(cls, "fps",   "I");
            jfieldID fId5 = env->GetFieldID(cls, "pixel", "I");
            SupportInfo info(env->GetIntField(supportInfo, fId1),env->GetIntField(supportInfo, fId2),
                             env->GetIntField(supportInfo, fId3),env->GetIntField(supportInfo, fId4));
            ret = camera->setSupportInfo(info);
            if(STATUS_SUCCESS == ret){
                jint pixel = env->GetIntField(supportInfo, fId5);
                auto *process = new FrameProcess(info.width, info.height, info.format, pixel);
                if(process->buffer_size != 0){
                    jclass cls2 = env->GetObjectClass(thiz);
                    //ByteBuffer
                    jfieldID fId = env->GetFieldID(cls2, "frame", "Ljava/nio/ByteBuffer;");
                    jobject buffer = env->NewDirectByteBuffer(process->buffer, process->buffer_size);
                    env->SetObjectField(thiz, fId, buffer);
                    //updateFrame
                    jmethodID mid = env->GetMethodID(cls2, "updateFrame", "()V");
                    jobject obj = env->NewGlobalRef(thiz);
                    //setCallback
                    process->setJavaObj(getJVM(), obj, mid);
                } else {
                    LOGW("setSupportInfo: pixel:%d, frame not be send to java.", pixel);
                }
                camera->setFrameProcess(process);
            } else {
                LOGE("setSupportInfo: failed:%d", ret);
            }
        } else {
            ret = STATUS_EMPTY_PARAM;
            LOGE("setSupportInfo: configInfo is null.");
        }
    } else {
        LOGE("setSupportInfo: UsbCamera had been release.");
    }
    return ret;
}

static jint nativePreview(JNIEnv *env, jobject thiz, CAMERA_ID cameraId, jobject surface) {
    auto *camera = reinterpret_cast<UsbCamera *>(cameraId);
    jint ret = STATUS_NONE_INIT;
    if (camera) {
        ANativeWindow* window = nullptr;
        if (surface!= nullptr){
            window = ANativeWindow_fromSurface(env, surface);
        } else {
            LOGW("setPreview: surface is null.");
        }
        ret = camera->setPreview(window);
    } else {
        LOGE("open: UsbCamera had been release.");
    }
    return ret;
}

static jint nativeStart(JNIEnv *env, jobject thiz, CAMERA_ID cameraId) {
    auto *camera = reinterpret_cast<UsbCamera *>(cameraId);
    jint ret = STATUS_NONE_INIT;
    if (camera) {
        ret = camera->startStream();
    } else {
        LOGE("start: UsbCamera had been release.");
    }
    return ret;
}

static jint nativeStop(JNIEnv *env, jobject thiz, CAMERA_ID cameraId) {
    auto *camera = reinterpret_cast<UsbCamera *>(cameraId);
    jint ret = STATUS_NONE_INIT;
    if (camera) {
        ret = camera->stopStream();
    } else {
        LOGE("stop: UsbCamera had been release.");
    }
    return ret;
}

static jint nativeClose(JNIEnv *env, jobject thiz, CAMERA_ID cameraId) {
    auto *camera = reinterpret_cast<UsbCamera *>(cameraId);
    jint ret = STATUS_SUCCESS;
    if (camera) {
        ret = camera->closeDevice();
    } else {
        LOGE("close: UsbCamera had been release.");
    }
    return ret;
}

static void nativeDestroy(JNIEnv *env, jobject thiz, CAMERA_ID cameraId) {
    auto *camera = reinterpret_cast<UsbCamera *>(cameraId);
    if (camera) {
        camera->destroy();
        setFieldLong(env, thiz, OBJECT_ID, 0);
        SAFE_DELETE(camera);
    } else {
        LOGW("destroy: UsbCamera had been release.");
    }
}

static const JNINativeMethod USB_CAMERA_METHODS[] = {
    {"nativeInit",          "()J",                                         (void *) nativeInit},
    {"nativeConnect",       "(JI)I",                                       (void *) nativeConnect},
    {"nativeOpen",          "(JIIII)I",                                    (void *) nativeOpen},
    {"nativeGetSupportInfo","(JLjava/util/List;)I",                        (void *) nativeGetSupportInfo},
    {"nativeSetSupportInfo","(JLcom/hsj/camera/UsbCamera$SupportInfo;)I",  (void *) nativeSetSupportInfo},
    {"nativePreview",       "(JLandroid/view/Surface;)I",                  (void *) nativePreview},
    {"nativeStart",         "(J)I",                                        (void *) nativeStart},
    {"nativeStop",          "(J)I",                                        (void *) nativeStop},
    {"nativeClose",         "(J)I",                                        (void *) nativeClose},
    {"nativeDestroy",       "(J)V",                                        (void *) nativeDestroy}
};

//==================================================================================================

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env;
    jint ret = vm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (JNI_OK == ret) {
        //UsbCamera
        jclass cls = env->FindClass(USB_CAMERA);
        if (cls != nullptr) {
            jint size = sizeof(USB_CAMERA_METHODS) / sizeof(JNINativeMethod);
            ret = env->RegisterNatives(cls, USB_CAMERA_METHODS, size);
        } else {
            LOGE("RegisterNatives failed: not find %s class", USB_CAMERA);
            ret = JNI_ERR;
        }
        //JVM
        if (JNI_OK == ret) setJVM(vm);
    }
    return ret == JNI_OK ? JNI_VERSION_1_6 : ret;
}
