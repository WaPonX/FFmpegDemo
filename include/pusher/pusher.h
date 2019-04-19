/// \file pusher.h
/// \brife
///
/// \date 2019/4/16
/// \version 0.1.0
/// \author waponxie
/// \copyright 
/// |修改人|修改日期|修改描述|
/// |:-----|:-------|:-------|
/// |waponxie|2019/4/16|创建文件|

#pragma once

#include <stdint.h>
#include <memory>
#include <string>
#include <libavformat/avformat.h>
#include <vector>

#include "general/nocopyable.h"

class Pusher : public Nocopyable
{
public:
    Pusher();

    Pusher(const std::string &input, const std::string &output);

    ~Pusher();

    /// \brief 初始化函数
    /// \return 初始化成功则返回true，否则返回0
    /// \warning 该函数是普通成员函数，调用时需注意指针类型
    bool Initialize();

    /// \brief 延迟推流时间
    /// 数据的推送需要按照音视频的实际帧率推送，否则服务器可能扛不住。该函数是阻塞的，直到可以发送数据，才返回。
    /// \param packet 要被发送的数据包
    /// \param startTime 起始的时间戳
    void Delay(const AVPacket &packet, const int64_t &startTime) const;

    /// \brief 将数据推送到输出端
    /// \return 0 表示成功，否则表示失败
    int32_t Push();

private:
    /// \brief 从输入流中分辨出音频是和视频轨的流ID
    /// \return 0表示成功，否则表示失败
    int32_t ParseVideoAndAudioStreamIndex();

private:
    bool m_isInit; ///< 是否初始化成功
    std::string m_output; ///< 接收端
    std::string m_input;  ///< 推送端的数据来源，可以是本地文件，也可以另一条直播流
    AVFormatContext *m_inputContext; ///< 输入端的上下文
    AVFormatContext *m_outputContext; ///< 输出端的上下文
    std::vector <uint32_t> m_videoStreamIndex; ///< 视频轨所在的流ID
    std::vector <uint32_t> m_audioStreamIndex; ///< 音频轨所在的流ID
};