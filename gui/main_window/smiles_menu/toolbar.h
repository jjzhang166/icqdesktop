#pragma once
#include "../../controls/CustomButton.h"
namespace Ui
{
	class smiles_Widget;
	class CustomButton;

	namespace Smiles
	{
		class AttachedView
		{
			QWidget*	view_;
			QWidget*	view_parent_;

		public:

			AttachedView(QWidget* _view, QWidget* _view_parent = nullptr);

			QWidget* get_view();
			QWidget* get_view_parent();
		};

		class TabButton : public CustomButton
		{
			Q_OBJECT

			void Init();

			const QString	resource_;

			AttachedView	attached_view_;

		public:

			TabButton(QWidget* _parent);
			TabButton(QWidget* _parent, const QString& _resource);
			
			void AttachView(const AttachedView& _view);
			const AttachedView& GetAttachedView() const;

			~TabButton();
		};

		enum buttons_align
		{
			center	= 1,
			left	= 2,
			right	= 3
		};

		
		class Toolbar : public QFrame
		{
			Q_OBJECT
					
		private:

			enum direction
			{
				left	= 0,
				right	= 1
			};

			buttons_align			align_;
			QHBoxLayout*			hor_layout_;
			QList<TabButton*>		buttons_;
			QScrollArea*			view_area_;

			CustomButton*			button_left_;
			QWidget*				button_left_cap_;
			CustomButton*			button_right_;
			QWidget*				button_right_cap_;
			QPropertyAnimation*		anim_scroll_;
			

			void addButton(TabButton* _button);
			void initScroll();
			void scrollStep(direction _direction);
			void showButtons(int _min, int _max, int _cur);

		private Q_SLOTS:
			void touchScrollStateChanged(QScroller::State);

		protected:

			virtual void paintEvent(QPaintEvent* _e) override;
			virtual void resizeEvent(QResizeEvent * _e) override;
			virtual void wheelEvent(QWheelEvent* _e) override;
		public:

			Toolbar(QWidget* _parent, buttons_align _align);
			~Toolbar();

			TabButton* addButton(const QString& _resource);
			TabButton* addButton(const QPixmap& _icon);

			void scrollToButton(TabButton* _button);

			const QList<TabButton*>& GetButtons() const;
            void updateArrowButtonsVisibility();
		};
	}
}