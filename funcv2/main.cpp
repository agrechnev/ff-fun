#include <iostream>
#include <cstdlib>
#include <cstdio>

#include <opencv2/opencv.hpp>

#include "VEncoder.h"
#include "VDecoder.h"

int main(){
    using namespace std;
    using namespace cv;


    constexpr int CAMERA_WIDTH = 640;
    constexpr int CAMERA_HEIGHT = 480;

    VEncoder venc(CAMERA_WIDTH, CAMERA_HEIGHT); // Encoder
    VDecoder vdec(CAMERA_WIDTH, CAMERA_HEIGHT); // Decoder

    VideoCapture cam(0);
    Mat frameIn, frameInYuv; // In Frames
    Mat frameOut; // Out Frames
    Mat frameOutYuv(CAMERA_HEIGHT*3/2, CAMERA_WIDTH, CV_8UC1); // Pre-allocate the YUV frame

    // Buffer for the h264 packet between encode and decode
    constexpr size_t BUFFER_SIZE = 128*1024;
    uint8_t buffer[BUFFER_SIZE  + AV_INPUT_BUFFER_PADDING_SIZE]; // With 32 bytes padding required by ffmpeg

//    FILE * pFile = fopen("goblin2.h264", "wb");

    for (;;){
        cam.read(frameIn);  // Read a frame
        imshow("frameIn", frameIn);
        cvtColor(frameIn, frameInYuv, CV_BGR2YUV_I420); // Convert to YUV420p
        // Encode to H264
        int packetSize = venc.writeYUV((uint8_t *)frameInYuv.data, buffer, BUFFER_SIZE);
        if (packetSize > 0) {
            vdec.parse(buffer, packetSize,
                       [&frameOut, &frameOutYuv, CAMERA_WIDTH, CAMERA_HEIGHT]
                       (void * d0, void * d1, void * d2)->void{
                // Create a Yuv frame
                const size_t size0 = CAMERA_WIDTH*CAMERA_HEIGHT/4;
                memcpy(frameOutYuv.data, d0, size0*4);
                memcpy(frameOutYuv.data + size0*4, d1, size0);
                memcpy(frameOutYuv.data + size0*5, d2, size0);

                cvtColor(frameOutYuv, frameOut, CV_YUV2BGR_I420); // Convert YUV420p to BGR
                imshow("frameOut", frameOut);

                // Will need it here if no other waitKey
//                if (waitKey(1) == 'q')
//                    exit(0);
            });
//            fwrite(buffer, packetSize, 1, pFile);

        }

        if (waitKey(1) == 'q')
            break;
    }
//    fclose(pFile);
    return 0;
}
