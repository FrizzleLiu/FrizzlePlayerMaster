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

VideoChannel::VideoChannel(int id, JavaCallHepler *javaCallHepler, AVCodecContext *avCodecContext)
        : BaseChannel(id, javaCallHepler, avCodecContext) {
    this->javaCallHelper=javaCallHepler;
    this->avCodecContext=avCodecContext;
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
        //暂时未做音视频同步 这里先延迟16ms 每帧隔16ms去渲染
        av_usleep(16 * 1000);
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
