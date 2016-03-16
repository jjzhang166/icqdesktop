//
//  VideoFrameMacos.h
//  ICQ
//
//  Created by IceSeer on 04/12/15.
//  Copyright © 2015 Mail.RU. All rights reserved.
//

#ifndef VideoFrameMacos_h
#define VideoFrameMacos_h

#include "VideoFrame.h"

namespace platform_macos {
    
    void setPanelAttachedAsChild(bool attach, QWidget& parent, QWidget& child);
    void setAspectRatioForWindow(QWidget& wnd, float aspectRatio);
    bool windowIsOverlapped(QWidget* frame);
    void setWindowPosition(QWidget& widget, const QWidget& parent, const bool top);
    
    void fadeIn(QWidget* wnd);
    void fadeOut(QWidget* wnd);
    
    class GraphicsPanelMacos {
    public:
        static platform_specific::GraphicsPanel* create(QWidget* parent, std::vector<QWidget*>& panels);
    };
    
}

#endif /* VideoFrameMacos_h */
