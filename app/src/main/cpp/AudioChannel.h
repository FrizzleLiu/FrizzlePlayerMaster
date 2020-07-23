//
// Created by Administrator on 2019/6/4.
//

#ifndef FRIZZLEPLAYERMASTER_AUDIOCHANNEL_H
#define FRIZZLEPLAYERMASTER_AUDIOCHANNEL_H

#include <SLES/OpenSLES_Android.h>

extern "C"{
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
}

#include "BaseChannel.h"
#include "JavaCallHepler.h"

class AudioChannel :public BaseChannel {

public:
    AudioChannel(int id, JavaCallHepler *javaCallHelper, AVCodecContext *avCodecContext);
    virtual void play();

    virtual void stop();

    void initOpenSL();

    void decode();

    int getPcm();

private:
    pthread_t pid_audio_play;
    pthread_t pid_audio_decode;
    SwrContext *swr_ctx = NULL;
    int out_channels;
    int out_samplesize;
    int out_sample_rate;
public:
    uint8_t *buffer;
};


#endif
