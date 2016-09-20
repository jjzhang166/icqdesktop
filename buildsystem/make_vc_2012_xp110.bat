if not exist "..\build" mkdir ..\build
cd ..\build
cmake .. -DCMAKE_BUILD_TYPE=Debug -G "Visual Studio 11 2012" -T v110_xp