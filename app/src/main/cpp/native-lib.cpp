#include <jni.h>
#include <string>
#include <android/native_window_jni.h>
#include "FrizzleFFmpeg.h"
#include "JavaCallHepler.h"
extern "C"{
#include <libavcodec/avcodec.h>
}

ANativeWindow *window=0;
FrizzleFFmpeg *frizzleFFmpeg;
JavaCallHepler *javaCallHepler;
//JVM 因为在子线程回调,所以需要使用JVM对象
JavaVM *javaVm=NULL;
//获取JavaVM对象,该方法写在这里会主动调用
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    javaVm = vm;
    return JNI_VERSION_1_4;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_frizzle_frizzleplayermaster_player_FrizzlePlayer_native_1prepare(JNIEnv *env, jobject thiz,
                                                                          jstring data_source_) {
    //视频路径
    const char *data_source=env->GetStringUTFChars(data_source_,0);
    javaCallHepler =new JavaCallHepler(javaVm,env,thiz);
    frizzleFFmpeg= new FrizzleFFmpeg(javaCallHepler, data_source);
    //初始化ffmpeg
    frizzleFFmpeg->prepare();

    env->ReleaseStringUTFChars(data_source_,data_source);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_frizzle_frizzleplayermaster_player_FrizzlePlayer_native_1start(JNIEnv *env, jobject thiz) {


}

extern "C"
JNIEXPORT void JNICALL
Java_com_frizzle_frizzleplayermaster_player_FrizzlePlayer_native_1set_1surface(JNIEnv *env,
                                                                               jobject thiz,
                                                                               jobject surface) {
    if (window){
        ANativeWindow_release(window);
        window=0;
    }
    //创建window窗体显示视频
    window = ANativeWindow_fromSurface(env,surface);

}