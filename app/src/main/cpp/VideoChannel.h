//
// Created by tpson on 2020/7/20.
//

#ifndef FRIZZLEPLAYERMASTER_VIDEOCHANNEL_H
#define FRIZZLEPLAYERMASTER_VIDEOCHANNEL_H

#include "BaseChannel.h"
#include <pthread.h>
#include <android/native_window.h>
#include "AudioChannel.h"
#include "JavaCallHepler.h"
//接口回调 将VideoChannel中解码转码之后拿到的一帧数据回调给native-lib
//参数依次是 rgb数据 rgb的hang像素个数 图片宽 图片稿
typedef void(*RenderFrame)(uint8_t * , int, int, int);
class VideoChannel : public BaseChannel{

public:
    VideoChannel(int id, JavaCallHepler *javaCallHepler, AVCodecContext *avCodecContext);

    virtual void play();

    virtual void stop();

    void decodePacket();

    void synchronzieFrame();

    void setRenderCallback(RenderFrame renderFrame);

private:
    pthread_t pid_video_play;
    pthread_t pid_synchronzie;
    RenderFrame renderFrame;
};


#endif //FRIZZLEPLAYERMASTER_VIDEOCHANNEL_H
