#pragma once

#ifndef __ICQ__CustomAbstractListModel__
#define __ICQ__CustomAbstractListModel__

namespace Logic
{
    enum CustomAbstractListModelFlags
    {
        HasMouseOver    = 0x00000001,
    };
    
    class CustomAbstractListModel: public QAbstractListModel
    {
        Q_OBJECT

    private Q_SLOTS:
        void forceRefreshList(QAbstractItemModel *, bool);

    private:
        int flags_;

    private:
        void setFlag(int flag);
        void unsetFlag(int flag);

    protected:
        virtual void refreshList();
        
    public:
        CustomAbstractListModel(QObject *parent = 0);
        virtual ~CustomAbstractListModel();
        
        inline bool customFlagIsSet(int flag) const
        {
            return ((flags_ & flag) == flag);
        }
    };
}

#endif /* defined(__ICQ__CustomAbstractListModel__) */
