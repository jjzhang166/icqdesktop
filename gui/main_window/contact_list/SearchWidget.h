#pragma once

namespace Ui
{
    class LineEditEx;
    class CustomButton;

    class SearchWidget : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void search(QString);
        void searchBegin();
        void searchEnd();
        void inputEmpty();
        void enterPressed();
        void upPressed();
        void downPressed();
        void escapePressed();
        void nonActiveButtonPressed();
        void searchIconClicked();
        void activeChanged(bool _isActive);

    public Q_SLOTS:
        void searchCompleted();

    private Q_SLOTS:
        void searchStarted();
        void searchChanged(QString);
        void clearPressed();
        void editEnterPressed();
        void editUpPressed();
        void editDownPressed();
        void focusedOut();
        void onEscapePress();

    public:
        SearchWidget(bool _isWithButton, QWidget* _parent = 0, int _offset = 0);
        ~SearchWidget();

        QString getText() const;
        bool isEmpty() const;
        void clearInput();
        void setShowButton(bool _isShow);
        void setTransparent(bool _isTransparent);
        void setFocus();
        void clearFocus();

        inline CustomButton *searchIcon() { return searchIcon_; }
        inline CustomButton *searchEditIcon() { return searchEditIcon_; }
        void setShortView(bool _isShort);
        void setSearchEditIconVisible(bool _isShow);

    private:
        void setActive(bool _active);
        LineEditEx* searchEdit_;
        bool active_;
        bool isShowButton_;
        bool isTransparent_;

        void paintEvent(QPaintEvent *_e) override;
        void retranslateUi(QWidget* _searchWidget);
        QVBoxLayout *vMainLayout_;
        QVBoxLayout *vSearchLayout_;
        QHBoxLayout *hMainLayout;
        QWidget *horLineWidget_;
        QWidget *parentWidget_;
        QHBoxLayout *horLineLayout_;
        CustomButton *searchIcon_;
        QWidget *horLine_;
        CustomButton *searchEditIcon_;
        QMenu* menu_;
        bool isShortView_;
        bool isWithButton_;
    };
}