/// \file pusher.cpp
/// \brife
///
/// \date 2019/4/16
/// \version 0.1.0
/// \author waponxie
/// \copyright
/// |修改人|修改日期|修改描述|
/// |:-----|:-------|:-------|
/// |waponxie|2019/4/16|创建文件|

extern "C"
{
#include <libavformat/avformat.h>
#include <libavutil/mathematics.h>
#include <libavutil/time.h>
}

#include "pusher/pusher.h"
#include "general/log.h"

/// \brief 关闭输入和输出的上下文
/// \param inputContext 需要被关闭的输入流上下文
/// \param outputContext 需要被关闭的输出流的上下文
/// \return 0 表示成功，否则表示失败
int32_t CloseContext(AVFormatContext *inputContext, AVFormatContext *outputContext)
{
    if (inputContext != nullptr)
    {
        avformat_close_input(&inputContext);
    }

    if (outputContext != nullptr)
    {
        if (outputContext->oformat->flags & AVFMT_NOFILE)
        {
            avio_close(outputContext->pb);
        }
        avformat_free_context(outputContext);
    }

    return 0;
}

Pusher::Pusher() : m_isInit(false), m_output(NULL), m_input(""), m_inputContext(nullptr), m_outputContext(nullptr)
{

}

Pusher::Pusher(const std::string &input, const std::string &output)
        : m_isInit(false),
          m_output(output),
          m_input(input),
          m_inputContext(nullptr),
          m_outputContext(nullptr)
{
}

Pusher::~Pusher()
{
    CloseContext(m_inputContext, m_outputContext);
}

bool Pusher::Initialize()
{
    int ret = 0;

    m_inputContext = avformat_alloc_context();
    if(nullptr == m_inputContext)
    {
        return false;
    }

    m_outputContext = avformat_alloc_context();
    if(nullptr == m_inputContext)
    {
        avformat_close_input(&m_inputContext);
        return false;
    }

    ret = avformat_open_input(&m_inputContext, m_input.c_str(), nullptr, nullptr);
    if (ret < 0)
    {
        LOG("Could not open input file. error code is %d", ret);
        avformat_close_input(&m_inputContext);
        return false;
    }
    LOG("Input format %s, duration %lld us", m_inputContext->iformat->long_name, m_inputContext->duration);

    ret = avformat_find_stream_info(m_inputContext, 0);
    if (ret < 0)
    {
        LOG("Failed to retrieve input stream information. error code is %d", ret);
        avformat_close_input(&m_inputContext);
    }

    av_dump_format(m_inputContext, 0, m_input.c_str(), 0);
    avformat_alloc_output_context2(&m_outputContext, NULL, "flv", m_output.c_str());
    LOG("Input format %s, duration %lld us", m_outputContext->oformat->long_name, m_outputContext->duration);

    if (m_outputContext == nullptr)
    {
        LOG("Could not create output context\n");
        CloseContext(m_inputContext, m_outputContext);
        return false;
    }

    for (uint32_t index = 0; index < m_inputContext->nb_streams; ++index)
    {
        //根据输入流创建输出流
        AVStream *inStream = m_inputContext->streams[index];
        AVStream *outStream = avformat_new_stream(m_outputContext, inStream->codec->codec);
        if (nullptr == outStream)
        {
            LOG("Failed allocating output/input stream\n");
            CloseContext(m_inputContext, m_outputContext);
            return false;
        }
        //复制AVCodecContext的设置（Copy the settings of AVCodecContext）
        ret = avcodec_copy_context(outStream->codec, inStream->codec);
        if (ret < 0)
        {
            LOG("Failed to copy context from input to output stream codec context\n");
            CloseContext(m_inputContext, m_outputContext);
            return false;
        }
        outStream->codec->codec_tag = 0;
        if (m_outputContext->oformat->flags & AV_CODEC_FLAG_GLOBAL_HEADER)
        {
            outStream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        }
    }

    av_dump_format(m_outputContext, 0, m_output.c_str(), 1);
    /// 打开输出端，建立连接，访问URL。
    if (!(m_outputContext->oformat->flags & AVFMT_NOFILE))
    {
        /// 使用写标记打开
        int ret = avio_open(&m_outputContext->pb, m_output.c_str(), AVIO_FLAG_WRITE);
        if (ret < 0)
        {
            LOG("Could not open output URL '%s', Error Code is %d", m_output, ret);
            CloseContext(m_inputContext, m_outputContext);
            return false;
        }
    }

    ret = ParseVideoAndAudioStreamIndex();
    if (ret != 0)
    {
        return false;
    }

    m_isInit = true;
    return true;
}


