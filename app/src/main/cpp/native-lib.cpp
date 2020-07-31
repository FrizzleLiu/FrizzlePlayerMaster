#include <jni.h>
#include <string>
#include <android/native_window_jni.h>
#include "JavaCallHepler.h"
#include "FrizzleFFmpeg.h"

extern  "C"{
#include "libavcodec/avcodec.h"
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

void renderFrame(uint8_t *data,int linesize , int width ,int height){
    //渲染
    //设置窗口属性
    ANativeWindow_setBuffersGeometry(window, width,
                                     height,
                                     WINDOW_FORMAT_RGBA_8888);

    ANativeWindow_Buffer window_buffer;
    if (ANativeWindow_lock(window, &window_buffer, 0)) {
        ANativeWindow_release(window);
        window = 0;
        return;
    }
    //缓冲区  window_data[0] =255;
    uint8_t *window_data = static_cast<uint8_t *>(window_buffer.bits);
    //r     g   b  a int  字节转换成int所以需要*4
    int window_linesize = window_buffer.stride * 4;
    uint8_t *src_data = data;
    //数据copy 按像素遍历宽高循环速度太慢 gif等播放要求不高的可以采取这种方式
    //整体直接copy容易花屏(数据中一行的数据和屏幕的像素宽度可能不一致)
    //所以一样一行的copy 处理内存对齐的问题
    for (int i = 0; i < window_buffer.height; ++i) {
        //每一行偏移的时候window偏移window_linesize   src_data偏移数据源的一行的数据size
        memcpy(window_data + i * window_linesize, src_data + i * linesize, window_linesize);
    }
    ANativeWindow_unlockAndPost(window);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_frizzle_frizzleplayermaster_player_FrizzlePlayer_native_1prepare(JNIEnv *env, jobject thiz,
                                                                          jstring data_source_) {
    //视频路径
    const char *data_source=env->GetStringUTFChars(data_source_,0);
    javaCallHepler =new JavaCallHepler(javaVm,env,thiz);
    frizzleFFmpeg= new FrizzleFFmpeg(javaCallHepler, data_source);
    frizzleFFmpeg->setRenderCallback(renderFrame);
    //初始化ffmpeg
    frizzleFFmpeg->prepare();

    env->ReleaseStringUTFChars(data_source_,data_source);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_frizzle_frizzleplayermaster_player_FrizzlePlayer_native_1start(JNIEnv *env, jobject thiz) {
    //开始播放
    if (frizzleFFmpeg){
        LOGE("FFmpeg准备完毕,开始播放");
        frizzleFFmpeg->start();
    }

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

extern "C"
JNIEXPORT jint JNICALL
Java_com_frizzle_frizzleplayermaster_player_FrizzlePlayer_native_1getDuration(JNIEnv *env,
                                                                              jobject thiz) {
   //获取视频时长
   if(frizzleFFmpeg){
       return frizzleFFmpeg->getDuration();
   }
}

//拖拽进度
extern "C"
JNIEXPORT void JNICALL
Java_com_frizzle_frizzleplayermaster_player_FrizzlePlayer_native_1seek(JNIEnv *env, jobject thiz,
                                                                       jint progress) {
    if (frizzleFFmpeg){
        frizzleFFmpeg->seek(progress);
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_frizzle_frizzleplayermaster_player_FrizzlePlayer_native_1stop(JNIEnv *env, jobject thiz) {
    //停止播放
    if (frizzleFFmpeg){
        frizzleFFmpeg->stop();
    }

    if (javaCallHepler){
        delete javaCallHepler;
        javaCallHepler = 0;
    }

}

extern "C"
JNIEXPORT void JNICALL
Java_com_frizzle_frizzleplayermaster_player_FrizzlePlayer_native_1release(JNIEnv *env,
                                                                          jobject thiz) {
   //释放资源
   if (window){
       ANativeWindow_release(window);
       window=0;
   }
}