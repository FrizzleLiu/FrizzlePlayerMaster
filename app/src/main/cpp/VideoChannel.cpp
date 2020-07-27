//
// Created by tpson on 2020/7/20.
//

#include "BaseChannel.h"

#include "VideoChannel.h"
#include "JavaCallHepler.h"
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
#include <libswscale/swscale.h>
}

//丢弃Packet
//丢Packet需要考虑关键帧保留 以及Frame队列还有数据
void dropPacket(queue<AVPacket *> &q) {

    while (!q.empty()) {
        LOGE("丢弃视频帧数据......");
        AVPacket *pkt = q.front();
        if (pkt->flags != AV_PKT_FLAG_KEY) {
            q.pop();
            BaseChannel::releaseAvPacket(pkt);
        } else{
            break;
        }
    }
}

//丢弃Frame直接清空Frame队列
//Frame队列是和渲染直接关联的,当然两种方式都可以实现,建议丢弃Frame
void dropFrame(queue<AVFrame *> &q) {
    if (!q.empty()) {
        AVFrame *frame = q.front();
        q.pop();
        BaseChannel::releaseAvFrame(frame);
    }
}

VideoChannel::VideoChannel(int id, JavaCallHepler *javaCallHepler, AVCodecContext *avCodecContext,AVRational time_base)
        : BaseChannel(id, javaCallHepler, avCodecContext,time_base) {
    this->javaCallHelper=javaCallHepler;
    this->avCodecContext=avCodecContext;
    //设置队列丢帧的策略(相当于接口)
    frame_queue.setReleaseHandle(releaseAvFrame);
    frame_queue.setSyncHandle(dropFrame);
}

void *decode(void *args) {
    VideoChannel *videoChannel = static_cast<VideoChannel *>(args);
    videoChannel->decodePacket();
    return 0;
}

void *synchronzie(void *args) {
    VideoChannel *videoChannel = static_cast<VideoChannel *>(args);
    videoChannel->synchronzieFrame();
    return 0;
}

void VideoChannel::play() {
    packet_queue.setWork(1);
    frame_queue.setWork(1);
    isPlaying = true;
    //开启子线程解码
    pthread_create(&pid_video_play, NULL, decode, this);
    //开启子线程播放
    pthread_create(&pid_synchronzie, NULL, synchronzie, this);
}

void VideoChannel::stop() {

}

//子线程解码
void VideoChannel::decodePacket() {

    AVPacket *avPacket = 0;
    while (isPlaying) {
        int resultCode = packet_queue.deQueue(avPacket);
        if (!isPlaying) {
            break;
        }

        if (!resultCode) {
            continue;
        }

        resultCode = avcodec_send_packet(avCodecContext, avPacket);

        //使用后释放AVPacket数据
        releaseAvPacket(avPacket);

        if (resultCode == AVERROR(EAGAIN)) {
            LOGE("读取失败 或 需要更多数据");
            continue;
        } else if (resultCode < 0) {
            //失败
            LOGE("失败");
            break;
        }

        //AVFrame  yuvi420   nv21  --->rgb8888
        AVFrame *frame = av_frame_alloc();
        resultCode = avcodec_receive_frame(avCodecContext, frame);
        //压缩数据,要解压
        frame_queue.enQueue(frame);
        //如果frame队列满了,同样延迟10毫秒
        while (frame_queue.size() > 100 && isPlaying) {
            av_usleep(1000 * 10);
            continue;
        }
    }
    releaseAvPacket(avPacket);
}

//子线程播放
void VideoChannel::synchronzieFrame() {
    //初始化转换器上下文 avFrame中存放的是h264(yuv420)的编码格式 转换成SurfaceView默认支持的rgb888
    SwsContext *sws_ctx = sws_getContext(
            avCodecContext->width, avCodecContext->height, avCodecContext->pix_fmt,
            avCodecContext->width, avCodecContext->height, AV_PIX_FMT_RGBA,
            SWS_BILINEAR, 0, 0, 0);

    //1s
    uint8_t *dst_data[4]; //argb
    int dst_linesize[4];
    //声明一帧的容器,这步是初始化,还没有个avFrame有联系,还没拿到数据
    av_image_alloc(dst_data, dst_linesize,
                   avCodecContext->width, avCodecContext->height, AV_PIX_FMT_RGBA, 1);
    AVFrame *frame = 0;

    while (isPlaying) {
        //取出frame
        int resultCode = frame_queue.deQueue(frame);
        if (!isPlaying) {
            break;
        }
        if (!resultCode) {
            continue;
        }
        //这一步是h264(yuv420)转rgb888的
        sws_scale(sws_ctx,
                  reinterpret_cast<const uint8_t *const *>(frame->data), frame->linesize, 0,
                  frame->height,
                  dst_data, dst_linesize);
        //将一帧的数据回调出去,给native-lib(有Window对象)
        renderFrame(dst_data[0], dst_linesize[0], avCodecContext->width, avCodecContext->height);
        LOGE("解码一帧视频 %d",frame_queue.size());
        clock = frame->pts * av_q2d(time_base);
        //计算每一帧的延迟时间,同下,帧率并没有将解码时间计算进去
        double frame_delays = 1.0/fps;
        //音视频同步以音频的播放速度为基准
        //使用ffmpeg里面封装的 (pts数量) * (time_base计算出来的单位) 计算出音频播放的相对时间
        double audioClock = audioChannel->clock;
        //计算解码时间extra_delay 手机配置低或比较卡的时候解码时间会明显导致音视频不同步
        double extra_delay = frame->repeat_pict/(2*fps);
        double delay = extra_delay + frame_delays;
        //计算音视频播放时间的差值
        double diff = clock - audioClock;
        if (clock > audioClock){ //视频超前
            if (diff>1){//如果相差时间比较久,就休眠久一点
                av_usleep((10*2) * 1000000);
            } else {
                av_usleep((delay+diff) * 1000000);
            }
        } else {//视频滞后
            if (diff>1){//如果相差时间比较久,不休眠

            } else if (diff>0.05){//认为视频需要追赶音频,需要丢帧,如果采用减少延迟时间的方式容易造成视频频繁超前和滞后
                //将frame队列中的非关键帧丢掉,不可以丢packet数据,因为packet中存放的是压缩数据,有关键帧的数据
                releaseAvFrame(frame);
                frame_queue.sync();
            } else{

            }
        }


//        double frame_delays = 1000.0/fps;
//        av_usleep(frame_delays);
        releaseAvFrame(frame);
    }
    av_freep(&dst_data[0]);
    isPlaying = false;
    releaseAvFrame(frame);
    sws_freeContext(sws_ctx);
}

void VideoChannel::setRenderCallback(RenderFrame renderFrame) {
    this->renderFrame = renderFrame;
}

void VideoChannel::setFps(int fps) {
    this->fps=fps;
}
