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
    FrizzleFFmpeg(JavaCallHepler *javaCallHepler,const char *data_path);
    ~FrizzleFFmpeg();
    void prepare();
    void prepareFFmpeg();

    void start();
    void play();

    void setRenderCallback(RenderFrame renderFrame);

private:
    pthread_t pid_prepare;//准备线程运行完销毁
    pthread_t pid_play;//播放线程,一直存在知道播放完毕
    AVFormatContext *avFormatContext;
    char *url;
    JavaCallHepler *javaCallHepler;
    VideoChannel *videoChannel;
    AudioChannel *audioChannel;
    bool isPlaying;
    RenderFrame renderFrame;
};


#endif //FRIZZLEPLAYERMASTER_FRIZZLEFFMPEG_H
