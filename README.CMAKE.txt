How to build with CMake on Windows
----------------------------------

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




How to build with CMake on MacOS
--------------------------------

From the root of project directory:
    mkdir build
    cd build

You can build project with Xcode or with make

XCode:
    cmake .. -G Xcode -DCMAKE_BUILD_TYPE=Debug (also you can set Release instead Debug)
    Open build\icq.sln and build

make:
    cmake .. -DCMAKE_BUILD_TYPE=Debug (also you can set Release instead Debug)
    make -j2
