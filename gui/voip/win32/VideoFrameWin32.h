#ifndef __VIDEO_FRAME_WIN32_H__
#define __VIDEO_FRAME_WIN32_H__

#include <QWidget>
#include "../VideoFrame.h"

namespace platform_win32 {
    
class GraphicsPanelWin32 : public platform_specific::GraphicsPanel { Q_OBJECT
public:
    GraphicsPanelWin32(QWidget* parent, std::vector<QWidget*>& panels);
    virtual ~GraphicsPanelWin32();
};

}

#endif//__VIDEO_FRAME_WIN32_H__