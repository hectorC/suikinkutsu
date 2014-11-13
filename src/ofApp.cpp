


#include "ofApp.h"

float ofApp::yaw;
float ofApp::pitch;
float ofApp::roll;

float ofApp::offYaw;
float ofApp::offPitch;
float ofApp::offRoll;

float ofApp::rYaw;
float ofApp::rPitch;
float ofApp::rRoll;

Csound * ofApp::csound;
CsoundPerformanceThread * ofApp::csThread;

//--------------------------------------------------------------
void ofApp::setup(){
    
    ofSetFrameRate(60);
    ofBackground(0, 0, 0);
    
    // OpenCV image size
    widthGrab = 320;
    heightGrab = 240;
    
    widthWindow = ofGetWidth();
    heightWindow = ofGetHeight();
    
    bLearnBakground = true;
    showVideo = true;
    threshold = 17; // for Diff
    
    minArea = 3; // OpenCV min area blob
    maxArea = 53; // OpenCV max area blob
    
    // Leap image region of interest
    roiUpX = 48;
    roiUpY = 111;
    roiLoX = 266;
    roiLoY = 223;
    
    // Setup video player
    numMovie = 1;
    videoPlayer.loadMovie(ofToDataPath("assets/video_1.mov"));
    videoPlayer.play();
    videoPlayer.setLoopState(OF_LOOP_NONE);
    nextVid = false;
    
    // Setup Leap Motion Controller
    Leap::Controller::PolicyFlag addImagePolicy = (Leap::Controller::PolicyFlag)(Leap::Controller::POLICY_IMAGES | leapController.policyFlags());
    leapController.setPolicyFlags(addImagePolicy);
    
    gImageLeap1.allocate(640, 240); // Leap Motion image size
    gImageLeapRes.allocate(widthGrab, heightGrab);
    gImageBack.allocate(widthGrab, heightGrab);
    gImageDiff.allocate(widthGrab, heightGrab);
    
    // Setup LiquidFun
    box2d.init();
    box2d.setGravity(0, 0);
    box2d.setFPS(30.0);
    
    box2d.registerGrabbing();
    box2d.createBounds();
    
    ofColor partColor;
    partColor.setHex(0xffffff, 100);
    particles.setParticleFlag(b2_tensileParticle);
    particles.loadImage("particle32.png");
    particles.setup(box2d.getWorld(), 2000, 0.0, 6.0, 42.0, partColor); //setup(b2World * b2world, int maxCount, float lifetime, float radius, float particleSize, ofColor color);
    
    for (int i = 0; i < 2000; i++) {
        ofVec2f position = ofVec2f(ofRandom(100), ofRandom(ofGetHeight()));
        ofVec2f velocity = ofVec2f(0, 0);
        particles.createParticle(position, velocity);
    }
    
    // Setup Csound
    string csdFile = ofToDataPath("suikinkutsu.csd");
    
    string ssdir = ofToDataPath("assets", true);
    string flag = "SSDIR=" + ssdir;
    putenv((char*)flag.c_str());
    flag = "SADIR=" + ssdir;
    putenv((char*)flag.c_str());
    
    csGain = 0;
    
    csound = new Csound;
    csound->Compile((char*)csdFile.c_str());
    csThread = new CsoundPerformanceThread(csound->GetCsound());
    csThread->Play();
    
    MYFLT pfields[] = {10, 0, 14400, 1, 0};
    csound->ScoreEvent('i', pfields, 5);
    
    pfields[0] = 20;
    csound->ScoreEvent('i', pfields, 5);
    csound->ScoreEvent('i', pfields, 5);
    csound->ScoreEvent('i', pfields, 5);
    
    // setup tracker
    offYaw = 0;
    offPitch = 0;
    offRoll = 0;
}


void ofApp::razor_on_data(const float data[])
{
    rYaw = data[0];
    rPitch = data[1];
    rRoll = data[2];
    
    yaw = ofWrap(rYaw - offYaw, -180, 180);
    pitch = ofWrap(rPitch - offPitch, -180, 180);
    roll = ofWrap(rRoll - offRoll, -180, 180);
    
    csound->SetChannel("yaw", TWO_PI - ofDegToRad(yaw));
    csound->SetChannel("pitch", ofDegToRad(pitch));
    csound->SetChannel("roll", ofDegToRad(roll));
}

void ofApp::razor_on_error(const string &msg)
{
    cout << "  " << "ERROR: " << msg << endl;
}

