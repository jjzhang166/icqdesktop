#pragma once

namespace Ui
{
    class FFMpegPlayer;
    class InputWidget;
    class TextEditEx;
    class CustomButton;
    class ContactAvatarWidget;

    namespace Smiles
    {
        class EmojiViewItemModel;
        class EmojiTableItemDelegate;
    }
    
    class PreviewWidget : public QWidget
    {
        Q_OBJECT
        
    public:
        PreviewWidget(QWidget* parent);
        ~PreviewWidget();
        
        void setPreview(const QPixmap& _preview);

        void setAimId(const QString& _aimid);
        QString getAimId() const;

        void updateFriendly();
        
    protected:
        virtual void paintEvent(QPaintEvent* e);
        virtual void resizeEvent(QResizeEvent* e);
        
    private:
        QPixmap Preview_;
        QString AimId_;
        QString FriendlyName_;
        ContactAvatarWidget* Avatar_;
        TextEditEx* Friendly_;
    };

    class ProgressBar : public QWidget
    {
        Q_OBJECT

    public:
        ProgressBar(QWidget* parent);
        ~ProgressBar();

        void setCount(int count);
        void setCurrent(int cur);
        void resetCurrent();
        void next();
        void prev();

        void setFriednly(const QString& _friednly);
        void setAimId(const QString& _aimId);
        void setViews(int _views);
        void setTimestamp(int32_t _timestamp);

    public Q_SLOTS:
        void durationChanged(qint64 _duration);
        void positionChanged(qint64 _position);

    protected:
        virtual void paintEvent(QPaintEvent *);
        virtual void resizeEvent(QResizeEvent *);
        virtual void mouseReleaseEvent(QMouseEvent *);
        virtual void mouseMoveEvent(QMouseEvent *);
        virtual void leaveEvent(QEvent *);

    private:
        void updateFriednly();

    private:
        int Pos_;
        int Duration_;
        int Current_;
        int Count_;

        bool Inverted_;

        TextEditEx* Friendly_;
        ContactAvatarWidget* Avatar_;

        QLabel* Views_;
        QLabel* Timestamp_;
        QString AimId_;
        QString FriendlyName_;
    };

    class ControlPanel : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void messageClicked();
        void openSmilesClicked();
        void forwardClicked();
        void animation(QImage, int, int, int, int);

    public:
        ControlPanel(QWidget* parent);
        ~ControlPanel();

        void predefined(const QString& aimId, const QString& text);
        void setLeftPadding(int pad) { LeftPadding_ = pad; }

        Q_PROPERTY(int prop READ getProp WRITE setProp);

        int getProp() const { return Prop_; }
        void setProp(int val) { Prop_ = val; update(); }

    protected:
        virtual void paintEvent(QPaintEvent *);
        virtual void mouseMoveEvent(QMouseEvent *);
        virtual void mouseReleaseEvent(QMouseEvent *);
        virtual void leaveEvent(QEvent *);

    private:
        bool isMessage(const QPoint& p);
        bool isForward(const QPoint& p);
        bool isFirstSmile(const QPoint& p);
        bool isSecondSmile(const QPoint& p);
        bool isThirdSmile(const QPoint& p);
        bool isSmilesOpen(const QPoint& p);
        void addEmoji(int32_t, int32_t);

    private Q_SLOTS:
        void send();

    private:
        bool MessageHovered_;
        bool ForwardHovered_;
        bool SmilesOpenHovered_;

        int LeftPadding_;
        int Prop_;

        TextEditEx* Text_;
        QTimer* Timer_;
        QString AimId_;
        QString Url_;

