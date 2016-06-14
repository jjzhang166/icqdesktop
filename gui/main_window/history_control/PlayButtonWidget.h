#pragma once

namespace Themes
{
    class IThemePixmap;

    typedef std::shared_ptr<IThemePixmap> IThemePixmapSptr;
}

namespace Ui
{
    class PlayButtonWidget final : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void startClickedSignal();

        void stopClickedSignal();

    public:
        PlayButtonWidget();

        virtual ~PlayButtonWidget() override;

        void setProgress(const double progress);

        virtual QSize sizeHint() const override;

    protected:
        virtual void mouseMoveEvent(QMouseEvent *event) override;

        virtual void mouseReleaseEvent(QMouseEvent *event) override;

        virtual void paintEvent(QPaintEvent *event) override;

    private:
        void initResources();

        Themes::IThemePixmapSptr StartButtonImage_;

        Themes::IThemePixmapSptr StartButtonImageHover_;

        Themes::IThemePixmapSptr StartButtonImageActive_;

        Themes::IThemePixmapSptr StopButtonImage_;

        Themes::IThemePixmapSptr StopButtonImageHover_;

        Themes::IThemePixmapSptr StopButtonImageActive_;

        double Progress_;

        bool ResourcesInitialized_;

    };
}