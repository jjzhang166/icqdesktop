#pragma once

#include "GenericBlock.h"

#include "../../../types/link_metadata.h"

UI_NS_BEGIN

class ActionButtonWidget;
class TextEditEx;

UI_NS_END

UI_COMPLEX_MESSAGE_NS_BEGIN

class EmbeddedPreviewWidgetBase;
class ILinkPreviewBlockLayout;

class LinkPreviewBlock final : public GenericBlock
{
    friend class LinkPreviewBlockLayout;

    Q_OBJECT

public:
    LinkPreviewBlock(ComplexMessageItem *parent, const QString &uri);

    virtual ~LinkPreviewBlock() override;

    virtual void clearSelection() override;

    QPoint getActionButtonLogicalCenter() const;

    QSize getActionButtonSize() const;

    virtual IItemBlockLayout* getBlockLayout() const override;

    const QString& getDescription() const;

    QSize getFaviconSizeUnscaled() const;

    QSize getPreviewImageSize() const;

    virtual QString getSelectedText(bool isFullSelect = false) const override;

    const QString& getSiteName() const;

    const QFont& getSiteNameFont() const;

    int32_t getTitleTextHeight() const;

    bool hasActionButton() const;

    virtual bool hasRightStatusPadding() const override;

    bool hasTitle() const;

    void hideActionButton();

    bool isInPreloadingState() const;

    virtual bool isSelected() const override;

    virtual void onVisibilityChanged(const bool isVisible) override;

    virtual void selectByPos(const QPoint& from, const QPoint& to, const BlockSelectionType selection) override;

    void setTitleGeometry(const QRect &geometry);

    void setTitleWidth(const int32_t width);

    void showActionButton(const QRect &pos);

    virtual void setMaxPreviewWidth(int width) override;

    virtual int getMaxPreviewWidth() const override;

    virtual void setFontSize(int size) override;

    virtual void setTextOpacity(double opacity) override;

protected:
    virtual void drawBlock(QPainter &p) override;

    virtual void initialize() override;

    virtual void mouseMoveEvent(QMouseEvent *event) override;

    virtual void mousePressEvent(QMouseEvent *event) override;

    virtual void mouseReleaseEvent(QMouseEvent *event) override;

    virtual void showEvent(QShowEvent *event) override;

    virtual void onMenuCopyLink() override;

    virtual bool drag() override;

private Q_SLOTS:
    void onLinkMetainfoMetaDownloaded(int64_t seq, bool success, Data::LinkMetadata meta);

    void onLinkMetainfoFaviconDownloaded(int64_t seq, bool success, QPixmap image);

    void onLinkMetainfoImageDownloaded(int64_t seq, bool success, QPixmap image);

    void onSnapMetainfoDownloaded(int64_t seq, bool success, uint64_t snap_id);

private:
    Q_PROPERTY(int PreloadingTicker READ getPreloadingTickerValue WRITE setPreloadingTickerValue);

    bool createDescriptionControl(const QString &description);

    void createTextControls(const QRect &blockGeometry);

    bool createTitleControl(const QString &title);

    int getPreloadingTickerValue() const;

    void requestSnapMetainfo();

    void setPreloadingTickerValue(const int32_t _val);

    Ui::TextEditEx *Title_;

    Ui::TextEditEx *Annotation_;

    QPixmap FavIcon_;

    QString SiteName_;

    QString Uri_;

    int64_t RequestId_;

    std::unique_ptr<ILinkPreviewBlockLayout> Layout_;

    QPixmap PreviewImage_;

    QSize PreviewSize_;

    QPropertyAnimation *PreloadingTickerAnimation_;

    int32_t PreloadingTickerValue_;

    QPainterPath Bubble_;

    int32_t Time_;

    QFont SiteNameFont_;

    int64_t SnapMetainfoRequestId_;

    bool PressedOverSiteLink_;

    bool MetaDownloaded_;

    bool ImageDownloaded_;

    bool FaviconDownloaded_;

    bool IsSelected_;

    ActionButtonWidget *ActionButton_;

    QString ContentType_;

    Data::LinkMetadata Meta_;

    uint64_t SnapId_;

    int TextFontSize_;

    double TextOpacity_;

    int MaxPreviewWidth_;

    void connectSignals(const bool isConnected);

    Ui::TextEditEx* createTextEditControl(const QString &text, const QFont &font);

    void drawFavicon(QPainter &p);

    void drawPreloader(QPainter &p);

    void drawPreview(QPainter &p);

    void drawSiteName(QPainter &p);

    void initializeActionButton();

    bool isOverSiteLink(const QPoint p) const;

    bool isTitleClickable() const;

    void scalePreview(QPixmap &image);

    QSize scalePreviewSize(const QSize &size) const;

    void updateRequestId();

};

UI_COMPLEX_MESSAGE_NS_END