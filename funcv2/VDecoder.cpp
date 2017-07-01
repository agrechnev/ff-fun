
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
    // Free things
    av_parser_close(pParser);
    av_frame_free(&pFrame);
    av_packet_free(&pPkt);
    avcodec_free_context(&pCtx);
}
//=======================================

void VDecoder::parse(void *inData, size_t inSize, std::function<void (void *, void *, void *)> callBack)
{

}
//=======================================

