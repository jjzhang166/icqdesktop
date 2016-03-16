sudo ./configure -static -release -no-glib -pkg-config -no-framework -c++11 -no-warnings-are-errors -no-opengl -no-audio-backend -no-openssl -no-harfbuzz -no-gstreamer -confirm-license -opensource -skip qtconnectivity -skip qtandroidextras -skip qtquick1 -skip qtquickcontrols -skip qtsensors -skip qtwayland -skip qtwebchannel -skip qtwebengine -skip qtwebkit -skip qtwebkit-examples -skip qtwebsockets -skip qtwinextras

sudo make -j12

sudo make -j12 install

then go to /usr/local

patch qt for remove SoftHyphens

1. qtbase/src/gui/text/qtextlayout.cpp:

if (lbh.currentPosition > 0 && lbh.currentPosition < end
                && attributes[lbh.currentPosition].lineBreak
                && eng->layoutData->string.at(lbh.currentPosition - 1).unicode() == QChar::SoftHyphen) {

комментируем весь блок if

2. qtbase/src/gui/text/qtextengine.cpp

if (si->position + itemLength >= lineEnd
        && eng->layoutData->string.at(lineEnd - 1).unicode() == QChar::SoftHyphen)
        glyphs.attributes[glyphsEnd - 1].dontPrint = false;

false меняем на true