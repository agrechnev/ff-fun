#pragma once

#include <cstdio>
#include <string>

extern "C" {
#include <libavcodec/avcodec.h>

#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
}

#include "fatal.h"

class VWriter
{
public: //======= Methods
    /// Constructor
    VWriter(const std::string & fileName, int width, int height);
    /// Destructor
    ~VWriter();

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
    FILE * pFile;
    AVFrame * pFrame;
};
