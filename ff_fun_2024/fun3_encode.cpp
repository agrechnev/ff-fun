//
// Created by Oleksiy Grechnyev
// Fun with encoding with classes
// For simplicity, I use the hardcoded resolution, change if needed

constexpr int FRAME_WIDTH = 640, FRAME_HEIGHT = 480;


#include <iostream>
#include <thread>
#include <string>
#include <chrono>
#include <format>
#include <fstream>
#include <functional>

#include <opencv2/opencv.hpp>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

//======================================================================================================================
/// Converts BGR frame into Yuv with SWScale context
class Rescaler {
public: //=========== Methods
    explicit Rescaler(int width, int height, AVPixelFormat pixFmt) :
            width(width),
            height(height),
            pixFmt(pixFmt) {

        // Create pFrame
        pFrame = av_frame_alloc();
        CV_Assert(pFrame);
        pFrame->format = pixFmt;
        pFrame->width = width;
        pFrame->height = height;
        CV_Assert(0 <= av_frame_get_buffer(pFrame, 0));

        // Create pFrameBGR
        pFrameBGR = av_frame_alloc();
        CV_Assert(pFrameBGR);
        pFrameBGR->format = AV_PIX_FMT_BGR24;
        pFrameBGR->width = width;
        pFrameBGR->height = height;
        CV_Assert(0 <= av_frame_get_buffer(pFrameBGR, 0));

        // And its opencv wrapper
        frameWrapper = cv::Mat(height, width, CV_8UC3, *pFrameBGR->data);

        // Create swscale context
        swsCtx = sws_getContext(width, height, AV_PIX_FMT_BGR24, width, height, pixFmt, SWS_BILINEAR, nullptr, nullptr,
                                nullptr);
        CV_Assert(swsCtx);
    }

    [[nodiscard]] const AVFrame *getPFrame() const {
        return pFrame;
    }

    void process(const cv::Mat &img, uint64_t pts) {
        using namespace std;
        using namespace cv;
        CV_Assert(img.type() == CV_8UC3);
        if (img.size() == frameWrapper.size())
            img.copyTo(frameWrapper);
        else
            resize(img, frameWrapper, frameWrapper.size());

//        CV_Assert((img.size()==Size2i{width, height}));
//        memcpy(*pFrameBGR->data, img.data, FRAME_WIDTH * FRAME_HEIGHT * 3);


        CV_Assert(0 <= av_frame_make_writable(pFrame));
        sws_scale(swsCtx, (uint8_t const *const *) pFrameBGR->data, pFrameBGR->linesize, 0, FRAME_HEIGHT, pFrame->data,
                  pFrame->linesize);
        pFrame->pts = pts;
    }

    ~Rescaler() {
        av_frame_free(&pFrame);
        av_frame_free(&pFrameBGR);
        sws_freeContext(swsCtx);
    }

private: //=========== Fields
    int width, height;
    AVPixelFormat pixFmt;
    AVFrame *pFrame = nullptr;
    AVFrame *pFrameBGR = nullptr;
    cv::Mat frameWrapper;
    SwsContext *swsCtx = nullptr;
};

//======================================================================================================================
/// Manages the encoding
class Encoder {
public: //=========== Methods
    explicit Encoder(int width, int height, AVCodecID codecID, AVPixelFormat pixFmt) {
        using namespace std;
        // Find encoder
        const AVCodec *pCodec = avcodec_find_encoder(codecID);
        CV_Assert(pCodec);
        cout << "\nCodec info from AVCodec :" << endl;
        cout << "name = " << pCodec->name << endl;
        cout << "long_name = " << pCodec->long_name << endl;
        // Create context
        pCodecCtx = avcodec_alloc_context3(pCodec);
        // Set up parameters and open
        pCodecCtx->bit_rate = 400000;
        pCodecCtx->width = width;
        pCodecCtx->height = height;
        pCodecCtx->time_base = AVRational{1, 30};
        pCodecCtx->framerate = AVRational{30, 1};
        pCodecCtx->gop_size = 10;
        pCodecCtx->max_b_frames = 1;
        pCodecCtx->pix_fmt = pixFmt;
        if (pCodec->id == AV_CODEC_ID_H264)
            av_opt_set(pCodecCtx->priv_data, "preset", "slow", 0);
        CV_Assert(0 <= avcodec_open2(pCodecCtx, pCodec, nullptr));
        // Create packet
        pPacket = av_packet_alloc();
        CV_Assert(pPacket);
    }

    ~Encoder() {
        avcodec_free_context(&pCodecCtx);
        av_packet_free(&pPacket);
    };

    void setPacketCb(const std::function<void(AVPacket *)> &packetCb) {
        packetCB = packetCb;
    }

    void encode(const AVFrame *pFrame) {
        CV_Assert(0 <= avcodec_send_frame(pCodecCtx, pFrame));
        for (;;) {
            int ret = avcodec_receive_packet(pCodecCtx, pPacket);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                return;
            CV_Assert(ret >= 0);
            if (packetCB)
                packetCB(pPacket);
            av_packet_unref(pPacket);
        }
    }

    void flush() {
        encode(nullptr);
    }

private: //=========== Fields
    std::function<void(AVPacket *)> packetCB;
    AVCodecContext *pCodecCtx = nullptr;
    AVPacket *pPacket = nullptr;
};

//======================================================================================================================
int main() {
    using namespace std;
    using namespace cv;
    string fileName = "idiot.h264";

    Rescaler rescaler{FRAME_WIDTH, FRAME_HEIGHT, AV_PIX_FMT_YUV420P};
    Encoder encoder{FRAME_WIDTH, FRAME_HEIGHT, AV_CODEC_ID_H264, AV_PIX_FMT_YUV420P};
    ofstream out(fileName, ofstream::binary);
    encoder.setPacketCb([&out](AVPacket *pPacket) -> void {
        cout << "Packet pts=" << pPacket->pts << ", size=" << pPacket->size << endl;
        out.write((const char *) pPacket->data, pPacket->size);
    });

    // Frame loop
    VideoCapture cap(CAP_ANY);
    cout << std::format("FPS={}", cap.get(CAP_PROP_FPS)) << endl;
    Mat img;
    for (int i = 0; i < 100; ++i) {
        if (!cap.read(img) || img.empty())
            break;
        cout << "frame " << i << " : size=" << img.size() << endl;
        rescaler.process(img, i);
        encoder.encode(rescaler.getPFrame());
    }
    encoder.flush();

    return 0;
}