#include "stdafx.h"
#include "BackButton.h"
#include "../utils/utils.h"

namespace Ui
{
    BackButton::BackButton(QWidget* parent)
        : QPushButton(parent)
    {
		setStyleSheet(Utils::LoadStyle(":/controls/back_button.qss", Utils::get_scale_coefficient(), true));
    }
    BackButton::~BackButton()
    {
        //
    }
}
