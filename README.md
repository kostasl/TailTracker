# 2 Photon Tail Tracker

Author: Konstantinos Lagogiannis 2020

A tail behaviour tracker for larval zebrafish video recorded during 2 photon imaging.
A camera is capturing 200x200 px grey scale images at 470 fps.

### How do I get set up? ###
* Dependencies
    - OpenCV
        -Install opencv and compile with flag -D WITH_QT=ON, improves experience and range of available functions.
        http://opencv.org/downloads.html. Latest known opencv version that worked with tracker is OpenCV 3.4.4
        -Random number generator library
    - QT5 or above
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
