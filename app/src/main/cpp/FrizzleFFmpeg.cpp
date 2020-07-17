//
// Created by tpson on 2020/7/16.
//

#include "FrizzleFFmpeg.h"

void *prepareFFmpeg_(void *args){
    FrizzleFFmpeg *frizzleFFmpeg = static_cast<FrizzleFFmpeg *>(args);
    frizzleFFmpeg->prepareFFmpeg();
}

FrizzleFFmpeg::FrizzleFFmpeg(const char *dataPath) {
    url=new char [strlen(dataPath)+1];
    //因为dataPath使用完毕内存会被释放,copy一下
    strcpy(url,dataPath);
}

FrizzleFFmpeg::~FrizzleFFmpeg(){

}

void FrizzleFFmpeg::prepare() {
    //FFmpeg初始化比较耗时,开启线程
    //线程调用prepareFFmpeg_方法,将 FrizzleFFmpeg自身作为参数传递过去
    pthread_create(&pid_prepare,NULL,prepareFFmpeg_,this);
}

void FrizzleFFmpeg::prepareFFmpeg() {
    //子线程中运行 可以访问到对象属性(prepareFFmpeg_的接收参数是FrizzleFFmpeg)
    //初始化网络模块
    avformat_network_init();
    //总的上下文
    avFormatContext=avformat_alloc_context();
}

