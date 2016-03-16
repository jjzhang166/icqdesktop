#pragma once

namespace Ui
{
	class HistoryControl;
    class HistoryControlPage;
	class InputWidget;
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

	Q_SIGNALS:
		void contactSelected(QString _aimId);
		void sendMessage(QString);

	private:
		HistoryControl*				historyControlWidget_;
		InputWidget*				inputWidget_;
		Smiles::SmilesMenu*			smilesMenu_;
        DragOverlayWindow*          dragOverlayWindow_;

		void initSmilesMenu();
		void initInputWidget();

	public:
		ContactDialog(QWidget* _parent);
		~ContactDialog();
        void cancelSelection();
        void hideInput();

        void showDragOverlay();
        void hideDragOverlay();

        const QString & currentAimId();
        
        HistoryControlPage* getHistoryPage(const QString& aimId) const;
        const Smiles::SmilesMenu* getSmilesMenu() const;

    protected:
        virtual void dragEnterEvent(QDragEnterEvent *);
        virtual void dragLeaveEvent(QDragLeaveEvent *);
        virtual void dragMoveEvent(QDragMoveEvent *);
	};
}