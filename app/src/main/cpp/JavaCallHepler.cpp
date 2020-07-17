//
// Created by tpson on 2020/7/17.
//

#include "JavaCallHepler.h"
#include "macro.h"

JavaCallHepler::JavaCallHepler(JavaVM *_javaVM, JNIEnv *_env, jobject &_jobj) : javaVm(_javaVM),
                                                                                env(_env) {
    jobj = env->NewGlobalRef(_jobj);

    jclass jclazz = env->GetObjectClass(jobj);
    //反射拿到的ArtMethod结构体
    jmid_error = env->GetMethodID(jclazz, "onError", "(I)V");
    jmid_prepare = env->GetMethodID(jclazz, "onPrepare", "()V");
    jmid_progress = env->GetMethodID(jclazz, "onProgress", "(I)V");
}

void JavaCallHepler::onPrepare(int thread) {
    if (thread == THREAD_CHILD) {
        JNIEnv *jniEnv;
        if (javaVm->AttachCurrentThread(&jniEnv, 0) != JNI_OK) {
            return;
        }
        jniEnv->CallVoidMethod(jobj, jmid_prepare);
        javaVm->DetachCurrentThread();
    } else {
        env->CallVoidMethod(jobj, jmid_prepare);
    }
}

void JavaCallHepler::onError(int thread, int code) {
    if (thread == THREAD_MAIN){//主线程
        //主线程和虚拟机存在联系,直接反射回调
        env->CallVoidMethod(jobj,jmid_error,code);
    } else if (thread == THREAD_CHILD){//子线程
        //子线程需要先JVM绑定线程
        JNIEnv *jniEnv;
        if (javaVm->AttachCurrentThread(&jniEnv,0) !=JNI_OK){
            return;
        }
        jniEnv->CallVoidMethod(jobj,jmid_error,code);
        javaVm->DetachCurrentThread();
    }
}

void JavaCallHepler::onProgress(int thread, int progress) {
    if (thread == THREAD_CHILD) {
        JNIEnv *jniEnv;
        if (javaVm->AttachCurrentThread(&jniEnv, 0) != JNI_OK) {
            return;
        }
        jniEnv->CallVoidMethod(jobj, jmid_progress, progress);
        javaVm->DetachCurrentThread();
    } else {
        env->CallVoidMethod(jobj, jmid_progress, progress);
    }
}
