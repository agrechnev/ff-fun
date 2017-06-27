#include <cstring>

#include "VCoder.h"

//=================================================

VCoder::VCoder(int width, int height) :
    width(width),
    height(height)
{
    avcodec_register_all();

    pCodec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!pCodec)
        fatal("Cannot find codec.");

    pCtx = avcodec_alloc_context3(pCodec);
    if (!pCtx)
        fatal("Cannot allocate codec context.");

    pPkt = av_packet_alloc();
    if (!pPkt)
        fatal("Cannot allocate packet.");

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
        fatal("Cannot open codec.");

    // Create frame
    pFrame = av_frame_alloc();
    if (!pFrame)
        fatal("Cannot allocate frame.");

    pFrame->format = pCtx->pix_fmt;
    pFrame->width = width;
    pFrame->height = height;

    if (av_frame_get_buffer(pFrame, 32) < 0)
        fatal("Cannot allocate frame buffer.");
}
//=================================================
VCoder::~VCoder()
{


    // Flush the encoder
//    encode(NULL);

    // Free things
    av_frame_free(&pFrame);
    av_packet_free(&pPkt);
    avcodec_free_context(&pCtx);
}
//=================================================
void VCoder::writeSample()
{
    uint8_t buf[2*width*height];

    // The main loop
    for (int i = 0; i < 25 ; ++i)
    {

        // Create a dummy image
        // Y
        for (int y = 0; y < height; ++y)
            for (int x = 0; x < width; ++x)
//                pFrame->data[0][y*pFrame->linesize[0] + x] = x + y + i*3;
                buf[y*pFrame->linesize[0] + x] = x + y + i*3;

        // Cb and Cr

        for (int y = 0; y < height/2; ++y)
            for (int x = 0; x < width/2; ++x)
            {
//                pFrame->data[1][y*pFrame->linesize[1] + x] = 128 + y + i*2;
//                pFrame->data[2][y*pFrame->linesize[2] + x] = ;

                buf[width*height     + y*pFrame->linesize[1] + x] = 128 + y + i*2;
                buf[width*height*5/4 + y*pFrame->linesize[2] + x] = 64 + x + i*5;
            }


        writeYUV(buf);
    }


}


//=================================================
void VCoder::writeYUV(const uint8_t* data)
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
    encode(pFrame);
}

//=================================================
void VCoder::encode(AVFrame* pF)
{
    // Send frame to the encoder
    if (avcodec_send_frame(pCtx, pF) < 0)
        fatal("avcodec_send_frame() failed.");

    for (;;)
    {
        // Receive packet
        int ret = avcodec_receive_packet(pCtx, pPkt);

        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        else if (ret< 0 )
            fatal("Cannot encode.");

        // Write to the file
        fwrite(pPkt->data, 1, pPkt->size, pFile);
        // Wipe the packet clean
        av_packet_unref(pPkt);
    }

}

