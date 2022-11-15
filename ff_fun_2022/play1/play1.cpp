#include <iostream>
#include <thread>
#include <string>
#include <chrono>

#include <opencv2/opencv.hpp>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

//======================================================================================================================
/// Stringify the four-cc
static std::string deFourCC(uint32_t tag) {
    std::string s;
    for (int i = 0; i < 4; ++i) {
        s += (char) (tag & 0xff);
        tag >>= 8;
    }
    return s;
}

//======================================================================================================================
int main(int argc, char **argv) {
    using namespace std;
    using namespace cv;

    cout << "PLAY 1 : A prototype video player" << endl;
    string fileName("/home/seymour/Videos/suteki.mp4");
    if (argc > 1)
        fileName = argv[1];
    cout << "fileName = " << fileName << endl;

    // Open the video file
    AVFormatContext *pFormatCtx = nullptr;
    CV_Assert(avformat_open_input(&pFormatCtx, fileName.c_str(), nullptr, nullptr) == 0);

    // Retrieve stream info
    CV_Assert(avformat_find_stream_info(pFormatCtx, nullptr) >= 0);

    // Dump the stream info
    av_dump_format(pFormatCtx, 0, fileName.c_str(), 0);

    this_thread::sleep_for(chrono::milliseconds(10)); // To avoid cout mixing with cerr
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
    CV_Assert(videoStream >= 0);

    // Get codec parameters
    AVCodecParameters *pCodecPar = pFormatCtx->streams[videoStream]->codecpar;
    //------------- CODEC, decoder
    cout << "Video codec ID = " << pCodecPar->codec_id << ", H264 = " << AV_CODEC_ID_H264 << endl;

    // Find decoder
    AVCodec *pCodec = avcodec_find_decoder(pCodecPar->codec_id);
    CV_Assert(pCodec);
    cout << "\nCodec info from AVCodec :" << endl;
    cout << "name = " << pCodec->name << endl;
    cout << "long_name = " << pCodec->long_name << endl;

    // Create decoder context
    AVCodecContext *pCodecCtx = avcodec_alloc_context3(pCodec);
    CV_Assert(pCodecCtx);
    CV_Assert(avcodec_parameters_to_context(pCodecCtx, pCodecPar) >= 0);
    // Open codec
    CV_Assert(avcodec_open2(pCodecCtx, pCodec, nullptr) >= 0);

    //------------- FRAME

    // Frame size
    int frWidth = pCodecCtx->width;
    int frHeight = pCodecCtx->height;
    cout << "width = " << frWidth << ", height = " << frHeight << endl;
    // RGB Buffer size (actually width*height*3)
    int bufSize = av_image_get_buffer_size(AV_PIX_FMT_BGR24, frWidth, frHeight, 1);
    cout << "bufSize = " << bufSize << " = " << frWidth * frHeight * 3 << endl;
    CV_Assert(bufSize == frWidth * frHeight * 3);

    // Decoder frame, for some kind of YuV
    AVFrame *pFrame = av_frame_alloc();

    // RGB frame + buffer
    AVFrame *pFrameBGR = av_frame_alloc();
    CV_Assert(pFrame && pFrameBGR);
    uint8_t *buffer = (uint8_t *) av_malloc(bufSize);
    CV_Assert(buffer);
    CV_Assert(av_image_fill_arrays(pFrameBGR->data, pFrameBGR->linesize, buffer, AV_PIX_FMT_BGR24, frWidth, frHeight,
                                   1) >= 0);

    // SWS context
    cout << "Decoder pixel format = " << pCodecCtx->pix_fmt << " = " << AV_PIX_FMT_YUV420P << endl;
    SwsContext *swsCtx = sws_getContext(frWidth, frHeight, pCodecCtx->pix_fmt, frWidth, frHeight,
                                               AV_PIX_FMT_BGR24, SWS_BILINEAR, nullptr, nullptr, nullptr);
    CV_Assert(swsCtx);

    //------------- FINALLY, THE GAME LOOP !
    AVPacket packet;
    Mat img(frHeight, frWidth, CV_8UC3);
    int frameCount = 0;
    bool stopFlag = false;
    while (!stopFlag) {
        // Read a packet from the stream
//        cout << "Packet !" << endl;
        CV_Assert(av_read_frame(pFormatCtx, &packet) >= 0);
//        int res = av_read_frame(pFormatCtx, &packet);
//        cout << "res=" << res << endl;
        if (packet.stream_index == videoStream) {
            // Send to decoder
            CV_Assert(avcodec_send_packet(pCodecCtx, &packet) == 0);

            // Receive frames decoded from the packet, can be 0, 1 or more per packet
            for (;;) {
                int err = avcodec_receive_frame(pCodecCtx, pFrame);
                if (err == AVERROR(EAGAIN))  // No more frames for now
                    break;
                else if (err)
                    throw runtime_error("avcodec_receive_frame() failure, err = " + to_string(err));

                ++frameCount;
//                cout << "FRAME " << frameCount << endl;

                // Rescale the frame to RGB with SWS
                sws_scale(swsCtx, (uint8_t const *const *) pFrame->data, pFrame->linesize, 0, frHeight, pFrameBGR->data,
                          pFrameBGR->linesize);

                // Display the frame with OpenCV
                memcpy(img.data, *pFrameBGR->data, (size_t) bufSize);
                imshow("img", img);
                if (27 == waitKey(20)) {
                    stopFlag = true;
                    break;
                }
            } // decode
        }

        // Free the packet
        av_packet_unref(&packet);
    } // read stream


    //------------- SHUTDOWN
    cout << "\nSHUTTING DOWN !\n" << endl;

    // Close frames
    av_free(buffer);
    av_frame_free(&pFrameBGR);
    av_frame_free(&pFrame);
    sws_freeContext(swsCtx);
    avcodec_free_context(&pCodecCtx);
    avformat_close_input(&pFormatCtx);


    return 0;
}
//======================================================================================================================
