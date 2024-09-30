//
// Created by Oleksiy Grechnyev
// Fun with encoding (no muxing yet)

#include <iostream>
#include <thread>
#include <string>
#include <chrono>
#include <format>
#include <fstream>

#include <opencv2/opencv.hpp>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}


//======================================================================================================================
static void encode(AVCodecContext *pCodecCtx, const AVFrame *pFrame, AVPacket *packet, std::ostream &out) {
    using namespace std;
    CV_Assert(0 <= avcodec_send_frame(pCodecCtx, pFrame));
    for (;;) {
        int ret = avcodec_receive_packet(pCodecCtx, packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        CV_Assert(ret >= 0);
        cout << "Packet pts=" << packet->pts << ", size=" << packet->size << endl;
        out.write((const char *)packet->data, packet->size);
        av_packet_unref(packet);
    }
}

//======================================================================================================================
int main(int argc, char **argv) {
    using namespace std;
    using namespace cv;

    constexpr int FRAME_WIDTH = 640, FRAME_HEIGHT = 480;
    string fileName = "idiot.h264";

    //--- codec
    const AVCodec *pCodec = avcodec_find_encoder(AV_CODEC_ID_H264);
//    const AVCodec *pCodec = avcodec_find_encoder_by_name("libx264");
    CV_Assert(pCodec);
    cout << "\nCodec info from AVCodec :" << endl;
    cout << "name = " << pCodec->name << endl;
    cout << "long_name = " << pCodec->long_name << endl;

    AVCodecContext *pCodecCtx = avcodec_alloc_context3(pCodec);
    // Set up parameters
    pCodecCtx->bit_rate = 400000;
    pCodecCtx->width = FRAME_WIDTH;
    pCodecCtx->height = FRAME_HEIGHT;
    pCodecCtx->time_base = AVRational{1, 25};
    pCodecCtx->framerate = AVRational{25, 1};
    pCodecCtx->gop_size = 10;
    pCodecCtx->max_b_frames = 1;
    pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    if (pCodec->id == AV_CODEC_ID_H264)
        av_opt_set(pCodecCtx->priv_data, "preset", "slow", 0);
    CV_Assert(0 <= avcodec_open2(pCodecCtx, pCodec, nullptr));

    // Frames
    AVFrame *pFrame = av_frame_alloc();
    CV_Assert(pFrame);
    pFrame->format = pCodecCtx->pix_fmt;
    pFrame->width = pCodecCtx->width;
    pFrame->height = pCodecCtx->height;
    CV_Assert(0 <= av_frame_get_buffer(pFrame, 0));

    AVFrame *pFrameBGR = av_frame_alloc();
    CV_Assert(pFrameBGR);
    pFrameBGR->format = AV_PIX_FMT_BGR24;
    pFrameBGR->width = pCodecCtx->width;
    pFrameBGR->height = pCodecCtx->height;
    CV_Assert(0 <= av_frame_get_buffer(pFrameBGR, 0));


    // SWS context
    SwsContext *swsCtx = sws_getContext(FRAME_WIDTH, FRAME_HEIGHT, AV_PIX_FMT_BGR24, FRAME_WIDTH, FRAME_HEIGHT,
                                        pCodecCtx->pix_fmt, SWS_BILINEAR, nullptr, nullptr, nullptr);
    CV_Assert(swsCtx);

    // Frame loop
    VideoCapture cap(CAP_ANY);
    cout << std::format("FPS={}", cap.get(CAP_PROP_FPS)) << endl;
    Mat img;
    ofstream out(fileName, ofstream::binary);
    CV_Assert(out.good());
    AVPacket* pPacket = av_packet_alloc();
    for (int i = 0; i < 100; ++i) {
        if (!cap.read(img) || img.empty())
            break;
        cout << "frame " << i << " : size=" << img.size() << endl;
        CV_Assert(img.cols == FRAME_WIDTH && img.rows == FRAME_HEIGHT);
        // Convert to yuv
        memcpy(*pFrameBGR->data, img.data, FRAME_WIDTH * FRAME_HEIGHT * 3);
        CV_Assert(0 <= av_frame_make_writable(pFrame));
        sws_scale(swsCtx, (uint8_t const *const *) pFrameBGR->data, pFrameBGR->linesize, 0, FRAME_HEIGHT, pFrame->data,
                  pFrame->linesize);
        // ENcode
        pFrame->pts = i;
        encode(pCodecCtx, pFrame, pPacket, out);
    }
    // FLush
    encode(pCodecCtx, nullptr, pPacket, out);


    // --- shutdown
    cout << "\nSHUTTING DOWN !\n" << endl;
    av_packet_free(&pPacket);
    sws_freeContext(swsCtx);
    av_frame_free(&pFrame);
    av_frame_free(&pFrameBGR);
    avcodec_free_context(&pCodecCtx);

    return 0;
}