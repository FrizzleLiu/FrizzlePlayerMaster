//
// Created by tpson on 2020/7/16.
//

#ifndef FRIZZLEPLAYERMASTER_FRIZZLEFFMPEG_H
#define FRIZZLEPLAYERMASTER_FRIZZLEFFMPEG_H
#include <pthread.h>
#include <android/native_window.h>
#include "JavaCallHepler.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/time.h>
}
//控制层
class FrizzleFFmpeg {
public:
    FrizzleFFmpeg(JavaCallHepler *javaCallHepler,const char *data_path);
    ~FrizzleFFmpeg();
    void prepare();
    void prepareFFmpeg();
private:
    pthread_t pid_prepare;
    AVFormatContext *avFormatContext;
    char *url;
    JavaCallHepler *javaCallHepler;
};


#endif //FRIZZLEPLAYERMASTER_FRIZZLEFFMPEG_H
