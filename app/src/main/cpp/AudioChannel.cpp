//
// Created by Administrator on 2019/6/4.
//
extern "C"{
#include <libavutil/time.h>
}
#include "AudioChannel.h"

void *audioPlay(void *args) {
    AudioChannel *audioChannel = static_cast<AudioChannel *>(args);
    audioChannel->initOpenSL();
    return 0;
}

void *audioDecode(void *args) {
    AudioChannel *audioChannel = static_cast<AudioChannel *>(args);
    audioChannel->decode();
    return 0;
}

AudioChannel::AudioChannel(int id, JavaCallHepler *javaCallHelper, AVCodecContext *avCodecContext,AVRational time_base)
        : BaseChannel(id, javaCallHelper, avCodecContext,time_base) {
    //根据布局获取声道数
    out_channels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
    out_samplesize = av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
    out_sample_rate = 44100;
    //CD音频标准
    //44100 双声道 2字节    out_samplesize  16位  2个字节   out_channels  2
    buffer = (uint8_t *) malloc(out_sample_rate * out_samplesize * out_channels);
}

void AudioChannel::play() {
    //初始化转换器的上下文
    swr_ctx = swr_alloc_set_opts(0, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16, out_sample_rate,
                                   avCodecContext->channel_layout,
                                   avCodecContext->sample_fmt,
                                   avCodecContext->sample_rate, 0, 0);
    swr_init(swr_ctx);
    packet_queue.setWork(1);
    frame_queue.setWork(1);
    isPlaying = true;
    //创建OPENSL ES的线程
    pthread_create(&pid_audio_play, NULL, audioPlay, this);
    //创建初始化音频解码的线程
    pthread_create(&pid_audio_decode, NULL, audioDecode, this);
}

void AudioChannel::stop() {

}

void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context) {
    AudioChannel *audioChannel = static_cast<AudioChannel *>(context);
    //开始播放,需要获取PCM原始音频数据
    int datalen = audioChannel->getPcm();
    if (datalen > 0) {
        (*bq)->Enqueue(bq, audioChannel->buffer, datalen);
    }
}

//初始化OpenSL ES 参考https://github.com/android/ndk-samples的audio_module
//使用OpenSl ES 的步骤
//①创建音频引擎 -> ②设置混音器 -> ③创建播放器(录音器) -> ④设置缓冲队列和回调函数  -> ⑤设置播放状态 -> ⑥启动回调函数

void AudioChannel::initOpenSL() {
    //音频引擎
    SLObjectItf engineObject = NULL;
    //音频对象 可以通过此对象设置音频引擎的参数
    SLEngineItf engineInterface = NULL;
    //混音器 (调音师)
    SLObjectItf outputMixObject = NULL;
    //播放器
    SLObjectItf bqPlayerObject = NULL;
    //回调接口
    SLPlayItf bqPlayerInterface = NULL;
    //缓冲队列
    SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue = NULL;

    SLresult sLresult;
    /****************初始化引擎相关**************/
    //创建引擎
    sLresult = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
    if (SL_RESULT_SUCCESS != sLresult) {
        //初始化失败,一般没有音频权限才会初始化失败
        return;
    }
    //初始化音频引擎
    sLresult = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != sLresult) {
        return;
    }
    //获取音频引擎的回调接口 相当于SurfaceView的SurfaceHolder
    sLresult = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineInterface);
    if (SL_RESULT_SUCCESS != sLresult) {
        return;
    }

    /****************初始化混音器相关**************/

    //创建混音器
    sLresult = (*engineInterface)->CreateOutputMix(engineInterface, &outputMixObject, 0, 0, 0);
    //初始化混音器
    sLresult = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != sLresult) {
        return;
    }

    /****************初始化播放器相关**************/

    //
    SLDataLocator_AndroidSimpleBufferQueue android_queue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
                                                            2};//双声道所以传2
    //pcm数据格式(视频文件中解码的Frame数据中存放的是pcm数据)
    SLDataFormat_PCM pcm = {SL_DATAFORMAT_PCM//播放pcm格式的数据
            , 2,//2个声道（立体声）
                            SL_SAMPLINGRATE_44_1, //44100hz的频率
                            SL_PCMSAMPLEFORMAT_FIXED_16,//位数 16位
                            SL_PCMSAMPLEFORMAT_FIXED_16,//和位数一致就行
                            SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,//立体声（前左前右）
                            SL_BYTEORDER_LITTLEENDIAN//小端模式
    };
    //混音器的队列
    SLDataLocator_OutputMix outputMix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};

    SLDataSink audioSnk = {&outputMix, NULL};
    SLDataSource slDataSource = {&android_queue, &pcm};
    //播放队列的id
    const SLInterfaceID ids[1] = {SL_IID_BUFFERQUEUE};
    const SLboolean req[1] = {SL_BOOLEAN_TRUE};
    (*engineInterface)->CreateAudioPlayer(engineInterface, &bqPlayerObject //播放器
            , &slDataSource //播放器参数  播放缓冲队列   播放格式
            , &audioSnk //播放缓冲区
            , 1 //播放接口回调个数
            , ids //设置播放队列ID
            , req //是否采用内置的播放队列
    );
    //初始化播放器
    (*bqPlayerObject)->Realize(bqPlayerObject, SL_BOOLEAN_FALSE);
    //    得到接口后调用  获取Player接口
    (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_PLAY, &bqPlayerInterface);
