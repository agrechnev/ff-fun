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
    Mat frameIn, frameInYuv;

    constexpr size_t BUFFER_SIZE = 128*1024;
    uint8_t buffer[BUFFER_SIZE]; // Buffer for the h264 packet

//    FILE * pFile = fopen("goblin2.h264", "wb");

    for (;;){
        cam.read(frameIn);  // Read a frame
        imshow("frameIn", frameIn);
        cvtColor(frameIn, frameInYuv, CV_BGR2YUV_I420); // Convert to YUV420p
        // Encode to H264
        int packetSize = venc.writeYUV((uint8_t *)frameInYuv.data, buffer, BUFFER_SIZE);
        if (packetSize > 0) {
//            fwrite(buffer, packetSize, 1, pFile);

        }

        if (waitKey(1) == 'q')
            break;
    }
//    fclose(pFile);
    return 0;
}
