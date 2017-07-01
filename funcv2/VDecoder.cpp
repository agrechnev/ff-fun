#include <iostream>

#include "fatal.h"

#include "VDecoder.h"

VDecoder::VDecoder(int width, int height) :
    width(width),
    height(height)
{
    avcodec_register_all();
    pCodec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!pCodec)
        fatal("VDecoder: Cannot find codec.");

    pParser = av_parser_init(pCodec -> id);
    if (!pParser)
        fatal("VDecoder: Cannot find parser.");

    pCtx = avcodec_alloc_context3(pCodec);
    if (!pCtx)
        fatal("VDecoder: Cannot allocate codec context.");

    pPkt = av_packet_alloc();
    if (!pPkt)
        fatal("VDecoder: Cannot allocate packet.");

    // Some parameters here
    pCtx->width = width;
    pCtx->height = height;

    // Time to open the codec
    if (avcodec_open2(pCtx, pCodec, NULL) < 0)
        fatal("VDecoder: Cannot open codec.");

    // Create frame
    pFrame = av_frame_alloc();
    if (!pFrame)
        fatal("VDecoder: Cannot allocate frame.");


}
//=======================================
VDecoder::~VDecoder()
{
    // Free stuff
    av_parser_close(pParser);
    av_frame_free(&pFrame);
    av_packet_free(&pPkt);
    avcodec_free_context(&pCtx);
}
//=======================================

void VDecoder::parse(void *inData, size_t inSize, std::function<void (void *, void *, void *)> callBack)
{
    using namespace std;
    // Loop over frames
    while (inSize > 0) {
        int ret = av_parser_parse2(pParser, pCtx, &pPkt->data, &pPkt->size,
                                   (uint8_t *)inData, inSize, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
        if (ret < 0)
            fatal("VDecoder: Parse error.");
        inData += ret;
        inSize -= ret;

        if (pPkt->size > 0)
            decode(callBack);  // Decode frame
    }
}
//=======================================

void VDecoder::decode(std::function<void (void *, void *, void *)> callBack)
{
    using namespace std;

    if (avcodec_send_packet(pCtx, pPkt) < 0)
        fatal("VDecoder: Error in avcodec_send_packet.");

    for (;;){ // Runs only once for me in reality
        int ret = avcodec_receive_frame(pCtx, pFrame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            break;
        else if (ret < 0)
            fatal("VDecoder::decoding error.");

        callBack(pFrame->data[0], pFrame->data[1], pFrame->data[2]); // 3 parts of the YUV420p data
    }
}
//=======================================

