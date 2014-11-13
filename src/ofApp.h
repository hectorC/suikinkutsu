#pragma once

#include "ofMain.h"
#include "ofxOpenCv.h"
#include "ofxLiquidFun.h"
#include "ofxOsc.h"
#include "Leap.h"
#include "RazorAHRS.h"
#include <csound.hpp>
#include <csPerfThread.hpp>

#define numHues 2
#define maxObjects 10
#define variance 5

class ofApp : public ofBaseApp{
    
public:
    void                            setup();
    void                            update();
    void                            draw();
    
    void                            keyPressed(int key);
    void                            keyReleased(int key);
    void                            mouseMoved(int x, int y );
    void                            mouseDragged(int x, int y, int button);
    void                            mousePressed(int x, int y, int button);
    void                            mouseReleased(int x, int y, int button);
    void                            windowResized(int w, int h);
    void                            dragEvent(ofDragInfo dragInfo);
    void                            gotMessage(ofMessage msg);
    
    ofxCvGrayscaleImage             gImageBack, gImageDiff, gImageLeap1, gImageLeapRes;
    ofxCvContourFinder              contoursCV;
    
    int                             widthGrab, heightGrab;
    int                             widthWindow, heightWindow;
    
    int                             threshold, minArea, maxArea;
    bool                            bLearnBakground;
    bool                            showVideo;
    int                             roiUpX, roiUpY, roiLoX, roiLoY;
    
    ofVideoPlayer                   videoPlayer;
    int                             numMovie;
    bool                            nextVid;
    
    
    // LiquidFun
    ofxBox2d                        box2d;
    vector<ofPtr<ofxBox2dCircle> >  boxes;
    ofxBox2dParticleSystem          particles;
    
    // Leap Motion
    Leap::Controller                leapController;
    Leap::Frame leapFrame;
    Leap::ImageList                 leapImages;
    Leap::Image                     leapCam1;
    
    
    // Razor head-tracker
    const string                    serial_port_name = "/dev/tty.FireFly-E4B5-SPP";
    RazorAHRS                       *razor;
    static void                     razor_on_data(const float data[]);
    static void                     razor_on_error(const string &msg);
    static float                    yaw, pitch, roll, offYaw, offPitch, offRoll, rYaw, rPitch, rRoll;
    
    // Csound
    static Csound                   *csound;
    static CsoundPerformanceThread  *csThread;
    int                             csPitch[4] = {0.125, 0.5, 1, 1.5};
    double                          csGain;
    
};
