#include "stdafx.h"
#include "VideoFrame.h"

#ifdef __APPLE__
    #include "macos/VideoFrameMacos.h"
#elif _WIN32
    #include "win32/VideoFrameWin32.h"
#else
    #error "video frame need to create"
#endif

platform_specific::GraphicsPanel* platform_specific::GraphicsPanel::create(QWidget* parent, std::vector<QWidget*>& panels) {
#ifdef __APPLE__
    return  platform_macos::GraphicsPanelMacos::create(parent, panels);
#elif _WIN32
    return  new platform_win32::GraphicsPanelWin32(parent, panels);
#endif
    return nullptr;
}

WId platform_specific::GraphicsPanel::frameId() const {
	return QWidget::winId();
}

void platform_specific::GraphicsPanel::addPanels(std::vector<QWidget*>& /*panels*/) {

}

void platform_specific::GraphicsPanel::clearPanels() {
	
}

void platform_specific::GraphicsPanel::fullscreenModeChanged(bool fullscreen) {
    
}
