//
// Created by tpson on 2020/7/17.
//

#ifndef FRIZZLEPLAYERMASTER_JAVACALLHEPLER_H
#define FRIZZLEPLAYERMASTER_JAVACALLHEPLER_H

#include "../../../../../../../AppData/Local/Android/Sdk/ndk-bundle/toolchains/llvm/prebuilt/windows-x86_64/sysroot/usr/include/jni.h"

//JNI回调Java
class JavaCallHepler {
public:
    //回调Java层可能会有线程问题所以需要JavaVM
    JavaCallHepler(JavaVM *_javaVM,JNIEnv *_env,jobject &_jobj);

    void onPrepare(int thread);

    void onError(int thread,int code);

    void onProgress(int thread,int progress);

    ~JavaCallHepler();

private:
    JavaVM *javaVm;
    JNIEnv *env;
    jobject jobj;
    jmethodID jmid_prepare;
    jmethodID jmid_error;
    jmethodID jmid_progress;
};


#endif //FRIZZLEPLAYERMASTER_JAVACALLHEPLER_H