//--------------------------------------------------------------
void ofApp::update(){
    
    leapFrame = leapController.frame();
    leapImages = leapFrame.images();
    leapCam1 = leapImages[0];
    
    for (int i=0; i<(leapCam1.width()*leapCam1.height()); i++) {
        gImageLeap1.getPixels()[i] = leapCam1.data()[i];
    }
    gImageLeap1.flagImageChanged();
    
//    gImageLeap1.getPixelsRef().resizeTo(gImageLeapRes.getPixelsRef());
    gImageLeapRes.scaleIntoMe(gImageLeap1);
    gImageLeapRes.flagImageChanged();
    
    // Capture image for background removal on first run or when the spacebar is pressed (see keyPressed() below)
    if (bLearnBakground == true){
        gImageBack = gImageLeapRes;
        bLearnBakground = false;
    }
    gImageDiff.absDiff(gImageBack, gImageLeapRes);
    
    // Brightness thresholding
    gImageDiff.threshold(threshold);
    gImageDiff.flagImageChanged();
    
    // Run the contour finder on the filtered image to find blobs
    contoursCV.findContours(gImageDiff, minArea, maxArea, 10, false);
    
    int numBlobs = contoursCV.nBlobs;
    
    // Remove all the invisible circles
    boxes.clear();
    
    for (int k=0; k<numBlobs; k++) {
        
        boxes.push_back(ofPtr<ofxBox2dCircle>(new ofxBox2dCircle));
        boxes.back().get()->setPhysics(1.0, 0, 0); // Parameters: float density, float bounce, float friction
        boxes.back().get()->setup(box2d.getWorld(), ofMap(contoursCV.blobs[k].centroid.x, roiUpX, roiLoX, 0, widthWindow), ofMap(contoursCV.blobs[k].centroid.y, roiUpY, roiLoY, 0, heightWindow), 40);
        boxes.back().get()->addRepulsionForce(1, 2, 0.5);
        
        // Send blob position data to Csound for the first 3 blobs
        if (k < 3) {
            string csPosX = "posx" + ofToString(k+1);
            string csPosY = "posy" + ofToString(k+1);
            
            csound->SetChannel(csPosX.c_str(), (double) ofMap(contoursCV.blobs[k].centroid.x, roiUpX, roiLoX, 0, 1));
            csound->SetChannel(csPosY.c_str(), csPitch[k]);
        }
    }
    
    // Fade out synthesized sounds if no blobs are present, fade in when present. Use a timer to filter out jitter.
    if (numBlobs > 0) {
        csGain = 4;
        ofResetElapsedTimeCounter();
    } else if (ofGetElapsedTimef() > 2) {
        csGain = 0;
    }
    
    csound->SetChannel("outgain", csGain);
    
    // Play next video and start new soundscape when video finished
    if (videoPlayer.getIsMovieDone() || nextVid) {
        
        numMovie++;
        if (numMovie > 3) numMovie = 1;
        
        string videoFile = ofToDataPath("assets/video_" + ofToString(numMovie) + ".mov");
        videoPlayer.loadMovie(videoFile);
        videoPlayer.play();
        
        csoundKillInstance(csound->GetCsound(), 10, NULL, 0, 0);
        csoundKillInstance(csound->GetCsound(), 20, NULL, 0, 0);
        
        MYFLT pfields[] = {10, 0, 14400, numMovie, 0};
        csound->ScoreEvent('i', pfields, 5);
        
        pfields[0] = 20;
        csound->ScoreEvent('i', pfields, 5);
        csound->ScoreEvent('i', pfields, 5);
        csound->ScoreEvent('i', pfields, 5);
        
        nextVid = false;
    }
    
    // Change the physics for scene number 3
    if(numMovie == 3) {
        
        box2d.setGravity(-5, 0);
        
        boxes.push_back(ofPtr<ofxBox2dCircle>(new ofxBox2dCircle));
        boxes.back().get()->setPhysics(1.0, 0, 0); //float density, float bounce, float friction
        boxes.back().get()->setup(box2d.getWorld(), 0, 0, 150);
        boxes.back().get()->addRepulsionForce(1, 2, 0.5);
        
        boxes.push_back(ofPtr<ofxBox2dCircle>(new ofxBox2dCircle));
        boxes.back().get()->setPhysics(1.0, 0, 0); //float density, float bounce, float friction
        boxes.back().get()->setup(box2d.getWorld(), 0, heightWindow, 150);
        boxes.back().get()->addRepulsionForce(1, 2, 0.5);
    } else {
        box2d.setGravity(0, 0);
    }
    
    box2d.update();
    videoPlayer.update();
}

//--------------------------------------------------------------
void ofApp::draw(){
    
    videoPlayer.draw(0, 0, widthWindow, heightWindow);
    
    particles.draw();
    
    if (showVideo) {
        gImageDiff.draw(gImageLeapRes.width,0);
        gImageLeapRes.draw(0, 0);
        
        ofSetHexColor(0xffffff);
        ofDrawBitmapString("minArea: " + ofToString(minArea) + " maxArea: " + ofToString(maxArea) + " Threshold: " + ofToString(threshold), 10, heightWindow - 40);
        ofDrawBitmapString("Yaw: " + ofToString(yaw) + " Pitch: " + ofToString(pitch) + " Roll: " + ofToString(roll), 10, heightWindow - 20);
    }
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    switch (key){
        case ' ':
            bLearnBakground = true;
            break;
        case '=':
            threshold++;
            if (threshold > 255) threshold = 255;
            ofLog() << "threshold: " << threshold;
            break;
        case '-':
            threshold--;
            if (threshold < 0) threshold = 0;
            ofLog() << "threshold: " << threshold;
            break;
        case 'z':
            minArea--;
            if (minArea <0) minArea = 0;
            ofLog() << "minArea: " << minArea;
            break;
        case 'x':
            minArea++;
            ofLog() << "minArea: " << minArea;
            break;
        case 'a':
            maxArea--;
            if (maxArea < minArea) maxArea = minArea;
            ofLog() << "maxArea: " << maxArea;
            break;
        case 's':
            maxArea++;
            ofLog() << "maxArea: " << maxArea;
            break;
        case 'i':
            showVideo = !showVideo;
            break;
        case 'f':
            ofToggleFullscreen();
            break;
        case 'c':
            offYaw = rYaw;
            offPitch = rPitch;
            offRoll = rRoll;
            break;
        case 't':
            delete razor;
            
            try
        {
            razor = new RazorAHRS(serial_port_name, razor_on_data, razor_on_error, RazorAHRS::YAW_PITCH_ROLL);
        }
            catch(runtime_error &e)
        {
            cout << "  " << (string("Could not create tracker: ") + string(e.what())) << endl;
            return 0;
        }
            break;
        case 'n':
            nextVid = true;
            break;
            
    }
    
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){
    
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){
    
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){
    
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
    
    ofLog() << x << " " << y; // For finding the ROI, report mouse click x and y
    
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){
    
}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){
    
}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){
    
}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){
    
}