//    获得播放器接口
    (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_BUFFERQUEUE,
                                    &bqPlayerBufferQueue);

    (*bqPlayerBufferQueue)->RegisterCallback(bqPlayerBufferQueue, bqPlayerCallback, this);
    //    设置播放状态(可播放)
    (*bqPlayerInterface)->SetPlayState(bqPlayerInterface, SL_PLAYSTATE_PLAYING);
    //启动回调函数 会触发缓冲区不断地读取数据
    bqPlayerCallback(bqPlayerBufferQueue, this);
    LOGE("--- 手动调用播放 packet:%d", this->packet_queue.size());
}

//解码音频数据
//不断的将AVPacket的数据解码拿到Frame数据
void AudioChannel::decode() {
    AVPacket *packet = 0;
    while (isPlaying) {
        int resultCode = packet_queue.deQueue(packet);
        if (!isPlaying) {
            break;
        }
        if (!resultCode) {
            continue;
        }

        resultCode = avcodec_send_packet(avCodecContext, packet);
        releaseAvPacket(packet);
        if (resultCode == AVERROR(EAGAIN)) {
            //需要更多数据
            continue;
        } else if (resultCode < 0) {
            //失败
            break;
        }
        AVFrame *frame = av_frame_alloc();
        resultCode = avcodec_receive_frame(avCodecContext, frame);
        if (resultCode == AVERROR(EAGAIN)) {
            //需要更多数据
            continue;
        } else if (resultCode < 0) {
            break;
        }
        //防止队列内存过大,因为解码速度大于播放速度
        while (frame_queue.size() > 100 && isPlaying) {
            av_usleep(1000 * 10);
            continue;
        }
        frame_queue.enQueue(frame);
    }
}

//将aac mp3等格式的文件转码成统一格式的数据播放,会设置一些采样率,声道个数等参数,swr_ctx就是转换上下文
//将Frame中的数据 解码为PCM数据
int AudioChannel::getPcm() {
    AVFrame *frame = 0;
    int data_size = 0;
    while (isPlaying) {
        int ret = frame_queue.deQueue(frame);
        //转换
        if (!isPlaying) {
            break;
        }
        if (!ret) {
            continue;
        }
        uint64_t dst_nb_samples = av_rescale_rnd(
                swr_get_delay(swr_ctx, frame->sample_rate) + frame->nb_samples,
                out_sample_rate,
                frame->sample_rate,
                AV_ROUND_UP);
        // 转换，返回值为转换后的sample个数  buffer malloc（size）
        int nb = swr_convert(swr_ctx, &buffer, dst_nb_samples,
                             (const uint8_t **) frame->data, frame->nb_samples);
        //转换后多少数据  计算buffer size  44110*2*2
        data_size = nb * out_channels * out_samplesize;
        //计算音频播放的时间 pts是一个数量 av_q2d(time_base)获取的是单位,获取相对时间
        clock= frame->pts * av_q2d(time_base);
        break;
    }
    releaseAvFrame(frame);
    return data_size;
}
