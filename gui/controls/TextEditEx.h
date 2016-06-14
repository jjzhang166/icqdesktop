#pragma once
#include "../utils/utils.h"

namespace Ui
{
    class TextEditEx : public QTextBrowser
    {
        Q_OBJECT

    Q_SIGNALS:

        void focusIn();
        void focusOut();
        void clicked();
        void emptyTextBackspace();
        void escapePressed();
        void upArrow();
        void downArrow();
        void enter();
        void setSize(int, int);
        void keyPressed(int);

    private Q_SLOTS:
        void edit_content_changed();

    private:

        int index_;

        typedef std::map<QString, QString>	ResourceMap;

        ResourceMap resource_index_;
        QFont font_;
        QColor color_;
        int prev_pos_;
        bool input_;
        bool isFitToText_;

        bool isCatchEnter_;

        QString getPlainText(int _from, int _to = -1) const;
        void init(int fontSize);

    public:

        TextEditEx(QWidget* parent, const Utils::FontsFamily fontFamily, int fontSize, const QPalette& palette, bool input, bool _isFitToText);
        TextEditEx(QWidget* parent, const Utils::FontsFamily fontFamily, int fontSize, const QColor& color, bool input, bool _isFitToText);

        QString getPlainText() const;
        void setPlainText(const QString& _text, bool _convertLinks = true, const QTextCharFormat::VerticalAlignment _aligment = QTextCharFormat::AlignBaseline);

        void mergeResources(const ResourceMap& _resources);
        void insertEmoji(int _main, int _ext);
        void insertPlainText_(const QString& _text);

        void selectByPos(const QPoint& p);
        void clearSelection();
        bool isAllSelected();
        QString selection();

        QSize getTextSize() const;
        void setCatchEnter(bool _isCatchEnter);
        int adjustHeight(int _width);

    protected:
        virtual void focusInEvent(QFocusEvent*) override;
        virtual void focusOutEvent(QFocusEvent*) override;
        virtual void mousePressEvent(QMouseEvent*) override;
        virtual void mouseReleaseEvent(QMouseEvent*) override;
        virtual void mouseMoveEvent(QMouseEvent *) override;
        virtual void keyPressEvent(QKeyEvent*) override;
        virtual QSize sizeHint() const override;

        virtual QMimeData* createMimeDataFromSelection() const override;
        virtual bool canInsertFromMimeData(const QMimeData* _source) const override;
        virtual void insertFromMimeData(const QMimeData* _source) override;
        virtual bool catchEnter(int _modifiers);
    };
}