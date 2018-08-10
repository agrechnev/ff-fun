//
// Created by Oleksiy Grechnyev on 8/9/18.
//

// A prototype video player - a simple ffmpeg example
// This version is close in spirit to the "OLD" tutorial by ???
// But I use some C++ and OPenCV for visualization
// No classes yet

#include <iostream>
#include <thread>
#include <chrono>

#include <opencv2/opencv.hpp>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

using namespace std;
using namespace cv;

/// Stringify the four-cc
static string deFourCC(uint32_t tag) {
    string s;
    for (int i = 0; i < 4; ++i) {
        s += (char) (tag & 0xff);
        tag >>= 8;
    }
    return s;
}

int main(int argc, char **argv) {
    cout << "PLAY 1 : A prototype video player" << endl;

//    string fileName("/home/seymour/Videos/suteki.mp4");
    string fileName("rtsp://b1.dnsdojo.com:1935/live/sys3.stream");


    if (argc > 1)
        fileName = argv[1];

    cout << "fileName = " << fileName << endl;

    // Init ffmpeg, register formats+codecs
    av_register_all();
    avformat_network_init();

    // Open the video file
    AVFormatContext *pFormatCtx = nullptr;
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
    AVCodecParameters *pCodecPar = pFormatCtx->streams[videoStream]->codecpar;

    //------------- CODEC, decoder
    cout << "Video codec ID = " << pCodecPar->codec_id << ", H264 = " << AV_CODEC_ID_H264 << endl;

    // Find decoder
    AVCodec *pCodec = avcodec_find_decoder(pCodecPar->codec_id);
    if (nullptr == pCodec)
        throw runtime_error("Cannot find decoder !");

    cout << "\nCodec info from AVCodec :" << endl;
    cout << "name = " << pCodec->name << endl;
    cout << "long_name = " << pCodec->long_name << endl;

    // Create context
    AVCodecContext *pCodecCtx = avcodec_alloc_context3(pCodec);
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
    cout << "bufSize = " << bufSize << " = " << frWidth * frHeight * 3 << endl;

    // YUV (I guess) frame
    AVFrame *pFrame = av_frame_alloc();

    // RGB frame + buffer
    // Note : we don't really need RGB frame or buffer or swscale if we use OpenCV
    // As we can use OpenCV for YUV<->BGR (or RGB) conversion
    // I'll get rid of it in the next example
    AVFrame *pFrameRGB = av_frame_alloc();
    if (!pFrame || !pFrameRGB)
        throw runtime_error("av_frame_alloc() failure");
    uint8_t *buffer = (uint8_t *) av_malloc(bufSize * sizeof(uint8_t));
    if (!buffer)
        throw runtime_error("av_malloc() failure");
    if (av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize, buffer, AV_PIX_FMT_RGB24, frWidth, frHeight, 1) < 0)
        throw runtime_error("av_image_fill_arrays() failure");

    // SWS context
    struct SwsContext *swsCtx = sws_getContext(frWidth, frHeight, pCodecCtx->pix_fmt, frWidth, frHeight,
                                               AV_PIX_FMT_RGB24, SWS_BILINEAR, nullptr, nullptr, nullptr);
    if (!swsCtx)
        throw runtime_error("sws_getContext() failure");

    //------------- FINALLY, THE GAME LOOP !
    AVPacket packet;
    Mat imgRGB(frHeight, frWidth, CV_8UC3); // Opencv images (preallocate !)
    Mat img(frHeight, frWidth, CV_8UC3);
    int frameCount = 0;
    bool stopFlag = false;
    while (!stopFlag) {
        // Read a packet (usually one frame ?) from the stream
        if (av_read_frame(pFormatCtx, &packet) < 0)
            throw runtime_error("av_read_frame() failure");
//        cout << "PACKET : index = " << packet.stream_index << endl;
        // We can receive both audio and video here, select the video !
        if (packet.stream_index == videoStream) {
            // Send it to the decoder
            if (avcodec_send_packet(pCodecCtx, &packet))
                throw runtime_error("avcodec_send_packet() failure");

            // Receive frames decoded from the packet, can be 0, 1 or more per packet
            // That is why fun1 failed with webm !
            for (;;) {
                int err = avcodec_receive_frame(pCodecCtx, pFrame);
                if (err == AVERROR(EAGAIN))  // No more frames for now
                    break;
                else if (err)
                    throw runtime_error("avcodec_receive_frame() failure, err = " + to_string(err));

                ++frameCount;
//            cout << "FRAME " << frameCount << endl;

                // Rescale the frame to RGB with SWS
                sws_scale(swsCtx, (uint8_t const *const *) pFrame->data, pFrame->linesize, 0, frHeight, pFrameRGB->data,
                          pFrameRGB->linesize);

                // Display the frame with OpenCV
                memcpy(imgRGB.data, *pFrameRGB->data, (size_t) bufSize);
                cvtColor(imgRGB, img, COLOR_RGB2BGR);
                imshow("img", img);
                if (27 == waitKey(1)) {
                    stopFlag = true;
                    break;
                }
//                this_thread::sleep_for(chrono::milliseconds(38)); // Delay
            }
        }
        // Free the packet
        av_packet_unref(&packet);
    }

    //------------- SHUTDOWN

    cout << "\nSHUTTING DOWN !\n" << endl;

    // Close frames
    av_free(buffer);
    av_frame_free(&pFrameRGB);
    av_frame_free(&pFrame);

    // Close context
    avcodec_free_context(&pCodecCtx);

    // Close the video file
    avformat_close_input(&pFormatCtx);

    return 0;
}