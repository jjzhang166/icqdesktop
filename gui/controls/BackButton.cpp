#include "stdafx.h"
#include "BackButton.h"
#include "../utils/utils.h"

namespace Ui
{
    BackButton::BackButton(QWidget* _parent)
        : QPushButton(_parent)
    {
		setStyleSheet(Utils::LoadStyle(":/controls/controls_style.qss"));
        setObjectName("backButton");
    }
    BackButton::~BackButton()
    {
        //
    }
}
