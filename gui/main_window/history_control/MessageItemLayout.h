#pragma once

namespace Ui
{

    class MessageItem;

    class MessageItemLayout : public QLayout
    {
    public:
        MessageItemLayout(MessageItem *parent);

        virtual void setGeometry(const QRect &r) override;

        virtual void addItem(QLayoutItem *item) override;

        virtual QLayoutItem *itemAt(int index) const override;

        virtual QLayoutItem *takeAt(int index) override;

        virtual int count() const override;

        virtual QSize sizeHint() const override;

        virtual void invalidate() override;

        void setDirty();

    private:
        QSize LastSize_;

        bool IsDirty_;
    };

}