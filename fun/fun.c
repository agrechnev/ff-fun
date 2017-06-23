#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>


//=================================================
static void fatal(const char * msg)
{
    fputs(msg, stderr);
    exit(1);
}
//=================================================
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

    // Vars, init to NULL just in case
    AVFormatContext * pFormatCtx = NULL;
    AVCodecContext * pCodexCtxOrig = NULL;
    AVCodecContext * pCodexCtx = NULL;


    //---------

    if (argc > 1 && strlen(argv[1]) < 500)
    {
        // Set file name
        strcpy(fileName, argv[1]);
    }

    // Init ffmpeg, register formats+codecs
    av_register_all();

    // Open the video file
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



    return 0;
}
