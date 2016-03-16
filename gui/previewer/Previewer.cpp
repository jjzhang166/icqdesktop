#include "stdafx.h"

#include "PreviewWidget.h"

#include "Previewer.h"

#include "../main_window/MainWindow.h"
#include "../utils/InterConnector.h"

namespace
{
	std::unique_ptr<QWidget> PreviewWidget_;
}

namespace Previewer
{

	void ShowPreview(QPixmap &preview)
	{
		assert(!preview.isNull());

        const auto screen = Utils::InterConnector::instance().getMainWindow()->getScreen();
        const auto screenGeometry = QApplication::desktop()->screenGeometry(screen);

		PreviewWidget_.reset(new PreviewWidget(preview));

        PreviewWidget_->move(screenGeometry.topLeft());
        PreviewWidget_->resize(screenGeometry.size());
		PreviewWidget_->showFullScreen();
	}

	void ClosePreview()
	{
		PreviewWidget_.reset();
	}

}

namespace
{

}