//
// Created by Administrator on 2019/6/4.
//

#ifndef FRIZZLEPLAYERMASTER_AUDIOCHANNEL_H
#define FRIZZLEPLAYERMASTER_AUDIOCHANNEL_H

extern "C"{
#include <libavcodec/avcodec.h>
}

#include "BaseChannel.h"
#include "JavaCallHepler.h"

class AudioChannel :public BaseChannel {

public:
    AudioChannel(int id, JavaCallHepler *javaCallHelper, AVCodecContext *avCodecContext);
    virtual void play();

    virtual void stop();

};


#endif
