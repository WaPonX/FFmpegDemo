/// \file rtmp_flv_pusher_demo.cpp
/// \brife
///
/// \date 2019/4/18
/// \version
/// \author waponxie
/// \copyright
/// |修改人|修改日期|修改描述|
/// |:-----|:-------|:-------|
/// |waponxie|2019/4/18|创建文件|

#include <string>
extern "C"
{
#include <libavformat/avformat.h>
}

#include "pusher/pusher.h"


int main()
{
    //avformat_network_init();
    std::string inFilename = "test.mp4";
    std::string outFilename = "rtmp://192.168.12.129/live";
    Pusher p(inFilename, outFilename);
    p.Initialize();
    p.Push();
}