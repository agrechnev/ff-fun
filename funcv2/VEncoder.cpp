#include <cstring>
#include <cstdlib>

#include "fatal.h"

#include "VEncoder.h"

//=================================================

VEncoder::VEncoder(int width, int height) :
    width(width),
    height(height)
{
    avcodec_register_all();

    pCodec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!pCodec)
        fatal("VEncoder: Cannot find codec.");

    pCtx = avcodec_alloc_context3(pCodec);
    if (!pCtx)
        fatal("VEncoder: Cannot allocate codec context.");

    pPkt = av_packet_alloc();
    if (!pPkt)
        fatal("VEncoder: Cannot allocate packet.");

    // Sample parameters. Evil !
//    pCtx->bit_rate = 400000;
    pCtx->bit_rate = 0;
    pCtx->width = width;
    pCtx->height = height;

    // FPS
    pCtx->time_base = (AVRational)
    {
        1, 25
    };
    pCtx->framerate = (AVRational)
    {
        25, 1
    };

    // Something stupid
//    pCtx->gop_size = 10;
    pCtx->max_b_frames = 1;
    pCtx->pix_fmt = AV_PIX_FMT_YUV420P;

    // WTF is this really? I guess I'll need "ultrafast" later
    if (pCodec->id == AV_CODEC_ID_H264)
        av_opt_set(pCtx->priv_data, "preset", "fast", 0);

    // Time to open the codec
    if (avcodec_open2(pCtx, pCodec, NULL) < 0)
        fatal("VEncoder: Cannot open codec.");

    // Create frame
    pFrame = av_frame_alloc();
    if (!pFrame)
        fatal("VEncoder: Cannot allocate frame.");

    pFrame->format = pCtx->pix_fmt;
    pFrame->width = width;
    pFrame->height = height;

    if (av_frame_get_buffer(pFrame, 32) < 0)
        fatal("Cannot allocate frame buffer.");
}
//=================================================
VEncoder::~VEncoder()
{


    // Flush the encoder
//    encode(NULL);

    // Free things
    av_frame_free(&pFrame);
    av_packet_free(&pPkt);
    avcodec_free_context(&pCtx);
}

//=================================================
int VEncoder::writeYUV(const uint8_t *data, void * dest, size_t maxSize)
{
    fflush(stdout);

    // Make sure data is writable, why ?
    if (av_frame_make_writable(pFrame) < 0)
        fatal("av_frame_make_writable() failed.");


    const size_t size0 = height*width/4;

    memcpy(pFrame->data[0], data, size0*4);  // Y
    memcpy(pFrame->data[1], data + size0*4, size0);  // Cb
    memcpy(pFrame->data[2], data + size0*5, size0);  // Cr

    // Seems to work just fine without any timestamp
    //pFrame->pts = i

    // Encode the image
    return encode(pFrame, dest, maxSize);
}

//=================================================
int VEncoder::encode(AVFrame* pF, void * dest, size_t maxSize)
{
    using namespace std;

    // Send frame to the encoder
    if (avcodec_send_frame(pCtx, pF) < 0)
        fatal("avcodec_send_frame() failed.");

    size_t totalSize = 0;

    for (;;)
    {
        // Receive packet
        int ret = avcodec_receive_packet(pCtx, pPkt);

        // Note: in my experience sending a frame either
        // gives you 0 packets (first few frames)
        // or exactly 1 packet, never more
        // But I did it by the book anyway

        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            break;
        else if (ret< 0 )
            fatal("Cannot encode.");

        if (totalSize + pPkt->size <= maxSize) {
            // Copy data to destination
            memcpy(dest, pPkt->data, pPkt->size);
            totalSize += pPkt->size;
            dest += pPkt->size;
        }

        // Wipe the packet clean
        av_packet_unref(pPkt);
    }

    return totalSize;
}

