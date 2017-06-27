#include <iostream>

#include <opencv2/opencv.hpp>

#include "VWriter.h"

int main(){
    using namespace std;
    using namespace cv;

    VWriter vw("goblin.h264", 640, 480);

//    vw.writeSample();
//
//    cout << "Goblins stopped fooling around !" << endl;
//    return 0;

    VideoCapture cam(0);
    Mat frame, yuv;

    for (;;){
        cam.read(frame);

        imshow("Idiot", frame);

//        cvtColor(frame, yuv, CV_BGR2YUV_I420);
        cvtColor(frame, yuv, CV_BGR2YUV_I420);

        vw.writeYUV((uint8_t *)yuv.data);

        imshow("IdiotYUV", yuv);

        if (waitKey(1) == 'q')
            break;
    }

    return 0;
}
