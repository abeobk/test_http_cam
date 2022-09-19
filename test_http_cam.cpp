#include <stdio.h>
#include <iostream>
#include <chrono>
#include <opencv2/opencv.hpp>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <tcpserver.h>
#include <tictoc.h>
#include <textcmd.h>
#include <atomic>
#include <vector>
#include <sstream>
#include <mutex>
#include <map>
#include <cmath>
#include <functional>
#include <thread>

//#define USE_GSTREAMER
#define CAM_W 1280
#define CAM_H 960

using namespace cv;
using namespace std;
using namespace abeosys;


std::mutex mtx;

cv::VideoCapture cap;
cv::Mat buff_img;
cv::Mat last_img;


TicToc tt;
#define PORT_NUM 12345

#define ASSERT_(expr, msg) \
    if (!(expr))           \
    throw std::runtime_error(msg)
#define ASSERT_ARGS_(n) ASSERT_(args.size() == n, "Invalid arguments.");

int bytesPerPixel(int type)
{
    switch (type)
    {
    case CV_8SC1: case CV_8UC1: return 1;
    case CV_8SC3: case CV_8UC3: return 3; 
    case CV_8SC4: case CV_8UC4: case CV_16UC2: case CV_16SC2: case CV_32SC1: case CV_32FC1: return 4; 
    case CV_32SC2: case CV_32FC2: case CV_64FC1: return 8;
    case CV_32SC3: case CV_32FC3: return 12;
    case CV_32SC4: case CV_32FC4: case CV_64FC2: return 16;
    case CV_64FC3: return 24;
    case CV_64FC4: return 32;
    default: return 1;
    }
}

#ifdef USE_GSTREAMER
std::string gstreamer_pipeline(int capture_width, int capture_height, int display_width, int display_height, int framerate, int flip_method)
{
    return "nvarguscamerasrc aelock=true exposuretimerange='33333333 33333333' ! video/x-raw(memory:NVMM), width=(int)" + std::to_string(capture_width) + ", height=(int)" +
           std::to_string(capture_height) + ", format=(string)NV12, framerate=(fraction)" + std::to_string(framerate) +
           "/1 ! nvvidconv flip-method=" + std::to_string(flip_method) + " ! video/x-raw, width=(int)" + std::to_string(display_width) + ", height=(int)" +
           std::to_string(display_height) + ", format=(string)BGRx ! videoconvert ! video/x-raw, format=(string)BGR ! appsink";
}
#endif


void clientService(TCPChannel &con)
{
    cv::Mat img, cimg;
    const int PIXEL_GRAY = 0;
    const int PIXEL_RGB = 1;
    const int PIXEL_BGR = 2;
    const int PIXEL_HSV = 3;

    std::map<std::string, CommandExecutor> cmdmap = {
         {"GET", {[&](TextCommand const &tc) {
             std::stringstream res;
             res << "HTTP/1.1 200 OK\r\n";
             //res << "Connection: Close\r\n";
             res << "Expires: -1\r\n";
             res << "Cache-Control: no-store, no-cache, must-revalidate, max-age=0, post-check=0, pre-check=0, false\r\n";
             res << "Pragma: no-cache\r\n";
             res << "Content-Type: multipart/x-mixed-replace;boundary=abeobk\r\n";
             res << "\r\n";
             con.send(res.str());

             while(true){
                 TicToc tt;
                 tt.tic();
                 std::vector<uchar> buf;
                 {
                    std::lock_guard<std::mutex> lock(mtx);    
                    last_img = buff_img.clone();
                 }
                 cv::imencode(".jpg",last_img, buf, {cv::IMWRITE_JPEG_QUALITY,50});
                 cout<<"Encode time: "<<tt.toc()<<"ms"<<endl;

                 tt.tic();
                 std::stringstream wcbuf;
                 wcbuf<<"--abeobk\r\n";
                 wcbuf<<"Content-Type: image/jpeg\r\n";
                 wcbuf<<"Content-Length: " << buf.size()<< "\r\n";
                 wcbuf<<"\r\n";
                 con.send(wcbuf.str());
                 auto cnt= con.send((char const*)buf.data(), buf.size());
                 cout<<"Send time: "<<tt.toc()<<"ms, byte_count="<<cnt<<endl;
                 
             }
           },
           "GET method"}},
    };

    con.runCommandLoop(cmdmap);
}

void preService() { }
void postService() { }

int main(int argc, char **argv)
{
    std::cout<<"Initializing camera..."<<std::endl;
    //cap.release();

#ifdef USE_GSTREAMER
    std::string pipeline = gstreamer_pipeline(CAM_W, CAM_H, CAM_W, CAM_H, 90, 0);
    cap = VideoCapture(pipeline);
#else
    cap = VideoCapture(0,cv::CAP_V4L2);
    cap.set(cv::CAP_PROP_FRAME_WIDTH, CAM_W);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, CAM_H);
    cap.set(cv::CAP_PROP_FPS, 90);

    cv::Mat img;
    cap >> img;
    cout << "Camera resolution: " << img.cols << "x" << img.rows<<endl;

    std::thread th([&]{
        cout<<"Starting capture thread..."<<endl;
        while(true){
             TicToc tt;
             tt.tic();
             cap.grab();
             {
                std::lock_guard<std::mutex> lock(mtx);    
                cap.retrieve(buff_img);
             }
             cout<<"Grab time: "<<tt.toc()<<"ms"<<endl;
             if(buff_img.empty())continue;
        }
    });
#endif

    cout << "Abeo pi camera server\n";
    TCPServer server;
    server.run( PORT_NUM, preService, postService, clientService);
    cout << "Program terminated." << endl;
    return 0;
}
