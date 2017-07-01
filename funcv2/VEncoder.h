#pragma once

#include <cstdio>
#include <string>

extern "C" {
#include <libavcodec/avcodec.h>

#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
}


// Codes to H264 using ffmpeg and writes to a mem area
class VEncoder
{
public: //======= Methods
    /// Constructor
    VEncoder(int width, int height);
    /// Destructor
    ~VEncoder();

    // Ban copy and assignment
    VEncoder(const VEncoder &) = delete;
    VEncoder & operator= (const VEncoder &) = delete;

    /// Write YUV data of sufficient size to the dest
    int writeYUV(const uint8_t *data, void * dest, size_t maxSize);

private: //======= Methods
    // Encode a frame to H264
    int encode(AVFrame *pF, void * dest, size_t maxSize);

private: //======= Data
    /// Image width
    int width;
    /// Image height
    int height;

    // Ffmpeg data
    AVCodec * pCodec;
    AVCodecContext *pCtx;
    AVPacket * pPkt;
    AVFrame * pFrame;
};
