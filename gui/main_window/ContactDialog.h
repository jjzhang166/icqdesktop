#pragma once

namespace Ui
{
	class HistoryControl;
    class HistoryControlPage;
	class InputWidget;
    class Sidebar;

	namespace Smiles
	{
		class SmilesMenu;
	}
    class ContactDialog;

    class DragOverlayWindow : public QWidget
    {
        Q_OBJECT

    public:
        DragOverlayWindow(ContactDialog* _parent);

    protected:
        virtual void paintEvent(QPaintEvent *e);
        virtual void dragEnterEvent(QDragEnterEvent *e);
        virtual void dragLeaveEvent(QDragLeaveEvent *e);
        virtual void dragMoveEvent(QDragMoveEvent *e);
        virtual void dropEvent(QDropEvent *e);

    private:
        ContactDialog* Parent_;
    };

	class ContactDialog : public QWidget
	{
		Q_OBJECT

	public Q_SLOTS:

		void onContactSelected(QString _aimId);
		void onSmilesMenu();
		void onInputEditFocusOut();

    private Q_SLOTS:
        void updateDragOverlay();
        void historyControlClicked();

	Q_SIGNALS:
		void contactSelected(QString _aimId);
		void sendMessage(QString);

	private:
		HistoryControl*				historyControlWidget_;
		InputWidget*				inputWidget_;
		Smiles::SmilesMenu*			smilesMenu_;
        DragOverlayWindow*          dragOverlayWindow_;
        Sidebar*                    sidebar_;
        QTimer*                     overlayUpdateTimer_;
        QStackedWidget*             topWidget_;
        QVBoxLayout*                rootLayout_;
        QMap<QString, QWidget*>     topWidgetsCache_;
        bool                        sidebarVisible_;
        QHBoxLayout*                layout_;

		void initSmilesMenu();
		void initInputWidget();

	public:
		ContactDialog(QWidget* _parent);
		~ContactDialog();
        void cancelSelection();
        void hideInput();

        void showDragOverlay();
        void hideDragOverlay();

        void showSidebar(const QString& aimId, int page);
        bool isSidebarVisible() const;
        void setSidebarVisible(bool show);
        bool needShowSidebar() const;

        static bool needShowSidebar(int _contact_dialog_width);
        static bool sideBarShowSingle(int _contact_dialog_width);
        static std::string getSideBarPolicy(int _contact_dialog_width);

        void takeSidebar();
        Sidebar* getSidebar() const;

        void insertTopWidget(const QString& aimId, QWidget* widget);
        void removeTopWidget(const QString& aimId);
        
        HistoryControlPage* getHistoryPage(const QString& aimId) const;
        const Smiles::SmilesMenu* getSmilesMenu() const;

    protected:
        virtual void dragEnterEvent(QDragEnterEvent *);
        virtual void dragLeaveEvent(QDragLeaveEvent *);
        virtual void dragMoveEvent(QDragMoveEvent *);
        virtual void resizeEvent(QResizeEvent*);
	};
}