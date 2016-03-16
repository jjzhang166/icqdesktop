#pragma once

#include "../../controls/TextEditEx.h"

class QPushButton;

namespace Ui
{
    class input_edit : public TextEditEx
    {
        virtual bool catch_enter(int _modifiers) override;

    public:

        input_edit(QWidget* _parent);
    };

    class InputWidget : public QWidget
    {
        Q_OBJECT

Q_SIGNALS:

        void smilesMenuSignal();
        void sendMessage(QString);
        void editFocusOut();

    public Q_SLOTS:
        void quote(QString);

    private Q_SLOTS:

        void edit_content_changed();
        void send();
        void attach_file();
        void stats_attach_file();

        void contactSelected(QString);
        void insert_emoji(int32_t _main, int32_t _ext);
        void send_sticker(int32_t _set_id, int32_t _sticker_id);
        void resize_to(int _height);
        
        void typed(int);

        void stats_message_enter();
        void stats_message_send();

    public:

        InputWidget(QWidget* parent);
        ~InputWidget();
        void hide();

        Q_PROPERTY(int current_height READ get_current_height WRITE set_current_height)

        void set_current_height(int _val);
        int get_current_height() const;

    private:

        QPushButton* smiles_button_;	
        QPushButton* send_button_;
        QPushButton* file_button_;
        QPropertyAnimation* anim_height_;

        input_edit* text_edit_;
        QString contact_;

        int active_height_;
        int need_height_;
        int active_document_height_;

    protected:

        virtual void paintEvent(QPaintEvent* _e) override;
        virtual void resizeEvent(QResizeEvent * _e) override;
        virtual void keyReleaseEvent(QKeyEvent * _e) override;
    };
}