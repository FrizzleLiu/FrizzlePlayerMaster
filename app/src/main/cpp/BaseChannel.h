//
// Created by tpson on 2020/7/20.
//

#ifndef FRIZZLEPLAYERMASTER_BASECHANNEL_H
#define FRIZZLEPLAYERMASTER_BASECHANNEL_H
extern "C"{
#include <libavcodec/avcodec.h>
};

#include "safe_queue.h"
#include "JavaCallHepler.h"

class BaseChannel {

public:
    BaseChannel(int id, JavaCallHepler *javaCallHelper, AVCodecContext *avCodecContext
    ,AVRational time_base) : channelId(id),
        javaCallHelper(javaCallHelper),
        avCodecContext(avCodecContext),time_base(time_base)
    {
        packet_queue.setReleaseHandle(releaseAvPacket);
        frame_queue.setReleaseHandle(releaseAvFrame);
    };
    static void releaseAvPacket(AVPacket *&packet) {
        if (packet) {
            av_packet_free(&packet);
            packet = 0;
        }
    }

    static void releaseAvFrame(AVFrame *&frame) {
        if (frame) {
            av_frame_free(&frame);
            frame = 0;
        }
    }

    virtual ~BaseChannel() {
        if (avCodecContext) {
            avcodec_close(avCodecContext);
            avcodec_free_context(&avCodecContext);
            avCodecContext = 0;
        }
        packet_queue.clear();
        frame_queue.clear();
        LOGE("释放channel:%d %d", packet_queue.size(), frame_queue.size());
    };

    virtual void play()=0;
    virtual void stop()=0;
    SafeQueue<AVPacket *> packet_queue;
    SafeQueue<AVFrame *> frame_queue;
    volatile int channelId;
    volatile bool isPlaying  ;
    AVCodecContext *avCodecContext;
    JavaCallHepler *javaCallHelper;
    AVRational time_base;
    //用于音视频同步,记录当前音频播放的时间线,是相对时间
    double clock;
};
#endif //FRIZZLEPLAYERMASTER_BASECHANNEL_H
