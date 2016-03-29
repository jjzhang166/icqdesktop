#include "stdafx.h"

#include "../../utils/utils.h"
#include "GeneralSettingsWidget.h"
#include "../../controls/TextEmojiWidget.h"
#include "../../../common.shared/version_info_constants.h"
#include "../../controls/TextEditEx.h"
#include "../../controls/LineEditEx.h"
#include "../../utils/gui_coll_helper.h"
#include "../../utils/InterConnector.h"
#include "../../core_dispatcher.h"
#include "../contact_list/ContactListModel.h"
#include "../../gui_settings.h"
#include "../contact_list/contact_profile.h"
#include "../../controls/CustomButton.h"

using namespace Ui;

void GeneralSettingsWidget::Creator::initContactUs(QWidget* parent, std::map<std::string, Synchronizator> &/*collector*/)
{
    static std::map<QString, QString> filesToSend;

    auto scroll_area = new QScrollArea(parent);
    scroll_area->setWidgetResizable(true);
    Utils::grabTouchWidget(scroll_area->viewport(), true);

    auto scroll_area_content = new QWidget(scroll_area);
    scroll_area_content->setGeometry(QRect(0, 0, Utils::scale_value(800), Utils::scale_value(600)));
    Utils::grabTouchWidget(scroll_area_content);

    auto scroll_area_content_layout = new QVBoxLayout(scroll_area_content);
    scroll_area_content_layout->setSpacing(0);
    scroll_area_content_layout->setAlignment(Qt::AlignTop);
    scroll_area_content_layout->setContentsMargins(Utils::scale_value(48), 0, 0, Utils::scale_value(48));

    scroll_area->setWidget(scroll_area_content);

    auto layout = new QHBoxLayout(parent);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(scroll_area);

    addHeader(scroll_area, scroll_area_content_layout, QT_TRANSLATE_NOOP("contactus_page", "Contact us"));
    {
        static quint64 filesSizeLimiter = 0;

        TextEditEx *suggestioner = nullptr;
        TextEmojiWidget *suggestionMinSizeError = nullptr;
        TextEmojiWidget *suggestionMaxSizeError = nullptr;
        QWidget *suggestionerCover = nullptr;
        QWidget *filer = nullptr;
        QWidget *filerCover = nullptr;
        TextEmojiWidget *filerTotalSizeError = nullptr;
        TextEmojiWidget *filerSizeError = nullptr;
        LineEditEx *emailer = nullptr;
        QWidget *emailerCover = nullptr;
        TextEmojiWidget *emailerError = nullptr;

        auto successPage = new QWidget(scroll_area);
        auto sendingPage = new QWidget(scroll_area);

        Utils::grabTouchWidget(successPage);
        Utils::grabTouchWidget(sendingPage);

        auto sendingPageLayout = new QVBoxLayout(sendingPage);
        sendingPageLayout->setContentsMargins(0, Utils::scale_value(28), Utils::scale_value(48), 0);
        sendingPageLayout->setSpacing(Utils::scale_value(8));
        sendingPageLayout->setAlignment(Qt::AlignTop);
        {
            suggestioner = new TextEditEx(sendingPage, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(18), QColor(0x28, 0x28, 0x28), true, false);
            Testing::setAccessibleName(suggestioner, "feedback_suggestioner");

            Utils::grabTouchWidget(suggestioner);
            suggestioner->setContentsMargins(0, Utils::scale_value(24), 0, 0);
            suggestioner->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Minimum);
            suggestioner->setFixedWidth(Utils::scale_value(440));
            suggestioner->setMinimumHeight(Utils::scale_value(88));
            suggestioner->setMaximumHeight(Utils::scale_value(252));
            suggestioner->setPlaceholderText(QT_TRANSLATE_NOOP("contactus_page","Your comments or suggestions"));
            suggestioner->setAutoFillBackground(false);
            suggestioner->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
            suggestioner->setTextInteractionFlags(Qt::TextEditable | Qt::TextEditorInteraction);
            {
                QString sgs = "QWidget[normal=\"true\"] { font-family: \"%FONT_FAMILY%\"; font-weight: %FONT_WEIGHT%; font-size: 18dip; color: #282828; background: #ffffff; border-radius: 8dip; border-color: #d7d7d7; border-width: 1dip; border-style: solid; padding-left: 12dip; padding-right: 0dip; padding-top: 10dip; padding-bottom: 10dip; } QWidget[normal=\"false\"] { font-family: \"%FONT_FAMILY%\"; font-weight: %FONT_WEIGHT%; font-size: 18dip; color: #e30f04; background: #ffffff; border-radius: 8dip; border-color: #e30f04; border-width: 1dip; border-style: solid; padding-left: 12dip; padding-right: 0dip; padding-top: 10dip; padding-bottom: 10dip; }";
                suggestioner->setProperty("normal", true);
                Utils::ApplyStyle(suggestioner, sgs);
            }
            {
                suggestionerCover = new QWidget(suggestioner);
                Utils::grabTouchWidget(suggestionerCover);
                suggestionerCover->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
                suggestionerCover->setFixedSize(suggestioner->minimumWidth(), suggestioner->minimumHeight());
                suggestionerCover->setStyleSheet("background-color: #7fffffff; border: none;");
                suggestionerCover->setVisible(false);
            }
            {
                GetDisconnector()->add("suggestioner", scroll_area->connect(suggestioner->document(), &QTextDocument::contentsChanged, [=]()
                {
                    auto nh = suggestioner->document()->size().height() + Utils::scale_value(12 + 10 + 2); // paddings + border_width*2
                    if (nh > suggestioner->maximumHeight())
                        suggestioner->setMinimumHeight(suggestioner->maximumHeight());
                    else if (nh > Utils::scale_value(88))
                        suggestioner->setMinimumHeight(nh);
                    else
                        suggestioner->setMinimumHeight(Utils::scale_value(88));
                }));
            }
            sendingPageLayout->addWidget(suggestioner);

            suggestionMinSizeError = new TextEmojiWidget(suggestioner, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(16), QColor("#e30f04"), Utils::scale_value(12));
            Utils::grabTouchWidget(suggestionMinSizeError);
            suggestionMinSizeError->setText(QT_TRANSLATE_NOOP("contactus_page", "Message is too short"));
            suggestionMinSizeError->setVisible(false);
            sendingPageLayout->addWidget(suggestionMinSizeError);

            suggestionMaxSizeError = new TextEmojiWidget(suggestioner, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(16), QColor("#e30f04"), Utils::scale_value(12));
            Utils::grabTouchWidget(suggestionMaxSizeError);
            suggestionMaxSizeError->setText(QT_TRANSLATE_NOOP("contactus_page", "Message is too long"));
            suggestionMaxSizeError->setVisible(false);
            sendingPageLayout->addWidget(suggestionMaxSizeError);
        }
        {
            filer = new QWidget(sendingPage);
            Testing::setAccessibleName(filer, "feedback_filer");

            Utils::grabTouchWidget(filer);
            auto fcl = new QVBoxLayout(filer);
            filer->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Preferred);
            filer->setFixedWidth(Utils::scale_value(440));
            fcl->setContentsMargins(0, Utils::scale_value(4), 0, 0);
            fcl->setSpacing(Utils::scale_value(8));
            {
                filerCover = new QWidget(filer);
                Utils::grabTouchWidget(filerCover);
                filerCover->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
                filerCover->setFixedSize(filer->minimumWidth(), filer->minimumHeight());
                filerCover->setStyleSheet("background-color: #7fffffff; border: none;");
                filerCover->setVisible(false);
            }
            {
                filerSizeError = new TextEmojiWidget(filer, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(16), QColor("#e30f04"), Utils::scale_value(12));
                Utils::grabTouchWidget(filerSizeError);
                filerSizeError->setText(QT_TRANSLATE_NOOP("contactus_page", "File size exceeds 1 MB"));
                filerSizeError->setVisible(false);
                fcl->addWidget(filerSizeError);

                filerTotalSizeError = new TextEmojiWidget(filer, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(16), QColor("#e30f04"), Utils::scale_value(12));
                Utils::grabTouchWidget(filerTotalSizeError);
                filerTotalSizeError->setText(QT_TRANSLATE_NOOP("contactus_page", "Attachments size exceeds 25 MB"));
                filerTotalSizeError->setVisible(false);
                fcl->addWidget(filerTotalSizeError);

                auto bsc = [=](QString fileName, QString fileSize, QString realName, qint64 realSize)
                {
                    auto bf = new QWidget(filer);
                    auto bfl = new QHBoxLayout(bf);
                    Utils::grabTouchWidget(bf);
                    bf->setObjectName(fileName);
                    bf->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
                    bf->setFixedWidth(Utils::scale_value(440));
                    bf->setFixedHeight(Utils::scale_value(32));
                    bfl->setContentsMargins(Utils::scale_value(12), 0, Utils::scale_value(12), 0);
                    bfl->setSpacing(Utils::scale_value(12));
                    bfl->setAlignment(Qt::AlignVCenter);
                    {
                        QString bfs = "background: #ebebeb; border-radius: 8dip; border-color: #d7d7d7; border-style: none; padding-left: 15dip; padding-right: 15dip; padding-top: 12dip; padding-bottom: 10dip;";
                        Utils::ApplyStyle(bf, bfs);
                    }
                    {
                        auto fns = new QWidget(bf);
                        auto fnsl = new QHBoxLayout(fns);
                        Utils::grabTouchWidget(fns);
                        fns->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
                        fnsl->setContentsMargins(0, 0, 0, 0);
                        fnsl->setSpacing(0);
                        fnsl->setAlignment(Qt::AlignLeft);
                        {
                            auto fn = new QLabel(fns);
                            Utils::grabTouchWidget(fn);
                            fn->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Preferred);
                            fn->setText(fileName);
                            {
                                QString fns = "font-family: \"%FONT_FAMILY%\"; font-weight: %FONT_WEIGHT%; font-size: 16dip; color: #282828; padding: 0;";
                                Utils::ApplyStyle(fn, fns);
                            }
                            fnsl->addWidget(fn);

                            auto fs = new QLabel(fns);
                            Utils::grabTouchWidget(fs);
                            fs->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Preferred);
                            fs->setText(QString(" - %1").arg(fileSize));
                            {
                                QString fss = "font-family: \"%FONT_FAMILY%\"; font-weight: %FONT_WEIGHT%; font-size: 16dip; color: #696969; padding: 0;";
                                Utils::ApplyStyle(fs, fss);
                            }
                            fnsl->addWidget(fs);
                        }
                        bfl->addWidget(fns);

                        auto d = new QPushButton(bf);
                        d->setFlat(true);
                        d->setStyleSheet("QPushButton { border-image: url(:/resources/controls_cotext_close_200_active.png); } QPushButton:hover { border-image: url(:/resources/controls_cotext_close_200_hover.png); }");
                        d->setCursor(Qt::PointingHandCursor);
                        d->setFixedSize(Utils::scale_value(12), Utils::scale_value(12));
                        GetDisconnector()->add("filer", filer->connect(d, &QPushButton::clicked, [=]()
                        {
                            filesToSend.erase(bf->objectName());
                            filesSizeLimiter -= realSize;
                            filerSizeError->setVisible(false);
                            filerTotalSizeError->setVisible(false);
                            bf->setVisible(false);
                            delete bf;
                        }));
                        bfl->addWidget(d);
                    }
                    fcl->insertWidget(0, bf); // always insert at the top
                };

                auto sc = new QWidget(sendingPage);
                auto scl = new QHBoxLayout(sc);
                Utils::grabTouchWidget(sc);
                sc->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
                sc->setFixedWidth(Utils::scale_value(440));
                sc->setCursor(Qt::PointingHandCursor);
                scl->setContentsMargins(0, Utils::scale_value(0), 0, 0);
                scl->setSpacing(Utils::scale_value(12));
                {
                    auto attachFilesRoutine = [=]()
                    {
                        filerSizeError->setVisible(false);
                        filerTotalSizeError->setVisible(false);
#ifdef __linux__
                        QWidget *parentForDialog = nullptr;
#else
                        QWidget *parentForDialog = scroll_area;
#endif //__linux__
                        static auto dd = QDir::homePath();
                        QFileDialog d(parentForDialog);
                        d.setDirectory(dd);
                        d.setFileMode(QFileDialog::ExistingFiles);
                        d.setNameFilter(QT_TRANSLATE_NOOP("contactus_page", "Images (*.jpg *.jpeg *.png *.bmp *.gif)"));
                        QStringList fileNames;
                        if (d.exec())
                        {
                            dd = d.directory().absolutePath();
                            auto fls = d.selectedFiles();
                            for (auto f: fls)
                            {
                                const auto rf = f;
                                const auto rfs = QFileInfo(f).size();

                                if ((rfs / 1024. / 1024.) > 1)
                                {
                                    filerSizeError->setVisible(true);
                                    continue;
                                }

                                filesSizeLimiter += rfs;
                                if ((filesSizeLimiter / 1024. / 1024.) > 25)
                                {
                                    filesSizeLimiter -= rfs;
                                    filerTotalSizeError->setVisible(true);
                                    break;
                                }

                                double fs = rfs / 1024.f;
                                QString fss;
                                if (fs < 100)
                                {
                                    fss = QString("%1 %2").arg(QString::number(fs, 'f', 2)). arg(QT_TRANSLATE_NOOP("contactus_page", "KB"));
                                }
                                else
                                {
                                    fs /= 1024.;
                                    fss = QString("%1 %2").arg(QString::number(fs, 'f', 2)). arg(QT_TRANSLATE_NOOP("contactus_page", "MB"));
                                }

                                auto ls = f.lastIndexOf('/');
                                if (ls < 0 || ls >= f.length())
                                    ls = f.lastIndexOf('\\');
                                auto fsn = f.remove(0, ls + 1);

                                if (filesToSend.find(fsn) == filesToSend.end())
                                {
                                    //filerSizeError->setVisible(false);
                                    //filerTotalSizeError->setVisible(false);
                                    filesToSend.insert(std::make_pair(fsn, rf));
                                    bsc(fsn, fss, rf, rfs);
                                }
                                else
                                {
                                    filesSizeLimiter -= rfs;
                                }
                            }
                        }
                    };

                    auto b = new QPushButton(sc);
                    b->setFlat(true);
                    const QString addImageStyle = "QPushButton { border-image: url(:/resources/controls_cotext_addimg_100_active.png); } QPushButton:hover { border-image: url(:/resources/controls_cotext_addimg_100_hover.png); }";
                    b->setStyleSheet(Utils::ScaleStyle(addImageStyle, Utils::get_scale_coefficient()));
                    b->setFixedSize(Utils::scale_value(24), Utils::scale_value(24));
                    GetDisconnector()->add("attach/button", scroll_area->connect(b, &QPushButton::clicked, attachFilesRoutine));
                    scl->addWidget(b);

                    auto w = new TextEmojiWidget(sc, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(16), QColor("#579e1c"), Utils::scale_value(16));
                    Utils::grabTouchWidget(w);
                    w->setText(QT_TRANSLATE_NOOP("contactus_page", "Add screenshot"));
                    GetDisconnector()->add("attach/text", scroll_area->connect(w, &TextEmojiWidget::clicked, attachFilesRoutine));
                    scl->addWidget(w);
                }
                fcl->addWidget(sc);
            }
            sendingPageLayout->addWidget(filer);
        }
        {
            emailer = new LineEditEx(sendingPage);
            {
                auto f = QFont(Utils::appFontFamily(Utils::FontsFamily::SEGOE_UI));
                f.setPixelSize(Utils::scale_value(18));
                emailer->setFont(f);
            }

            emailer->setContentsMargins(0, Utils::scale_value(16), 0, 0);
            emailer->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Preferred);
            emailer->setFixedWidth(Utils::scale_value(440));
            emailer->setPlaceholderText(QT_TRANSLATE_NOOP("contactus_page", "Your Email"));
            emailer->setAutoFillBackground(false);
            emailer->setText(get_gui_settings()->get_value(settings_feedback_email, QString("")));
            {
                QString ms = "QWidget[normal=\"true\"] { font-family: \"%FONT_FAMILY%\"; font-weight: %FONT_WEIGHT%; font-size: 18dip; color: #282828; background: #ffffff; border-radius: 8dip; border-color: #d7d7d7; border-width: 1dip; border-style: solid; padding-left: 12dip; padding-right: 12dip; padding-top: 10dip; padding-bottom: 10dip; } QWidget[normal=\"false\"] { font-family: \"%FONT_FAMILY%\"; font-weight: %FONT_WEIGHT%; font-size: 18dip; color: #e30f04; background: #ffffff; border-radius: 8dip; border-color: #e30f04; border-width: 1dip; border-style: solid; padding-left: 12dip; padding-right: 12dip; padding-top: 10dip; padding-bottom: 10dip; }";
                emailer->setProperty("normal", true);
                Utils::ApplyStyle(emailer, ms);
            }
            Testing::setAccessibleName(emailer, "feedback_email");

            {
                emailerCover = new QWidget(emailer);
                Utils::grabTouchWidget(emailerCover);
                emailerCover->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
                emailerCover->setFixedSize(emailer->minimumWidth(), emailer->minimumHeight());
                emailerCover->setStyleSheet("background-color: #7fffffff; border: none;");
                emailerCover->setVisible(false);
            }
            sendingPageLayout->addWidget(emailer);

            emailerError = new TextEmojiWidget(sendingPage, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(16), QColor("#e30f04"), Utils::scale_value(12));
            Utils::grabTouchWidget(emailerError);
            emailerError->setText(QT_TRANSLATE_NOOP("contactus_page","Please enter a valid email address"));
            emailerError->setVisible(false);
            sendingPageLayout->addWidget(emailerError);

            GetDisconnector()->add("emailer/checking", connect(&Utils::InterConnector::instance(), &Utils::InterConnector::generalSettingsShow, [=](int type)
            {
                if ((Utils::CommonSettingsType)type == Utils::CommonSettingsType::CommonSettingsType_ContactUs && !Utils::isValidEmailAddress(emailer->text()))
                    emailer->setText("");
                if (!emailer->property("normal").toBool())
                    Utils::ApplyPropertyParameter(emailer, "normal", true);
                emailerError->setVisible(false);
            }));
        }
        {
            auto sp = new QWidget(sendingPage);
            auto spl = new QHBoxLayout(sp);
            Utils::grabTouchWidget(sp);
            sp->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred);
            spl->setContentsMargins(0, Utils::scale_value(16), 0, 0);
            spl->setSpacing(Utils::scale_value(12));
            spl->setAlignment(Qt::AlignLeft);
            {
                auto sendButton = new QPushButton(sp);
                sendButton->setFlat(true);
                sendButton->setText(QT_TRANSLATE_NOOP("contactus_page", "Send"));
                sendButton->setCursor(Qt::PointingHandCursor);
                sendButton->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Fixed);
                sendButton->setMinimumWidth(Utils::scale_value(100));
                sendButton->setFixedHeight(Utils::scale_value(40));
                Testing::setAccessibleName(sendButton, "send_feedback");

                {
                    Utils::ApplyStyle(sendButton, (suggestioner->getPlainText().length() && emailer->text().length()) ? main_button_style : disable_button_style);
                }
                spl->addWidget(sendButton);
                auto updateSendButton = [=](bool state)
                {
                    Utils::ApplyStyle(sendButton, (state) ? main_button_style : disable_button_style);
                };
                GetDisconnector()->add("suggestioner/sendbutton", scroll_area->connect(suggestioner->document(), &QTextDocument::contentsChanged, [=]()
                {
                    bool state = suggestioner->getPlainText().length();
                    if (state && suggestioner->property("normal").toBool() != state)
                    {
                        Utils::ApplyPropertyParameter(suggestioner, "normal", state);
                        suggestionMinSizeError->setVisible(false);
                        suggestionMaxSizeError->setVisible(false);
                    }
                    updateSendButton(state && emailer->text().length());
                }));
                GetDisconnector()->add("emailer/sendbutton", scroll_area->connect(emailer, &QLineEdit::textChanged, [=](const QString &)
                {
                    bool state = emailer->text().length();
                    if (state && emailer->property("normal").toBool() != state)
                    {
                        Utils::ApplyPropertyParameter(emailer, "normal", state);
                        emailerError->setVisible(false);
                    }
                    updateSendButton(suggestioner->getPlainText().length() && state);
                }));

                auto errorOccuredSign = new TextEmojiWidget(sp, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(16), QColor("#e30f04"), Utils::scale_value(16));
                Utils::grabTouchWidget(errorOccuredSign);
                errorOccuredSign->setText(QT_TRANSLATE_NOOP("contactus_page", "Error occured, try again later"));
                errorOccuredSign->setVisible(false);
                spl->addWidget(errorOccuredSign);

                auto sendSpinner = new QLabel(sp);
                auto abm = new QMovie(":/resources/gifs/r_spiner200.gif");
                Utils::grabTouchWidget(sendSpinner);
                sendSpinner->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
                sendSpinner->setContentsMargins(0, 0, 0, 0);
                sendSpinner->setFixedSize(Utils::scale_value(40), Utils::scale_value(40));
                abm->setScaledSize(QSize(Utils::scale_value(40), Utils::scale_value(40)));
                abm->start();
                sendSpinner->setMovie(abm);
                sendSpinner->setVisible(false);
                spl->addWidget(sendSpinner);

                auto sendingSign = new TextEmojiWidget(sp, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(16), QColor("#579e1c"), Utils::scale_value(16));
                Utils::grabTouchWidget(sendingSign);
                sendingSign->setText(QT_TRANSLATE_NOOP("contactus_page", "Sending..."));
                sendingSign->setVisible(false);
                spl->addWidget(sendingSign);

                GetDisconnector()->add("feedback/send", scroll_area->connect(sendButton, &QPushButton::pressed, [=]()
                {
                    get_gui_settings()->set_value(settings_feedback_email, emailer->text());

                    const auto sb = suggestioner->property("normal").toBool();
                    const auto eb = emailer->property("normal").toBool();
                    if (!sb)
                        suggestioner->setProperty("normal", true);
                    if (!eb)
                        emailer->setProperty("normal", true);
                    suggestionMinSizeError->setVisible(false);
                    suggestionMaxSizeError->setVisible(false);
                    emailerError->setVisible(false);
                    filerSizeError->setVisible(false);
                    filerTotalSizeError->setVisible(false);
                    auto sg = suggestioner->getPlainText();
                    sg.remove(' ');
                    if (!sg.length())
                    {
                        suggestioner->setProperty("normal", false);
                    }
                    else if (suggestioner->getPlainText().length() < 1)
                    {
                        suggestioner->setProperty("normal", false);
                        suggestionMinSizeError->setVisible(true);
                    }
                    else if (suggestioner->getPlainText().length() > 2048)
                    {
                        suggestioner->setProperty("normal", false);
                        suggestionMaxSizeError->setVisible(true);
                    }
                    else if (!Utils::isValidEmailAddress(emailer->text()))
                    {
                        emailer->setProperty("normal", false);
                        emailerError->setVisible(true);
                    }
                    else
                    {
                        sendButton->setVisible(false);
                        errorOccuredSign->setVisible(false);
                        sendSpinner->setVisible(true);
                        sendingSign->setVisible(true);

                        suggestioner->setEnabled(false);
                        filer->setEnabled(false);
                        emailer->setEnabled(false);

                        suggestionerCover->setFixedSize(suggestioner->minimumWidth(), suggestioner->minimumHeight());
                        filerCover->setFixedSize(filer->contentsRect().width(), filer->contentsRect().height());
                        emailerCover->setFixedSize(emailer->minimumWidth(), emailer->minimumHeight());

                        suggestionerCover->setVisible(true);
                        filerCover->setVisible(true);
                        emailerCover->setVisible(true);

                        filerCover->raise();

                        Logic::GetContactListModel()->get_contact_profile("", [=](Logic::profile_ptr _profile, int32_t /*error*/)
                        {
                            if (_profile)
                            {
                                core::coll_helper col(GetDispatcher()->create_collection(), true);
                                col.set_value_as_string("url", "https://help.mail.ru/icqdesktop-support/all");
                                col.set_value_as_string("fb.screen_resolution", (QString("%1x%2").arg(qApp->desktop()->screenGeometry().width()).arg(qApp->desktop()->screenGeometry().height())).toStdString());
                                col.set_value_as_string("fb.referrer", "icq");
                                {
                                    auto icqv = QString("ICQ %1").arg(VERSION_INFO_STR);
                                    auto osv = QSysInfo::prettyProductName();
                                    if (!osv.length() || osv == "unknown")
                                        osv = QString("%1 %2 (%3 %4)").arg(QSysInfo::productType()).arg(QSysInfo::productVersion()).arg(QSysInfo::kernelType()).arg(QSysInfo::kernelVersion());
                                    auto concat = QString("%1 %2 icq:%3").arg(osv).arg(icqv).arg(_profile ? _profile->get_aimid() : "");
                                    col.set_value_as_string("fb.question.3004", concat.toStdString());
                                    col.set_value_as_string("fb.question.159", osv.toStdString());
                                    col.set_value_as_string("fb.question.178", build::is_debug() ? "beta" : "live");
                                    if (platform::is_apple())
                                        col.set_value_as_string("fb.question.3005", "OSx");
                                    else if (platform::is_windows())
                                        col.set_value_as_string("fb.question.3005", "Windows");
                                    else if (platform::is_linux())
                                        col.set_value_as_string("fb.question.3005", "Linux");
                                    else
                                        col.set_value_as_string("fb.question.3005", "Unknown");
                                }
                                if (_profile)
                                {
                                    auto fn = QString("%1%2%3").arg(_profile->get_first_name()).arg(_profile->get_first_name().length() ? " " : "").arg(_profile->get_last_name());
                                    if (!fn.length())
                                    {
                                        if (_profile->get_contact_name().length())
                                            fn = _profile->get_contact_name();
                                        else if (_profile->get_displayid().length())
                                            fn = _profile->get_displayid();
                                        else if (_profile->get_friendly().length())
                                            fn = _profile->get_friendly();
                                    }
                                    col.set_value_as_string("fb.user_name", fn.toStdString());
                                }
                                else
                                {
                                    col.set_value_as_string("fb.user_name", "");
                                }
                                col.set_value_as_string("fb.message", suggestioner->getPlainText().toStdString());
                                col.set_value_as_string("fb.communication_email", emailer->text().toStdString());
                                col.set_value_as_string("Lang", QLocale::system().name().toStdString());
                                col.set_value_as_string("attachements_count", QString::number((filesToSend.size() + 1)).toStdString()); // +1 'coz we're sending log txt
                                if (!filesToSend.empty())
                                {
                                    core::ifptr<core::iarray> farray(col->create_array());
                                    farray->reserve((int)filesToSend.size());
                                    for (const auto &f: filesToSend)
                                    {
                                        core::ifptr<core::ivalue> val(col->create_value());
                                        val->set_as_string(f.second.toUtf8().data(), (int)f.second.toUtf8().length());
                                        farray->push_back(val.get());
                                    }
                                    col.set_value_as_array("fb.attachement", farray.get());
                                }
                                GetDispatcher()->post_message_to_core("feedback/send", col.get());
                            }
                            else
                            {
                                emit GetDispatcher()->feedbackSent(false);
                            }
                        });
                    }
                    if (sb != suggestioner->property("normal").toBool())
                        Utils::ApplyStyle(suggestioner, suggestioner->styleSheet());
                    if (eb != emailer->property("normal").toBool())
                        Utils::ApplyStyle(emailer, emailer->styleSheet());
                }));

                GetDisconnector()->add("feedback/sent", scroll_area->connect(GetDispatcher(), &core_dispatcher::feedbackSent, [=](bool succeeded)
                {
                    sendButton->setVisible(true);
                    sendSpinner->setVisible(false);
                    sendingSign->setVisible(false);

                    suggestioner->setEnabled(true);
                    filer->setEnabled(true);
                    emailer->setEnabled(true);

                    suggestionerCover->setVisible(false);
                    filerCover->setVisible(false);
                    emailerCover->setVisible(false);

                    if (!succeeded)
                    {
                        errorOccuredSign->setVisible(true);
                    }
                    else
                    {
                        for (auto p: filesToSend)
                        {
                            auto f = filer->findChild<QWidget *>(p.first);
                            if (f)
                            {
                                f->setVisible(false);
                                delete f;
                            }
                        }
                        filesToSend.clear();
                        filesSizeLimiter = 0;

                        suggestioner->setText("");

                        sendingPage->setVisible(false);
                        successPage->setVisible(true);
                    }
                }));

            }
            sendingPageLayout->addWidget(sp);
        }
        scroll_area_content_layout->addWidget(sendingPage);

        auto successPageLayout = new QVBoxLayout(successPage);
        successPage->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);
        successPageLayout->setContentsMargins(0, 0, 0, 0);
        successPageLayout->setSpacing(0);
        successPageLayout->setAlignment(Qt::AlignCenter);
        {
            auto mp = new QWidget(successPage);
            Utils::grabTouchWidget(mp);
            auto mpl = new QVBoxLayout(mp);
            mp->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
            mpl->setAlignment(Qt::AlignCenter);
            mpl->setContentsMargins(0, 0, 0, 0);
            {
                auto m = new QPushButton(mp);
                m->setFlat(true);
                m->setStyleSheet("border-image: url(:/resources/placeholders/content_illustration_good_200.png);");
                m->setFixedSize(Utils::scale_value(120), Utils::scale_value(136));
                mpl->addWidget(m);
            }
            successPageLayout->addWidget(mp);

            auto t1p = new QWidget(successPage);
            auto t1pl = new QVBoxLayout(t1p);
            Utils::grabTouchWidget(t1p);
            t1p->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
            t1pl->setAlignment(Qt::AlignCenter);
            t1pl->setContentsMargins(0, 0, 0, 0);
            {
                auto t1 = new TextEmojiWidget(t1p, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(16), QColor("#282828"), Utils::scale_value(32));
                t1->setText(QT_TRANSLATE_NOOP("contactus_page", "Thank you for contacting us!"));
                t1->setSizePolicy(QSizePolicy::Policy::Preferred, t1->sizePolicy().verticalPolicy());
                t1pl->addWidget(t1);
            }
            successPageLayout->addWidget(t1p);

            auto t2p = new QWidget(successPage);
            auto t2pl = new QVBoxLayout(t2p);
            Utils::grabTouchWidget(t2p);
            t2p->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
            t2pl->setAlignment(Qt::AlignCenter);
            t2pl->setContentsMargins(0, 0, 0, 0);
            {
                auto t2 = new TextEmojiWidget(t2p, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(16), QColor("#579e1c"), Utils::scale_value(32));
                t2->setText(QT_TRANSLATE_NOOP("contactus_page", "Send another review"));
                t2->setSizePolicy(QSizePolicy::Policy::Preferred, t2->sizePolicy().verticalPolicy());
                t2->setCursor(Qt::PointingHandCursor);
                GetDisconnector()->add("feedback/repeat", scroll_area->connect(t2, &TextEmojiWidget::clicked, [=]()
                {
                    successPage->setVisible(false);
                    sendingPage->setVisible(true);
                }));
                t2pl->addWidget(t2);
            }
            successPageLayout->addWidget(t2p);
        }
        successPage->setVisible(false);
        scroll_area_content_layout->addWidget(successPage);
    }
}