int32_t Pusher::ParseVideoAndAudioStreamIndex()
{
    for (uint32_t index = 0; index < m_inputContext->nb_streams; ++index)
    {
        if (m_inputContext->streams[index]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            m_videoStreamIndex.push_back(index);
        }

        if (m_inputContext->streams[index]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            m_audioStreamIndex.push_back(index);
        }
    }

    if (m_videoStreamIndex.size() == 0)
    {
        return -__LINE__;
    }

    return 0;
}


void Pusher::Delay(const AVPacket &packet, const int64_t &startTime) const
{
    if (packet.stream_index == m_videoStreamIndex[0])
    {
        AVRational timeBase = m_inputContext->streams[m_videoStreamIndex[0]]->time_base;
        AVRational timeBaseQ = {1, AV_TIME_BASE};
        int64_t ptsTime = av_rescale_q(packet.dts, timeBase, timeBaseQ);
        int64_t nowTime = av_gettime() - startTime;
        if (ptsTime > nowTime)
        {
            av_usleep(ptsTime - nowTime);
        }
    }
}

int32_t Pusher::Push()
{
    //写文件头（Write file header）
    int ret = avformat_write_header(m_outputContext, NULL);

    if (ret < 0)
    {
        LOG("Error occurred when opening output URL, Error Code is %d\n", ret);
        CloseContext(m_inputContext, m_outputContext);
    }
    AVPacket packet;
    uint32_t videoWriteFrameCount = 0;
    int64_t start_time = av_gettime();
    while (true)
    {
        AVStream *inStream, *outStream;
        //获取一个数据包
        ret = av_read_frame(m_inputContext, &packet);
        if (ret < 0)
        {
            LOG("faild to read one packet from input, Error Code is %d\n", ret);
            break;
        }

        //FIX：No PTS (Example: Raw H.264)
        //Simple Write PTS
        if (packet.pts == AV_NOPTS_VALUE)
        {
            //Write PTS
            AVRational time_base1 = m_inputContext->streams[m_videoStreamIndex[0]]->time_base;
            //Duration between 2 frames (us)
            int64_t calc_duration =
                    (double) AV_TIME_BASE / av_q2d(m_inputContext->streams[m_videoStreamIndex[0]]->r_frame_rate);
            //Parameters
            packet.pts = (double) (videoWriteFrameCount * calc_duration) / (double) (av_q2d(time_base1) * AV_TIME_BASE);
            packet.dts = packet.pts;
            packet.duration = (double) calc_duration / (double) (av_q2d(time_base1) * AV_TIME_BASE);
        }

        Delay(packet, start_time);

        inStream = m_inputContext->streams[packet.stream_index];
        outStream = m_outputContext->streams[packet.stream_index];
        /* copy packet */
        //转换PTS/DTS（Convert PTS/DTS）
        packet.pts = av_rescale_q_rnd(packet.pts, inStream->time_base, outStream->time_base,
                                      (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        packet.dts = av_rescale_q_rnd(packet.dts, inStream->time_base, outStream->time_base,
                                      (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        packet.duration = av_rescale_q(packet.duration, inStream->time_base, outStream->time_base);
        packet.pos = -1;

        //Print to Screen
        if (packet.stream_index == m_videoStreamIndex[0])
        {
            LOG("Send %8d video frames to output URL\n", videoWriteFrameCount);
            ++videoWriteFrameCount;
        }

        ret = av_interleaved_write_frame(m_outputContext, &packet);

        if (ret < 0)
        {
            LOG("Error muxing packet\n");
            av_free_packet(&packet);
            break;
        }

        av_free_packet(&packet);

    }
    //写文件尾（Write file trailer）
    av_write_trailer(m_outputContext);

    return 0;
}