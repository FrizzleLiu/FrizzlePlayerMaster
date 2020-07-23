//
// Created by tpson on 2020/7/16.
//

#include "FrizzleFFmpeg.h"
#include "JavaCallHepler.h"
#include "macro.h"
#include "AudioChannel.h"

void *prepareFFmpeg_(void *args) {
    FrizzleFFmpeg *frizzleFFmpeg = static_cast<FrizzleFFmpeg *>(args);
    frizzleFFmpeg->prepareFFmpeg();
    return 0;
}

FrizzleFFmpeg::FrizzleFFmpeg(JavaCallHepler *javaCallHepler, const char *data_path) {
    url = new char[strlen(data_path) + 1];
    this->javaCallHepler = javaCallHepler;
    //因为dataPath使用完毕内存会被释放,copy一下
    strcpy(url, data_path);
}

FrizzleFFmpeg::~FrizzleFFmpeg() {

}

void FrizzleFFmpeg::prepare() {
    //FFmpeg初始化比较耗时,开启线程
    //线程调用prepareFFmpeg_方法,将 FrizzleFFmpeg自身作为参数传递过去
    pthread_create(&pid_prepare, NULL, prepareFFmpeg_, this);
}

void FrizzleFFmpeg::prepareFFmpeg() {
    //子线程中运行 可以访问到对象属性(prepareFFmpeg_的接收参数是FrizzleFFmpeg)
    //初始化网络模块
    avformat_network_init();
    //总的上下文
    avFormatContext = avformat_alloc_context();
    //设置超时时间3s
    AVDictionary *opts = NULL;
    av_dict_set(&opts, "timeout", "3000000", 0);
    //打开url 可以使本地视频文件,也可以是网络流
    int resultCode = avformat_open_input(&avFormatContext, url, NULL, &opts);
    //打开Url失败,反射回调Java层
    if (resultCode != 0) {
        if (javaCallHepler) {
            javaCallHepler->onError(THREAD_CHILD, FFMPEG_CAN_NOT_OPEN_URL);
        }
        return;
    }
    //查找流失败
    if (avformat_find_stream_info(avFormatContext, NULL) < 0) {
        if (javaCallHepler) {
            javaCallHepler->onError(THREAD_CHILD, FFMPEG_CAN_NOT_FIND_STREAMS);
        }
        return;
    }

    for (int i = 0; i < avFormatContext->nb_streams; ++i) {
        AVCodecParameters *codecpar = avFormatContext->streams[i]->codecpar;
        //找解码器
        AVCodec *avCodec = avcodec_find_decoder(codecpar->codec_id);
        //找不到解码器
        if (!avCodec) {
            if (javaCallHepler) {
                javaCallHepler->onError(THREAD_CHILD, FFMPEG_FIND_DECODER_FAIL);
            }
            return;
        }

        //创建解码器上下文
        AVCodecContext *avCodecContext = avcodec_alloc_context3(avCodec);
        if (!avCodecContext) {
            if (javaCallHepler) {
                javaCallHepler->onError(THREAD_CHILD, FFMPEG_ALLOC_CODEC_CONTEXT_FAIL);
            }
            return;
        }

        //复制参数给上下文
        if (avcodec_parameters_to_context(avCodecContext, codecpar) < 0) {
            if (javaCallHepler) {
                javaCallHepler->onError(THREAD_CHILD, FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL);
            }
            return;
        }

        //打开解码器
        if (avcodec_open2(avCodecContext, avCodec, 0) != 0) {
            if (javaCallHepler) {
                javaCallHepler->onError(THREAD_CHILD, FFMPEG_OPEN_DECODER_FAIL);
            }
            return;
        }

        //音频流
        if (codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioChannel = new AudioChannel(i, javaCallHepler, avCodecContext);
        }

        //视频流
        if (codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoChannel = new VideoChannel(i, javaCallHepler, avCodecContext);
            videoChannel->setRenderCallback(renderFrame);
        }
    }
    //音视频都没有
    if (!audioChannel && !videoChannel) {
        if (javaCallHepler)
            javaCallHepler->onError(THREAD_CHILD, FFMPEG_NOMEDIA);
        return;
    }
    //FFmpeg准备完成回调
    if (javaCallHepler) {
        javaCallHepler->onPrepare(THREAD_CHILD);
    }

}

//开始解码的线程
void *startThread(void *args) {
    FrizzleFFmpeg *frizzleFFmpeg = static_cast<FrizzleFFmpeg *>(args);
    frizzleFFmpeg->play();
    //线程函数 不返回0容易编译失败
    return 0;
}

//开始解码播放
void FrizzleFFmpeg::start() {
    //播放视频
    isPlaying = true;
    if (videoChannel) {
        videoChannel->play();
        LOGE("videoChannel开始播放");
    }
    //播放音频
    if (audioChannel) {
        audioChannel->play();
    }

    pthread_create(&pid_play, NULL, startThread, this);
}

void FrizzleFFmpeg::play() {
    int resultCode = 0;
    while (isPlaying) {
        // 读数据的速度一般是大于渲染速度的 因为渲染每一帧之间都是有延迟时间的
        // 也就是生产者的生产速度远远大于消费者的消费速度,做下延迟处理,休眠10ms,不处理可能会OOM
        if (audioChannel && audioChannel->packet_queue.size()>100){
            av_usleep(1000*10);
            continue;
        }

        if (videoChannel && videoChannel->packet_queue.size()>100){
            av_usleep(1000*10);
            continue;
        }

        AVPacket *avPacket = av_packet_alloc();
        resultCode = av_read_frame(avFormatContext, avPacket);

        //  将音/视频Packet放入对应队列
        if (resultCode == 0) {
            if (audioChannel && avPacket->stream_index == audioChannel->channelId) {
                audioChannel->packet_queue.enQueue(avPacket);
            } else if (videoChannel && avPacket->stream_index == videoChannel->channelId) {
                LOGE("存入一帧视频数据");
                videoChannel->packet_queue.enQueue(avPacket);
            }
        }else if (resultCode == AVERROR_EOF) {
            //读取完毕 但是不一定播放完毕
            LOGE("读取完毕 但是不一定播放完毕");
            if (videoChannel->packet_queue.empty() && videoChannel->frame_queue.empty() &&
                audioChannel->packet_queue.empty() && audioChannel->frame_queue.empty()) {
                LOGE("播放完毕");
                break;
            }
            //因为seek 的存在，就算读取完毕，依然要循环 去执行av_read_frame(否则seek了没用...)
        } else {
            break;
        }
    }

    isPlaying= false;
    if (audioChannel){
        audioChannel->stop();
    }
    if (videoChannel){
        videoChannel->stop();
    }
}

void FrizzleFFmpeg::setRenderCallback(RenderFrame renderFrame) {
    this->renderFrame=renderFrame;
}

