//
// Created by tpson on 2020/7/16.
//

#include "FrizzleFFmpeg.h"
#include "macro.h"

void *prepareFFmpeg_(void *args) {
    FrizzleFFmpeg *frizzleFFmpeg = static_cast<FrizzleFFmpeg *>(args);
    frizzleFFmpeg->prepareFFmpeg();
}

FrizzleFFmpeg::FrizzleFFmpeg(JavaCallHepler *javaCallHepler, const char *data_path) {
    url = new char[strlen(data_path) + 1];
    this->javaCallHepler=javaCallHepler;
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
    }
}

