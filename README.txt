*Windows*

ICQ for windows is compiled by Visual Studio.
The solution file is gui/gui.vcxproj
To build it, you need to provide 3 environment variables. 
QT_PATH - path to dir containing the built QT version (dir with folders 'bin', 'include', 'lib')
BOOST_INCLUDE - path for boost inclusions (dir containing folder 'boost')
BOOST_LIB - path to boost libraries

*MacOS*

ICQ for mac is compiled by Xcode and does not require any dependencies resolving.
Xcode project file is mac/ICQ/ICQ.xcodeproj

*Linux*

ICQ for linux is compiled by QT creator, so you need to prepare QT environment first.
QT may be installed from the official installer (see QT official website) or compiled from source code and linked statically/dynamically.

To configure QT, we use the following command line:

./configure -static -release -c++11 -no-opengl -no-harfbuzz -no-audio-backend -system-xkbcommon-x11 -dbus-linked -no-openssl -qt-xcb -confirm-license -opensource -qt-zlib -qt-libjpeg -qt-libpng

You will also require the following packages, which can be obtained via your package manager:

libfontconfig1-dev
libfreetype6-dev
libx11-dev
libxext-dev
libxfixes-dev
libxi-dev
libxrender-dev
libxcb1-dev
libx11-xcb-dev
libxcb-glx0-dev

Additionally, we patched QT source code:

1. qtbase/src/gui/text/qtextlayout.cpp:

if (lbh.currentPosition > 0 && lbh.currentPosition < end
                && attributes[lbh.currentPosition].lineBreak
                && eng->layoutData->string.at(lbh.currentPosition - 1).unicode() == QChar::SoftHyphen) {

* comment or remove code block in 'if' construction;

2. qtbase/src/gui/text/qtextengine.cpp

if (si->position + itemLength >= lineEnd
        && eng->layoutData->string.at(lineEnd - 1).unicode() == QChar::SoftHyphen)
        glyphs.attributes[glyphsEnd - 1].dontPrint = false;

* change 'false' to 'true'


In order to install the prepared version of QT in QT Creator, use official QT manuals.

We provides 2 .pro files containing build configuration for the project:

corelib/corelib/corelib.pro - core library
gui/gui.pro - gui (executable file)

In gui.pro, you may switch between x32 and x64 architecture, as well as external and internal resources linking (CONFIG available).
You also have to provide a path to the directory, containing corelib.a file, which is obtained by building corelib.pro project (CORELIB_PATH variable).

Before building gui.pro file, you need to install libqt4-dev package and execute config_linux.sh in trunk root.

Additional library dependencies can be found in gui.pro or by following errors during the linkage process = )
The dependencies can be installed via your package manager
