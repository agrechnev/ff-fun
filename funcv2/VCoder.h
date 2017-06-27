#pragma once

#include <cstdio>
#include <string>

extern "C" {
#include <libavcodec/avcodec.h>

#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
}

#include "fatal.h"

// Codes
class VCoder
{
public: //======= Methods
    /// Constructor
    VCoder(int width, int height);
    /// Destructor
    ~VCoder();

    /// A sample 24-frame video
    void writeSample();

    /// Write YUV data of sufficient size
    void writeYUV(const uint8_t *data);


private: //======= Methods
    void encode(AVFrame *pF);

private: //======= Data
    int width;
    int height;

    AVCodec * pCodec;
    AVCodecContext *pCtx;
    AVPacket * pPkt;
    AVFrame * pFrame;
};
