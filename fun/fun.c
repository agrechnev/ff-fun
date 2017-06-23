#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>


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

int main(int argc, char* argv[])
{
    puts("Goblin fun with ffmpeg:");

    // File name + default
    char fileName[500] = "/home/kaa-dev/grechnyev/stream/media-samp/Suteki.mp4";

    if (argc > 1 && strlen(argv[1]) < 500)
    {
        // Set file name
        strcpy(fileName, argv[1]);
    }

    // Init ffmpeg, register formats+codecs
    av_register_all();

    // Open the video file
    AVFormatContext * pFormatCtx = NULL;
    if (avformat_open_input(&pFormatCtx, fileName, NULL, NULL) != 0)
        fatal2("Cannot open input file ", fileName);

    // Retrieve stream info
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
        fatal("Cannot find stream info");

    // Write stream info to stderr
    av_dump_format(pFormatCtx, 0, fileName, 0);


    // Find the 1st video stream
    int videoStream = -1;
    for (int i=0; i < pFormatCtx->nb_streams; ++i)
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            videoStream = i;
            break;
        }

    printf("videoStream = %d \n", videoStream);
    if (-1 == videoStream)
        fatal("Video stream not found");


    // Get codec parameters
    AVCodecParameters * pCodecPar = pFormatCtx->streams[videoStream]->codecpar;

    // Find the decoder
    AVCodec *pCodec = avcodec_find_decoder(pCodecPar->codec_id);
    if (pCodec == NULL)
        fatal("Cannot find decoder");

    // Create context out of codec parameters
    AVCodecContext * pCodecCtx = avcodec_alloc_context3(pCodec);
    if (avcodec_parameters_to_context(pCodecCtx, pCodecPar) < 0)
        fatal("Cannot create codec context from parameters");

    // Open codec
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
        fatal("Cannot open codec");

    //------------------------------------------------------
    // Now, the frame
    // Video frame
    AVFrame * pFrame = av_frame_alloc();
    // RGB frame
    AVFrame * pFrameRGB = av_frame_alloc();
    if (pFrame==NULL || pFrameRGB==NULL)
        fatal("Cannot allocate frame");


    // Frame width and height
    int frWidth = pCodecCtx->width;
    int frHeight = pCodecCtx->height;

    // Allocate buffer
    // Replaced another deprecated function, the result is actually width*height*3, how simple !
    int bufSize = av_image_get_buffer_size(AV_PIX_FMT_RGB24, frWidth, frHeight, 1);
    printf("width = %d, height = %d, numBytes = %d \n", frWidth, frHeight, bufSize);
    uint8_t * buffer = (uint8_t *)av_malloc(bufSize * sizeof(uint8_t));


    // Assign appropriate parts of buffer to image planes in pFrameRGB
    // Note that pFrameRGB is an AVFrame, but AVFrame is a superset
    // of AVPicture
    avpicture_fill((AVPicture *)pFrameRGB, buffer, AV_PIX_FMT_RGB24, frWidth, frHeight);
//    av_image_fill_arrays()

    // Set the SWS context. SWS is the SoftWare Scale library, here apparently we don't change size
    // But recode from the input pixel formal (YUV or whatever) to RGB
    struct SwsContext *swsCtx = sws_getContext(frWidth, frHeight, pCodecCtx->pix_fmt, frWidth, frHeight, AV_PIX_FMT_RGB24,
                            SWS_BILINEAR, NULL, NULL, NULL);

    //------------------------------------------------------
    // Finally, the Big Frame loop, we read a frame and process it

    int i = 0; // Frame number
    int frameFinshed; // bool: decoded successfully ?
    AVPacket packet;

    while (av_read_frame(pFormatCtx, &packet) >= 0) {
        if (packet.stream_index == videoStream) {
            // Decode video frame
            avcodec_decode_video2(pCodecCtx, pFrame, &frameFinshed, &packet);
            ++i; // Increase frame count


        }
    }


    puts("Goblins are finishing !");

    return 0;
}