        QPropertyAnimation* Animation_;
        int curSmile_;
    };

    class MyControlPanel : public QWidget
    {
        Q_OBJECT

Q_SIGNALS:
        void save();
        void deleteSnap();
        void forwardClicked();

    public:
        MyControlPanel(QWidget* parent);
        ~MyControlPanel();

    protected:
        virtual void paintEvent(QPaintEvent *);
        virtual void mouseMoveEvent(QMouseEvent *);
        virtual void mouseReleaseEvent(QMouseEvent *);

    private:
        bool isSave(const QPoint& p);
        bool isDelete(const QPoint& p);
        bool isForward(const QPoint& p);

    private:
        bool SaveHovered_;
        bool DeleteHovered_;
        bool ForwardHovered_;
    };

    class MessagePanel : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void needMove();
        void closeClicked();

    public:
        MessagePanel(QWidget* parent);
        ~MessagePanel();

        void predefined(const QString& aimId, const QString& text);
        void setFocus();

    protected:
        virtual void paintEvent(QPaintEvent * e);
        virtual void mouseReleaseEvent(QMouseEvent *e);

    private Q_SLOTS:
        void sizeChanged();

    private:
        Ui::InputWidget* Input_;
        QPushButton* Close_;
    };

    class SmilesPanel : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void closeClicked();
        void animation(QImage, int, int, int, int);

    public:
        SmilesPanel(QWidget* parent);
        ~SmilesPanel();

        void predefined(const QString& aimId, const QString& url);

    protected:
        virtual void paintEvent(QPaintEvent * e);
        virtual void resizeEvent(QResizeEvent * e);
        virtual void mouseReleaseEvent(QMouseEvent * e);

    private Q_SLOTS:
        void send();

    private:
        Smiles::EmojiViewItemModel* Model_;
        Smiles::EmojiTableItemDelegate* Delegate_;
        QTableView* View_;
        QScrollArea* Scroll_;
        QWidget* ViewWidget_;
        QLabel* EmojiLabel_;
        TextEditEx* Text_;
        QTimer* Timer_;
        QString AimId_;
        QString Url_;
    };

    class SmileAnimation : public QPropertyAnimation
    {
        Q_OBJECT

    public:
        SmileAnimation(QWidget* parent, const QString& prop, QImage smile, int xMin, int xMax, int yMin, int yMax);
        void paint(QPainter& painter);

    private:
        float calcOpacity();
        int calcX();

    private:
        QImage Smile_;
        int XMin_;
        int XMax_;
        int YMin_;
        int YMax_;
    };

    class AnimationArea : public QWidget
    {
        Q_OBJECT

    public:
        AnimationArea(QWidget* parent);
        void addAnimation(QImage smile, int xMin, int xMax, int yMin, int yMax);

        Q_PROPERTY(int prop1 READ getProp1 WRITE setProp1);
        Q_PROPERTY(int prop2 READ getProp2 WRITE setProp2);
        Q_PROPERTY(int prop3 READ getProp3 WRITE setProp3);
        Q_PROPERTY(int prop4 READ getProp4 WRITE setProp4);
        Q_PROPERTY(int prop5 READ getProp5 WRITE setProp5);

        int getProp1() const { return prop1_; }
        void setProp1(int val) { prop1_ = val; update(); }
        int getProp2() const { return prop2_; }
        void setProp2(int val) { prop2_ = val; update(); }
        int getProp3() const { return prop3_; }
        void setProp3(int val) { prop3_ = val; update(); }
        int getProp4() const { return prop4_; }
        void setProp4(int val) { prop4_ = val; update(); }
        int getProp5() const { return prop5_; }
        void setProp5(int val) { prop5_ = val; update(); }

    protected:
        virtual void paintEvent(QPaintEvent *);

    private:
        QString propNameById(int id);

    private:
        int prop1_;
        int prop2_;
        int prop3_;
        int prop4_;
        int prop5_;

        std::vector<SmileAnimation*> Animations_;
    };

    struct SnapId
    {
        SnapId()
            : Id_(-1)
            , New_(false)
            , First_(false)
            , Failed_(false)
        {
        }

        QString AimId_;
        QString OriginalAimdId_;
        QString Url_;
        QString Local_;
        qint64 Id_;
        bool New_;
        bool First_;
        bool Failed_;
    };

    class SnapsPage: public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void currentChanged(QString, bool);
        void close();
     
    public:
        SnapsPage(QWidget *parent);
        ~SnapsPage();

        void stop();
        void notifyApplicationWindowActive(const bool isActive);

        Q_PROPERTY(int prop READ getProp WRITE setProp);

        int getProp() const { return Prop_; }
        void setProp(int val);

    protected:
        virtual void resizeEvent(QResizeEvent *);
        virtual void mouseReleaseEvent(QMouseEvent *);

    private Q_SLOTS:
        void playSnap(QString, QString, QString, QString, qint64, bool);
        void fileLoaded();
        void mediaChanged(qint32);
        void messageClicked();
        void smilesClicked();
        void closeClicked();
        void moveMessage();
        void forwardClicked();
        void addAnimation(QImage, int, int, int, int);
        void save();
        void deleteSnap();
        void positionChanged(qint64);
        void clear();
        void showPreview();
        void showFailPreview(uint32_t);
        void hidePreview();
        void streamsOpenFailed(uint32_t);
        void next();

    private:
        void updatePreviews();
        void addPreview(const QString& _aimId, qint64 id);
        void clearPreviewWidgets();

    private:
        FFMpegPlayer* Player_;
        ProgressBar* Progress_;
        ControlPanel* Control_;
        MyControlPanel* MyControl_;
        MessagePanel* Message_;
        SmilesPanel* Smiles_;
        AnimationArea* Animations_;
        PreviewWidget* Preview_;
        QList<PreviewWidget*> PreviewWidgets_;
        QMap<qint32, SnapId> UserMediaId_;
        QList<SnapId> Queue_;
        QString Current_;
        QString CurrentUrl_;
        QString CurrentLocal_;
        qint64 CurrentId_;
        QString PreviewAimId_;
        QString PreviewUrl_;
        qint64 PreviewId_;
        uint32_t CurrentMedia_;
        QPropertyAnimation* FakeProgress_;
        TextEditEx* Error_;
        QTimer* NextTimer_;
        bool Prev_;
        int Prop_;

        CustomButton* Close_;
        std::shared_ptr<bool> ref_;
    };
}
