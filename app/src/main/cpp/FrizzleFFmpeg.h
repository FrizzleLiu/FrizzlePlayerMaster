//
// Created by tpson on 2020/7/16.
//

#ifndef FRIZZLEPLAYERMASTER_FRIZZLEFFMPEG_H
#define FRIZZLEPLAYERMASTER_FRIZZLEFFMPEG_H
#include <pthread.h>
#include <android/native_window.h>
#include "VideoChannel.h"
#include "AudioChannel.h"
#include "BaseChannel.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/time.h>
}
//控制层
class FrizzleFFmpeg {
public:
    int duration;
    FrizzleFFmpeg(JavaCallHepler *javaCallHepler,const char *data_path);
    virtual ~FrizzleFFmpeg();
    void prepare();
    void prepareFFmpeg();

    void start();
    void play();

    void setRenderCallback(RenderFrame renderFrame);

    int getDuration();

    void seek(jint progress);

    void stop();

public:
    pthread_t pid_prepare;//准备线程运行完销毁
    pthread_t pid_play;//播放线程,一直存在知道播放完毕
    pthread_t pid_stop;//释放属于耗时操作
    AVFormatContext *avFormatContext;
    char *url;
    JavaCallHepler *javaCallHepler;
    VideoChannel *videoChannel;
    AudioChannel *audioChannel;
    bool isPlaying;
    RenderFrame renderFrame;
    bool isSeek = 0;
    pthread_mutex_t seekMutex;
};


#endif //FRIZZLEPLAYERMASTER_FRIZZLEFFMPEG_H
