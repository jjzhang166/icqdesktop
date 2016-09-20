#pragma once

#include "IItemBlock.h"

UI_NS_BEGIN

class ActionButtonWidget;

UI_NS_END

UI_THEMES_NS_BEGIN

class theme;

UI_THEMES_NS_END

UI_COMPLEX_MESSAGE_NS_BEGIN

class ComplexMessageItem;

class GenericBlock :
    public QWidget,
    public IItemBlock
{
    Q_OBJECT

public:
    GenericBlock(
        ComplexMessageItem *parent,
        const QString &sourceText,
        const MenuFlags menuFlags,
        const bool isResourcesUnloadingEnabled);

    virtual ~GenericBlock() = 0;

    virtual QSize blockSizeForMaxWidth(const int32_t maxWidth) override;

    virtual void deleteLater() final override;

    virtual QString formatRecentsText() const override;

    virtual bool standaloneText() const override { return false; }

    virtual ComplexMessageItem* getParentComplexMessage() const final override;

    qint64 getId() const;

    QString getSenderAimid() const;

    QString getSenderFriendly() const;

    const QString& getChatAimid() const;

    virtual QString getSourceText() const override;

    std::shared_ptr<const themes::theme> getTheme() const;

    int getThemeId() const;

    virtual bool isBubbleRequired() const override;

    void setBubbleRequired(bool required);

    bool isOutgoing() const;

    virtual bool isDraggable() const override;

    virtual bool isSharingEnabled() const override;

    bool isStandalone() const;

    virtual MenuFlags getMenuFlags() const override;

    virtual bool onMenuItemTriggered(const QString &command) final override;

    virtual void onActivityChanged(const bool isActive) override;

    virtual void onVisibilityChanged(const bool isVisible) override;

    virtual QRect setBlockGeometry(const QRect &ltr) override;

    virtual QSize sizeHint() const override;

    virtual bool containsImage() const { return false; }

protected:
    virtual bool drag() override;

    virtual void drawBlock(QPainter &p) = 0;

    virtual void enterEvent(QEvent *) override;

    virtual void initialize() = 0;

    bool isInitialized() const;

    void notifyBlockContentsChanged();

    virtual void onMenuCopyLink();

    virtual void onMenuCopyFile();

    virtual void onMenuSaveFileAs();

    virtual void onRestoreResources();

    virtual void onUnloadResources();

    virtual void paintEvent(QPaintEvent *e) final override;

    virtual void mouseMoveEvent(QMouseEvent *e) override;

private:
    GenericBlock();

    void startResourcesUnloadTimer();

    void stopResourcesUnloadTimer();

    bool Initialized_;

    bool IsResourcesUnloadingEnabled_;

    MenuFlags MenuFlags_;

    ComplexMessageItem *Parent_;

    QTimer *ResourcesUnloadingTimer_;

    QString SourceText_;

    bool IsBubbleRequired_;

    QPoint mousePos_;

private Q_SLOTS:
    void onResourceUnloadingTimeout();

};

UI_COMPLEX_MESSAGE_NS_END