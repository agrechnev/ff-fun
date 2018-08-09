//
// Created by Oleksiy Grechnyev on 8/9/18.
//

// A prototype video player - a simple ffmpeg example

#include <iostream>
#include <thread>
#include <chrono>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

using namespace std;

/// Stringify the four-cc
string deFourCC(uint32_t tag){
    string s;
    for (int i = 0; i < 4; ++i) {
        s += (char) (tag & 0xff);
        tag >>= 8;
    }
    return s;
}


int main(int argc, char ** argv){
    cout << "PLAY 1 : A prototype video player" << endl;

    string fileName("/home/seymour/Videos/suteki.mp4");
    if (argc > 1)
        fileName = argv[1];

    // Init ffmpeg, register formats+codecs
    av_register_all();

    // Open the video file
    AVFormatContext * pFormatCtx = nullptr;
    if (avformat_open_input(&pFormatCtx, fileName.c_str(), nullptr, nullptr) != 0)
        throw runtime_error("Error opening file " + fileName);

    // Retrieve stream info
    if (avformat_find_stream_info(pFormatCtx, nullptr) < 0)
        throw runtime_error("Error in avformat_find_stream_info() !");

    // Dump the stream info
    av_dump_format(pFormatCtx, 0, fileName.c_str(), 0);

    this_thread::sleep_for(chrono::milliseconds(1)); // To avoid cout mixing with cerr

    cout << "Number of streams = " << pFormatCtx->nb_streams << endl;

    // Stream info
    int audioStream = -1, videoStream = -1;
    for (int i = 0; i < pFormatCtx->nb_streams; ++i) {
        cout << i << " : " << deFourCC(pFormatCtx->streams[i]->codecpar->codec_tag) << endl;
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
            audioStream = i;
        else if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
            videoStream = i;
    }

    cout << "audioStream = " << audioStream << endl;
    cout << "videoStream = " << videoStream << endl;
    if (-1 == videoStream)
        throw runtime_error("No video stream !");



    // Get codec parameters
    AVCodecParameters * pCodecPar = pFormatCtx->streams[videoStream]->codecpar;

    //------------- CODEC, decoder
    cout << "Video codec ID = " << pCodecPar->codec_id << ", H264 = " <<  AV_CODEC_ID_H264 << endl;

    // Find decoder
    AVCodec * pCodec = avcodec_find_decoder(pCodecPar->codec_id);
    if (nullptr == pCodec)
        throw runtime_error("Cannot find decoder !");

    cout << "\nCodec info from AVCodec :" << endl;
    cout << "name = " << pCodec->name << endl;
    cout << "long_name = " << pCodec->long_name << endl;

    // Create context
    AVCodecContext * pCodecCtx = avcodec_alloc_context3(pCodec);
    if (nullptr == pCodecCtx)
        throw runtime_error("avcodec_alloc_context3() failure");
    if (avcodec_parameters_to_context(pCodecCtx, pCodecPar) < 0)
        throw runtime_error("avcodec_parameters_to_context() failure");

    // Open codec
    if (avcodec_open2(pCodecCtx, pCodec, nullptr) < 0)
        throw runtime_error("avcodec_open2() failure");


    //------------- FRAME

    // Frame size
    int frWidth = pCodecCtx->width;
    int frHeight = pCodecCtx->height;
    cout << "width = " << frWidth << ", height = " << frHeight << endl;
    // RGB Buffer size (actually width*height*3)
    int bufSize = av_image_get_buffer_size(AV_PIX_FMT_RGB24, frWidth, frHeight, 1);
    cout << "bufSize = " << bufSize << " = " << frWidth*frHeight*3 << endl;

    // YUV (I guess) frame
    AVFrame * pFrame = av_frame_alloc();
    // RGB frame + buffer
    AVFrame * pFrameRGB = av_frame_alloc();

    //------------- SHUTDOWN

    cout << "\nSHUTTING DOWN !\n" << endl;

    // Close context
    avcodec_free_context(&pCodecCtx);

    // Close the video file
    avformat_close_input(&pFormatCtx);

    return 0;
}