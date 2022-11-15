//
// Created by Oleksiy Grechnyev on 8/9/18.
//

// A prototype video player - a simple ffmpeg example
// This version uses OpenCV YUV -> BGR conversion, no swscale, no frameRGB
// No classes yet

#include <iostream>
#include <thread>
#include <chrono>

#include <opencv2/opencv.hpp>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
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
    cout << "PLAY 2 : A prototype video player" << endl;

    string fileName("/home/seymour/Videos/tvoya.mp4");
    if (argc > 1)
        fileName = argv[1];

    // Init ffmpeg, register formats+codecs
    av_register_all();

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
    cout << "Video codec ID = " << pCodecPar->codec_id << ", H264 = " << AV_CODEC_ID_H264 << ", AV_CODEC_ID_VP8 = " <<  AV_CODEC_ID_VP8 << endl;

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

    // Input pixel format
    cout << "pixfmt = " << pCodecCtx->pix_fmt << endl;

    //------------- FINALLY, THE GAME LOOP !
    AVPacket packet;
    Mat imgYUV(frHeight * 3 / 2, frWidth, CV_8UC1); // Opencv YUV frame (must preallocate !)
    Mat img(frHeight, frWidth, CV_8UC3); // OpenCV BGR frame
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

                // Copy YUV data from pFrame to the pre-allocated imgYUV
                // STUPID STUPID linesizes !!!
                size_t size0 = (size_t) (frWidth * frHeight / 4);
                for (int i = 0; i < frHeight; ++i) {
                    memcpy(imgYUV.data + frWidth * i, pFrame->data[0] + i * pFrame->linesize[0], frWidth);
                }
                for (int i = 0; i < frHeight / 2; ++i) {
                    memcpy(imgYUV.data + 4 * size0 + frWidth * i / 2, pFrame->data[1] + i * pFrame->linesize[1],
                           frWidth/2);
                    memcpy(imgYUV.data + 5 * size0 + frWidth * i / 2, pFrame->data[2] + i * pFrame->linesize[2],
                           frWidth/2);
                }

//                imshow("imgYUV", imgYUV);

                // Convert the imgYUV to BGR and display with OpenCV
                cvtColor(imgYUV, img, COLOR_YUV2BGR_I420);
                imshow("img", img);
                if (27 == waitKey(1)) {
                    stopFlag = true;
                    break;
                }
                this_thread::sleep_for(chrono::milliseconds(38)); // Delay
            }
        }
        // Free the packet
        av_packet_unref(&packet);
    }

    //------------- SHUTDOWN

    cout << "\nSHUTTING DOWN !\n" << endl;

    // Close frames
    av_frame_free(&pFrame);

    // Close context
    avcodec_free_context(&pCodecCtx);

    // Close the video file
    avformat_close_input(&pFormatCtx);

    return 0;
}
