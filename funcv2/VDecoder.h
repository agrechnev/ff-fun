#pragma once
// By Oleksiy Grechnyev

#include <functional>
#include <cstdlib>

extern "C" {
#include <libavcodec/avcodec.h>
}

class VDecoder
{
public: //================ Methods
    /// Constructor
    VDecoder(int width, int height);
    /// Destructor
    ~VDecoder();

    /// Parse the input data into frames and call a callback with the YUV data for each one
    void parse(void *inData, size_t inSize, std::function<void(void*, void*, void*)> callBack);

    // Delete copy and assignment
    VDecoder(const VDecoder &) = delete;
    VDecoder & operator= (const VDecoder &) = delete;

private: //================ Methods
    /// Decode a frame and call the callback
    void decode(std::function<void(void*, void*, void*)> callBack);
private: //============= Data
    /// Image width
    int width;
    /// Image height
    int height;

    // Ffmpeg data
    AVCodec * pCodec;
    AVCodecContext *pCtx;
    AVCodecParserContext *pParser;
    AVPacket * pPkt;
    AVFrame * pFrame;
};

