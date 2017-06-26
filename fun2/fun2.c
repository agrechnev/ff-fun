#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include <libavcodec/avcodec.h>

#include <libavutil/opt.h>
#include <libavutil/imgutils.h>


// Based on the "encode" example in ffmpeg

//=================================================
/// Print an error message and exit
static void fatal(const char * msg)
{
    fputs(msg, stderr);
    exit(1);
}
//=================================================
/// Print a 2-part error message and exit
static void fatal2(const char * msg1, const char * msg2)
{
    fprintf(stderr, "%s %s\n", msg1, msg2);
    exit(1);
}
//=================================================
static void encode(AVCodecContext *pCtx, AVFrame *pFrame, AVPacket *pPkt, FILE * pFile) {
    // Send frame to the encoder
    if (avcodec_send_frame(pCtx, pFrame) < 0)
        fatal("avcodec_send_frame() failed.");

    for (;;){
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
//=================================================

int main()
{
    puts("Goblin encoder !");

    const char * fileName = "goblin.h264";

    avcodec_register_all();

    const AVCodec * pCodec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!pCodec)
        fatal("Cannot find codec.");

    AVCodecContext *pCtx = avcodec_alloc_context3(pCodec);
    if (!pCtx)
        fatal("Cannot allocate codec context.");

    AVPacket * pPkt = av_packet_alloc();
    if (!pPkt)
        fatal("Cannot allocate packet.");

    // Sample parameters. Evil !
//    pCtx->bit_rate = 400000;
    pCtx->bit_rate = 0;
    pCtx->width = 352;
    pCtx->height = 288;

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
//        av_opt_set(pCtx->priv_data, "preset", "slow", 0);
        av_opt_set(pCtx->priv_data, "preset", "fast", 0);

    // Time to open the codec
    if (avcodec_open2(pCtx, pCodec, NULL) < 0)
        fatal("Cannot open codec.");

    // Create file
    FILE * pFile = fopen(fileName, "wb");
    if (!pFile)
        fatal2("Cannot create file", fileName);

    // Create frame
    AVFrame *pFrame = av_frame_alloc();
    if (!pFrame)
        fatal("Cannot allocate frame.");

    pFrame->format = pCtx->pix_fmt;
    int width = pCtx->width;
    int height = pCtx->height;
    pFrame->width = width;
    pFrame->height = height;

    if (av_frame_get_buffer(pFrame, 32) < 0)
        fatal("Cannot allocate frame buffer.");


    // The main loop
    for (int i = 0; i < 25 ; ++i)
    {
        fflush(stdout);

        // Make sure data is writable, why ?
        if (av_frame_make_writable(pFrame) < 0)
            fatal("av_frame_make_writable() failed.");

        // Create a dummy image
        // Y
        for (int y = 0; y < height; ++y)
            for (int x = 0; x < width; ++x)
                pFrame->data[0][y*pFrame->linesize[0] + x] = x + y + i*3;

        // Cb and Cr

        for (int y = 0; y < height/2; ++y)
            for (int x = 0; x < width/2; ++x){
                pFrame->data[1][y*pFrame->linesize[1] + x] = 128 + y + i*2;
                pFrame->data[2][y*pFrame->linesize[2] + x] = 64 + x + i*5;
            }


        pFrame->pts = i; // Timestamp in time_base units

        // Encode the image
        encode(pCtx, pFrame, pPkt, pFile);
    }

    // Flush the encoder
    encode(pCtx, NULL, pPkt, pFile);


    // Finish for an MPEG file, probably wrong for H264
    const uint8_t endCode[] = {0, 0, 1, 0xb7};
    fwrite(endCode, 1, sizeof(endCode), pFile);
    fclose(pFile);

    // Free things

    av_frame_free(&pFrame);
    av_packet_free(&pPkt);
    avcodec_free_context(&pCtx);

    puts("Goblins finished!");

    return 0;
}
