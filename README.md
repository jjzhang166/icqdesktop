### Windows

ICQ for windows is compiled by Visual Studio and does not require any dependencies resolving.
The solution file is `icq.sln`

### MacOS

ICQ for Mac is compiled by Xcode and does not require any dependencies resolving.
Xcode project file is `mac/ICQ/ICQ.xcodeproj`

To configure Qt, we use the following command line:

./configure -static -release -confirm-license -opensource

Additionally, we patched Qt source code:

`qtbase/src/gui/text/qtextengine.cpp`
```cpp
if (Q_LIKELY(last_cluster != cluster)) {
  g.attributes[i].clusterStart = true;

  // fix up clusters so that the cluster indices will be monotonic
  // and thus we never return out-of-order indices
  while (last_cluster++ < cluster && str_pos < item_length)
      log_clusters[str_pos++] = last_glyph_pos;
  last_glyph_pos = i + glyphs_shaped;
  last_cluster = cluster;

  // hide characters that should normally be invisible
  switch (string[item_pos + str_pos]) {
    case QChar::LineFeed:
    case 0x000c: // FormFeed
    case QChar::CarriageReturn:
    case QChar::LineSeparator:
    case QChar::ParagraphSeparator:
        g.attributes[i].dontPrint = true;
        break;
    case QChar::SoftHyphen:
        if (!actualFontEngine->symbol) {
            // U+00AD [SOFT HYPHEN] is a default ignorable codepoint,
            // so we replace its glyph and metrics with ones for
            // U+002D [HYPHEN-MINUS] and make it visible if it appears at line-break
            g.glyphs[i] = actualFontEngine->glyphIndex('-');
            if (Q_LIKELY(g.glyphs[i] != 0)) {
                 QGlyphLayout tmp = g.mid(i, 1);
                 actualFontEngine->recalcAdvances(&tmp, 0);
            }
            g.attributes[i].dontPrint = true;
        }
        break;
    default:
        break;
  }
}
```

    *comment or remove 'case QChar::SoftHyphen:' with the body

### Linux

ICQ for Linux is compiled by Qt creator or qmake, so you need to prepare Qt environment first.

Qt source code may be obtained from the official Qt website:
http://download.qt.io/archive/qt/5.5/5.5.1/single/

To configure Qt, we use the following command line:

    ./configure -static -release -c++11 -no-opengl -no-harfbuzz \
      -no-audio-backend -system-xkbcommon-x11 -dbus-linked -no-openssl -qt-xcb \
      -confirm-license -opensource -qt-zlib -qt-libjpeg -qt-libpng

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

Additionally, we patched Qt source code:

1. `qtbase/src/gui/text/qtextlayout.cpp`
```cpp
if (lbh.currentPosition > 0 && lbh.currentPosition < end
                && attributes[lbh.currentPosition].lineBreak
                && eng->layoutData->string.at(lbh.currentPosition - 1).unicode() == QChar::SoftHyphen) {
```
   * comment or remove code block in `if` construction
   
2. `qtbase/src/gui/text/qtextengine.cpp`
```cpp
if (si->position + itemLength >= lineEnd
        && eng->layoutData->string.at(lineEnd - 1).unicode() == QChar::SoftHyphen)
        glyphs.attributes[glyphsEnd - 1].dontPrint = false;
```
   * change `false` to `true`
   
Then build the configured Qt using the followind command lines

    make
    make install

Now you can use qmake from installed Qt to generate Makefiles from our .pro files.

In order to install the prepared version of Qt in Qt Creator, use official Qt manuals.

We provides two `.pro` files containing build configuration for the project:

- `corelib/corelib/corelib.pro` - core library
- `gui/gui.pro` - gui (executable file)

In `gui.pro`, you may switch between `x32` and `x64` architecture, as well as external and internal resources linking (`CONFIG` variable).
You also have to provide a path to the directory, containing `corelib.a` file, which is obtained by building `corelib.pro` project (`CORELIB_PATH` variable).

Before building `gui.pro` file, you need to install `libqt4-dev` package and execute `config_linux.sh` in trunk root.

Additional library dependencies can be found in `gui.pro` or by following errors during the linkage process = )

The missing dependencies can be installed via your package manager.
