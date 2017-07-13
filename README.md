### Windows

Download external libraries here https://files.icq.net/get/s66y7gIKtiejxAxSFxWPIn5935227e1ac and extract it to ./external folder

From the root of project directory:
    mkdir build
    cd build

You can build project with Visual Studio 2012 or with NMake

Visual Studio 2012:
    cmake .. -G "Visual Studio 11 2012" -T "v110_xp" -DCMAKE_BUILD_TYPE=Debug (also you can set Release instead Debug)
    Open build\icq.sln and build

NMake:
    cmake .. -G "NMake Makefiles" -T "v110_xp" -DCMAKE_BUILD_TYPE=Debug (also you can set Release instead Debug)
    nmake


### MacOS

Download external libraries here https://files.icq.net/get/s66y7gIKtiejxAxSFxWPIn5935227e1ac and extract it to ./external folder

From the root of project directory:
    mkdir build
    cd build

You can build project with Xcode or with make

XCode:
    cmake .. -G Xcode -DCMAKE_BUILD_TYPE=Debug (also you can set Release instead Debug)
    Open build\icq.xcodeproj and build

make:
    cmake .. -DCMAKE_BUILD_TYPE=Debug (also you can set Release instead Debug)
    make


### Linux

Download external libraries here https://files.icq.net/get/s66y7gIKtiejxAxSFxWPIn5935227e1ac and extract it to ./external folder

In order to build ICQ execute the following command line (change -DLINUX_ARCH=32 to -DLINUX_ARCH=64 for 64bit binaries):

mkdir build && cd build && cmake .. -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DLINUX_ARCH=32 && make
