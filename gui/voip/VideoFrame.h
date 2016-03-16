//
//  WindowTest.h
//  ICQ
//
//  Created by IceSeer on 04/12/15.
//  Copyright © 2015 Mail.RU. All rights reserved.
//

#ifndef __WINDOW_TEST_H__
#define __WINDOW_TEST_H__

#include <QtWidgets/qwidget.h>

namespace platform_specific {
    
    class GraphicsPanel : public QWidget{
    public:
        static GraphicsPanel* create(QWidget* parent, std::vector<QWidget*>& panels);

        GraphicsPanel(QWidget* parent) : QWidget(parent) {}
        virtual ~GraphicsPanel() { }
        
        virtual WId frameId() const;

        virtual void addPanels(std::vector<QWidget*>& panels);
        virtual void clearPanels();
        virtual void fullscreenModeChanged(bool fullscreen);
    };
    
}

#endif//__WINDOW_TEST_H__
