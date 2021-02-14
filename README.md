# 2 Photon Tail Tracker

Author: Konstantinos Lagogiannis 2021

A tail behaviour tracker for larval zebrafish video recorded during 2 photon imaging.
A camera is capturing 200x200 px grey scale images at 470 fps.

### How do I get set up? ###
* Dependencies
    - OpenCV
        -Install opencv and compile with flag -D WITH_QT=ON, improves experience and range of available functions.
        http://opencv.org/downloads.html. Latest known opencv version that worked with tracker is OpenCV 3.4.4
        -Random number generator library
    - QT 5.15.2 was used  
    - GCC

Install prerequisites :

```
sudo apt-get install git libopencv-dev qt5-default g++ 
```

Building the tracker:

```
git clone https://github.com/dafishcode/TailTracker
cd TailTracker
qmake 2pTailTracker.pro
make
```


##  Usage 
I recommend using the [AppImage binary](https://github.com/kostasl/TailTracker/releases/download/V1-alpha-kappa/2p-muscope_Tail_Tracker-x86_64-k.AppImage) available in the Releases section, as this may minimize problems with dependencies. 
The app presents the user with a GUI to select input and output folders.
Once input video is selected, the background model is automatically calculated using the initial 2000 frames. 
To define a new spine location and length left click and hold for 1ses. A point and blue arrow will appear which while dragging it allows the the user to define length and initial direction of tail at the point of where the left mouse button is released.

Click start tracking to initiate the tracking. output to csv file is written during tracking. 

Command line options (not exhaustive) include :
 - help : Show all commandline options
 - outputdir  / o :  Folder where tracking data file will be saved in CSV text file
 - invideofile / v :  Behavioural Video file or directory of imagesequence to analyse 
 - FilterPixelNoise/ pn Filter Pixel Noise During Tracking (Note:This has major perf impact so use only when necessary due to pixel noise. BGProcessing does it by default)}
 
 
