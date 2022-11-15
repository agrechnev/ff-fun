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
// Save the frame to disk as PPM
static void saveFrame(const AVFrame * pFrame, int width, int height, int ind)
{
    char fileName[100];

    sprintf(fileName, "frame%d.ppm", ind);

    FILE * out = fopen(fileName, "wb");
    if (!out)
        fatal2("Cannot create file ", fileName);

    // Write PPM header
    fprintf(out, "P6\n%d %d\n255\n", width, height);

    // Write pixels
    for (int y=0; y<height; ++y)
        fwrite(pFrame->data[0] + y*(pFrame->linesize[0]), 1, width*3, out);

    fclose(out);
}

//=================================================

int main(int argc, char* argv[])
{
    puts("Goblin fun with ffmpeg:");

    // File name + default
    char fileName[500] = "/home/seymour/Videos/suteki.mp4";

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
    // Video frame, we allocate only frame
    AVFrame * pFrame = av_frame_alloc();
    // RGB frame, we must allocate frame+buffer
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
    // avpicture_fill((AVPicture *)pFrameRGB, buffer, AV_PIX_FMT_RGB24, frWidth, frHeight);


    av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize, buffer, AV_PIX_FMT_RGB24, frWidth, frHeight, 1);

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

    while (av_read_frame(pFormatCtx, &packet) >= 0)
    {
        if (packet.stream_index == videoStream)
        {
            // Decode the video frame

            // Remove the old deprecated function
//            avcodec_decode_video2(pCodecCtx, pFrame, &frameFinshed, &packet);


            // Note : in principle, there can be 0 , 1 or more frames per packet
            // Is this why this code fails on webm ?
            // See play1 for the correct version

            if (avcodec_send_packet(pCodecCtx, &packet))
                fatal("Error in avcodec_send_packet");
            if (avcodec_receive_frame(pCodecCtx, pFrame))
                fatal("Error in avcodec_receive_frame");

            ++i; // Increase frame count


            // Rescale the frame to the RGB format (the actual scale does not change)
            sws_scale(swsCtx, (uint8_t const * const *)pFrame->data, pFrame->linesize, 0,
                      frHeight, pFrameRGB->data, pFrameRGB->linesize);


            // Save frames 100, 110, ..., 200
            if (i>=100 && i<=200 && 0 == i % 10)
            {
                // Save frame to disk as a ppm file
                saveFrame(pFrameRGB, frWidth, frHeight, i);
            }


        }

        // Free the packet allocated by av_read_frame()
//        av_free_packet(&packet);
        av_packet_unref(&packet);
    }


    puts("Goblins are finishing !");

    // Free the frames
    av_free(buffer);
    av_frame_free(&pFrameRGB);
    av_frame_free(&pFrame);

    // Close the codecs
    avcodec_close(pCodecCtx);


    // Aparanetly this is NOT needed, causes avformat_close_input to crash
//    avcodec_parameters_free(&pCodecPar);


    // Close the video file
    avformat_close_input(&pFormatCtx);

    return 0;
}
