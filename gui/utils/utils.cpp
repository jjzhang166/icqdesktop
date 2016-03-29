#include "stdafx.h"

#include "../core_dispatcher.h"
#include "../gui_settings.h"
#include "../constants.h"
#include "SChar.h"
#include "../cache/emoji/Emoji.h"
#include "../cache/countries.h"
#include "profiling/auto_stop_watch.h"
#include "InterConnector.h"
#include "../main_window/MainWindow.h"

#include "utils.h"

#ifdef _WIN32
    #include <windows.h>
#else

#ifdef __APPLE__
    #include "mac_support.h"
#endif

#endif

namespace
{
    const int mobile_rect_width = 8;
    const int mobile_rect_height = 12;
	const QColor ColorTable[] = {
		"#FF0000",
		"#FF7373",
		"#CA4242",
		"#F47B5A",
		"#990000",
		"#FF6665",
		"#E69200",
		"#FF9010",
		"#E1CB00",
		"#1CD920",
		"#5DB82A",
		"#088DA5",
		"#008000",
		"#0092D0",
		"#389BFF",
		"#0099CC",
		"#3E687A",
		"#0166CC",
		"#0000FF",
		"#46C5FC",
		"#4356BC",
		"#73799B",
		"#FC62B4",
		"#800180",
		"#B682D8",
		"#B44FF2",
		"#D852AE",
		"#E8A2C4"
	};

#ifdef _WIN32
	const auto ColorTableSize = _countof(ColorTable);
#else
    const auto ColorTableSize = sizeof(ColorTable)/sizeof(ColorTable[0]);
#endif

	const quint32 CRC32Table[ 256 ] =
	{
		0x00000000, 0x77073096, 0xee0e612c, 0x990951ba,
		0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
		0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
		0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
		0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de,
		0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
		0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,
		0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
		0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
		0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
		0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940,
		0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
		0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116,
		0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
		0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
		0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
		0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a,
		0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
		0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818,
		0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
		0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
		0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
		0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c,
		0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
		0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2,
		0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
		0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
		0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
		0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086,
		0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
		0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4,
		0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
		0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
		0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
		0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8,
		0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
		0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe,
		0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
		0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
		0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
		0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252,
		0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
		0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60,
		0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
		0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
		0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
		0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04,
		0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
		0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a,
		0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
		0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
		0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
		0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e,
		0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
		0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c,
		0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
		0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
		0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
		0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0,
		0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
		0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6,
		0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
		0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
		0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
	};

	quint32 crc32FromIODevice( QIODevice * device )
	{
		quint32 crc32 = 0xffffffff;
		char * buf = new char[ 256 ];
		qint64 n;

		while( ( n = device->read( buf, 256 ) ) > 0 )
			for ( qint64 i = 0; i < n; i++ )
				crc32 = ( crc32 >> 8 ) ^ CRC32Table[ ( crc32 ^ buf[i] ) & 0xff ];
		delete[] buf;

		crc32 ^= 0xffffffff;
		return crc32;
	}

	quint32 crc32FromByteArray( const QByteArray & array )
	{
		QBuffer buffer;
		buffer.setData( array );
		if ( !buffer.open( QIODevice::ReadOnly ) )
			return 0;

		return crc32FromIODevice( &buffer );
	}

	quint32 crc32FromString( const QString & text )
	{
		return crc32FromByteArray( text.toLatin1() );
	}

#ifdef _WIN32
	QChar VKeyToChar( short vkey, HKL layout)
	{
		DWORD dwScan=MapVirtualKeyEx(vkey&0x00ff,0,layout);
		byte KeyStates[256]={0};
		KeyStates[vkey&0x00ff]=0x80;
		DWORD dwFlag=0;
		if(vkey&0x0100)
		{
			KeyStates[VK_SHIFT]=0x80;
		}
		if(vkey&0x0200)
		{
			KeyStates[VK_CONTROL]=0x80;
		}
		if(vkey&0x0400)
		{
			KeyStates[VK_MENU]=0x80;
			dwFlag=1;
		}
		wchar_t Result=L' ';
		if(!ToUnicodeEx(vkey&0x00ff,dwScan,KeyStates,&Result,1,dwFlag,layout)==1)
		{
			return QChar(' ');
		}
		return QChar(Result);
	}
#endif //_WIN32
}

namespace Utils
{
    void drawText(QPainter & painter, const QPointF & point, int flags,
        const QString & text, QRectF * boundingRect)
    {
        const qreal size = 32767.0;
        QPointF corner(point.x(), point.y() - size);
        if (flags & Qt::AlignHCenter) corner.rx() -= size/2.0;
        else if (flags & Qt::AlignRight) corner.rx() -= size;
        if (flags & Qt::AlignVCenter) corner.ry() += size/2.0;
        else if (flags & Qt::AlignTop) corner.ry() += size;
        else flags |= Qt::AlignBottom;
        QRectF rect(corner, QSizeF(size, size));
        painter.drawText(rect, flags, text, boundingRect);
    }

	ShadowWidgetEventFilter::ShadowWidgetEventFilter(int shadowWidth)
		: QObject(0)
		, ShadowWidth_(shadowWidth)
	{

	}

	bool ShadowWidgetEventFilter::eventFilter(QObject* obj, QEvent* event)
	{
		if (event->type() == QEvent::Paint)
		{
			QWidget* w = qobject_cast<QWidget*>(obj);

			QRect origin = w->rect();

			QRect right = QRect(QPoint(origin.width() - ShadowWidth_, origin.y() + ShadowWidth_ + 1), QPoint(origin.width(), origin.height() - ShadowWidth_ - 1));
			QRect left = QRect(QPoint(origin.x(), origin.y() + ShadowWidth_), QPoint(origin.x() + ShadowWidth_ - 1, origin.height() - ShadowWidth_ - 1));
			QRect top = QRect(QPoint(origin.x() + ShadowWidth_ + 1, origin.y()), QPoint(origin.width() - ShadowWidth_ - 1, origin.y() + ShadowWidth_));
			QRect bottom = QRect(QPoint(origin.x() + ShadowWidth_ + 1, origin.height() - ShadowWidth_), QPoint(origin.width() - ShadowWidth_ - 1, origin.height()));

			QRect topLeft = QRect(origin.topLeft(), QPoint(origin.x() + ShadowWidth_, origin.y() + ShadowWidth_));
			QRect topRight = QRect(QPoint(origin.width() - ShadowWidth_, origin.y()), QPoint(origin.width(), origin.y() + ShadowWidth_));
			QRect bottomLeft = QRect(QPoint(origin.x(), origin.height() - ShadowWidth_), QPoint(origin.x() + ShadowWidth_, origin.height()));
			QRect bottomRight = QRect(QPoint(origin.width() - ShadowWidth_, origin.height() - ShadowWidth_), origin.bottomRight());

			QPainter p(w);

			bool isActive = w->isActiveWindow();

			QLinearGradient lg = QLinearGradient(right.topLeft(), right.topRight());
			setGradientColor(lg, isActive);
			p.fillRect(right, QBrush(lg));

			lg = QLinearGradient(left.topRight(), left.topLeft());
			setGradientColor(lg, isActive);
			p.fillRect(left, QBrush(lg));

			lg = QLinearGradient(top.bottomLeft(), top.topLeft());
			setGradientColor(lg, isActive);
			p.fillRect(top, QBrush(lg));

			lg = QLinearGradient(bottom.topLeft(), bottom.bottomLeft());
			setGradientColor(lg, isActive);
			p.fillRect(bottom, QBrush(lg));

			QRadialGradient g = QRadialGradient(topLeft.bottomRight(), ShadowWidth_);
			setGradientColor(g, isActive);
			p.fillRect(topLeft, QBrush(g));

			g = QRadialGradient(topRight.bottomLeft(), ShadowWidth_);
			setGradientColor(g, isActive);
			p.fillRect(topRight, QBrush(g));

			g = QRadialGradient(bottomLeft.topRight(), ShadowWidth_);
			setGradientColor(g, isActive);
			p.fillRect(bottomLeft, QBrush(g));

			g = QRadialGradient(bottomRight.topLeft(), ShadowWidth_);
			setGradientColor(g, isActive);
			p.fillRect(bottomRight, QBrush(g));
		}

		return QObject::eventFilter(obj, event);
	}

	void ShadowWidgetEventFilter::setGradientColor(QGradient& gradient, bool isActive)
	{
		gradient.setColorAt(0, QColor(0, 0, 0, 50));
		gradient.setColorAt(0.2, QColor(0, 0, 0, isActive ? 20 : 10));
		gradient.setColorAt(0.6, isActive ? QColor(0, 0, 0, 5) : Qt::transparent);
		gradient.setColorAt(1, Qt::transparent);
	}


    SignalsDisconnector::SignalsDisconnector()
    {
    }

    SignalsDisconnector::~SignalsDisconnector()
    {
        clean();
    }

    void SignalsDisconnector::add(const char *key, QMetaObject::Connection &&connection)
    {
        if (connections_.find(key) != connections_.end())
            connections_.erase(key);
        connections_.emplace(std::make_pair(key, connection));
    }

    void SignalsDisconnector::remove(const char *key)
    {
        if (connections_.find(key) != connections_.end())
        {
            QObject::disconnect(connections_[key]);
            connections_.erase(key);
        }
    }

    void SignalsDisconnector::clean()
    {
        for (auto c: connections_)
            QObject::disconnect(c.second);
        connections_.clear();
    }



	QMap<QString, QString> GetCountryCodes()
	{
		auto countries = Ui::countries::get();

		QMap<QString, QString> result;

		for (const auto& country : countries)
			result.insert(country.name_, QString("+") + QVariant(country.phone_code_).toString());

		return result;
	}

	QString ScaleStyle(const QString& _style, double scale)
	{
		QString out_string;
		QTextStream result(&out_string);

		auto tokens =  _style.split(QRegExp("\\;"));

		for (auto iter_line = tokens.begin(); iter_line != tokens.end(); iter_line++)
		{
			if (iter_line != tokens.begin())
				result << ";";

			int pos = iter_line->indexOf(QRegExp("[\\-\\d]\\d*dip"));

			if (pos != -1)
			{
				result << iter_line->left(pos);
				QString tmp = iter_line->mid(pos, iter_line->right(pos).length());
				int size = QVariant(tmp.left(tmp.indexOf("dip"))).toInt();
                size *= scale;
				result << QVariant(size).toString();
				result << "px";
			}
			else
			{
				pos = iter_line->indexOf("_100");
				if (pos != -1)
				{
					result << iter_line->left(pos);
					result << "_";

                    if (Utils::is_mac_retina())
                    {
                        result << QVariant(2 * 100).toString();
                    }
                    else
                    {
                        result << QVariant(scale * 100).toString();
                    }
					result << iter_line->mid(pos + 4, iter_line->length());
				}
				else
				{
					pos = iter_line->indexOf("/100/");
					if (pos != -1)
					{
						result << iter_line->left(pos);
						result << "/";
                        result << QVariant(Utils::scale_bitmap(scale) * 100).toString();
						result << "/";
						result << iter_line->mid(pos + 5, iter_line->length());
					}
					else
					{
						result << *iter_line;
					}
				}
			}
		}

		return out_string;
	}

    void ApplyPropertyParameter(QWidget *widget, const char *property, QVariant parameter)
    {
        if (widget)
        {
            widget->setProperty(property, parameter);
            widget->style()->unpolish(widget);
            widget->style()->polish(widget);
            widget->update();
        }
    }

    void ApplyStyle(QWidget *widget, QString style)
    {
        if (widget)
        {
            Utils::SetFont(&style);
            widget->setStyleSheet(Utils::ScaleStyle(style, Utils::get_scale_coefficient()));
        }
    }

	QString LoadStyle(const QString& qss_file, double scale, bool import_common_style)
	{
		QFile file(qss_file);

		if (!file.open(QIODevice::ReadOnly))
		{
			assert(!"open style file error");
			return "";
		}

		QString qss = file.readAll();
		if (qss.isEmpty())
		{
			return "";
		}

		QString out_string;
		QTextStream result(&out_string);

		if (import_common_style)
			result << LoadStyle(":/resources/qss/common.qss", scale, false);

        SetFont(&qss);
		result << ScaleStyle(qss, scale);

		return out_string;
	}

    void SetFont(QString* qss)
    {
        qss->replace("%FONT_FAMILY%", Utils::appFontFamily(Utils::FontsFamily::SEGOE_UI));
		qss->replace("%FONT_FAMILY_BOLD%", Utils::appFontFamily(Utils::FontsFamily::SEGOE_UI_BOLD));
		qss->replace("%FONT_FAMILY_SEMIBOLD%", Utils::appFontFamily(Utils::FontsFamily::SEGOE_UI_SEMIBOLD));
		qss->replace("%FONT_FAMILY_LIGHT%", Utils::appFontFamily(Utils::FontsFamily::SEGOE_UI_LIGHT));

        qss->replace("%FONT_WEIGHT%", Utils::appFontWeightQss(Utils::FontsFamily::SEGOE_UI));
        qss->replace("%FONT_WEIGHT_BOLD%", Utils::appFontWeightQss(Utils::FontsFamily::SEGOE_UI_BOLD));
        qss->replace("%FONT_WEIGHT_SEMIBOLD%", Utils::appFontWeightQss(Utils::FontsFamily::SEGOE_UI_SEMIBOLD));
        qss->replace("%FONT_WEIGHT_LIGHT%", Utils::appFontWeightQss(Utils::FontsFamily::SEGOE_UI_LIGHT));
    }

	QPixmap GetDefaultAvatar(const QString &uin, const QString &displayName, const int sizePx, const bool isFilled)
	{
		const auto antialiasSizeMult = 8;
		const auto bigSizePx = (sizePx * antialiasSizeMult);
		const auto bigSize = QSize(bigSizePx, bigSizePx);

		QImage bigResult(bigSize, QImage::Format_ARGB32);

		// evaluate colors

		const auto &str = uin.isEmpty() ? QString("0") : uin;
		const auto crc32 = crc32FromString(str);
		const auto colorIndex = (crc32 % ColorTableSize);
		const auto color = ColorTable[colorIndex];

		QPainter painter(&bigResult);

		painter.setRenderHint(QPainter::Antialiasing);
		painter.setRenderHint(QPainter::TextAntialiasing);
		painter.setRenderHint(QPainter::SmoothPixmapTransform);

		// set pen

		QPen hollowAvatarPen(color);

		const auto hollowPenWidth = scale_value(2 * antialiasSizeMult);
		hollowAvatarPen.setWidth(hollowPenWidth);

		QPen filledAvatarPen(QColor("#ffffff"));

		const QPen &avatarPen = (isFilled ? filledAvatarPen : hollowAvatarPen);
		painter.setPen(avatarPen);

		// draw

		if (isFilled)
		{
			bigResult.fill(color);
		}
		else
		{
			bigResult.fill(Qt::transparent);
			painter.setBrush(Qt::white);

			const auto correction = ((hollowPenWidth / 2) + 1);
			const auto ellipseRadius = (bigSizePx - (correction * 2));
			painter.drawEllipse(correction, correction, ellipseRadius, ellipseRadius);
		}

		auto scaledBigResult = bigResult.scaled(QSize(sizePx, sizePx), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

		const auto trimmedDisplayName = displayName.trimmed();
		const auto firstChar = Utils::PeekNextSuperChar(trimmedDisplayName);
		if (firstChar.IsSimple())
		{
			QPainter letterPainter(&scaledBigResult);
			letterPainter.setRenderHint(QPainter::Antialiasing);

            QFont font = Utils::appFont(Utils::FontsFamily::SEGOE_UI, double(sizePx)/1.5);

			letterPainter.setFont(font);

			letterPainter.setPen(avatarPen);

			const auto toDisplay = trimmedDisplayName[0].toUpper();

			const auto rawFont = QRawFont::fromFont(font);
			quint32 glyphIndex = 0;
			int numGlyphs = 1;
			const auto glyphSearchSucceed = rawFont.glyphIndexesForChars(&toDisplay, 1, &glyphIndex, &numGlyphs);
			if (!glyphSearchSucceed)
			{
				return QPixmap::fromImage(scaledBigResult);
			}

			assert(numGlyphs == 1);

			auto glyphPath = rawFont.pathForGlyph(glyphIndex);
			const auto rawGlyphBounds = glyphPath.boundingRect();
			glyphPath.translate(-rawGlyphBounds.x(), -rawGlyphBounds.y());
			const auto glyphBounds = glyphPath.boundingRect();
			const auto glyphHeight = glyphBounds.height();
			const auto glyphWidth = glyphBounds.width();

			QFontMetrics m(font);
			QRect pseudoRect(0, 0, sizePx, sizePx);
			auto centeredRect = m.boundingRect(pseudoRect, Qt::AlignCenter, QString(toDisplay));

			qreal y = (sizePx / 2.0);
			y -= glyphHeight / 2.0;

			qreal x = centeredRect.x();
			x += (centeredRect.width() / 2.0);
			x -= (glyphWidth / 2.0);

			letterPainter.translate(x, y);
			letterPainter.fillPath(glyphPath, avatarPen.brush());
		}

		if (firstChar.IsEmoji())
		{
			QPainter emojiPainter(&scaledBigResult);

			const auto nearestSizeAvailable = Emoji::GetNearestSizeAvailable(sizePx / 2);

			const auto &emoji =  Emoji::GetEmoji(firstChar.Main(), firstChar.Ext(), nearestSizeAvailable);

			auto x = (sizePx / 2);
			x -= (emoji.width() / 2);

			auto y = (sizePx / 2);
			y -= (emoji.height() / 2);

			emojiPainter.drawImage(x, y, emoji);
		}

		return QPixmap::fromImage(scaledBigResult);
	}

	QStringList GetPossibleStrings(const QString& text)
	{
		QStringList result;

        if (text.isEmpty())
            return result;

#ifdef _WIN32
		HKL aLayouts[8] = {0};
		int nCount = ::GetKeyboardLayoutList(8, aLayouts);
		HKL hCurrent = ::GetKeyboardLayout(0);

		for (int j = 0; j < nCount; ++j)
		{
			result.append(VKeyToChar(::VkKeyScanEx(text.at(0).unicode(), hCurrent), aLayouts[j]));
		}

		for (int i = 1; i < text.length(); i++)
		{
			for (int j = 0; j < nCount; ++j)
			{
				result[j].append(VKeyToChar(::VkKeyScanEx(text.at(i).unicode(), hCurrent), aLayouts[j]));
			}
		}
#else

#ifdef __APPLE__
        MacSupport::getPossibleStrings(text, result);
#else
        result.push_back(text);
#endif

#endif //_WIN32
		return result;
	}

	QPixmap RoundImage(const QPixmap &img, const QString& state)
	{
		int scale = std::min(img.height(), img.width());
		QImage imageOut(QSize(scale, scale), QImage::Format_ARGB32);
		imageOut.fill(Qt::transparent);

		QPainter painter(&imageOut);

		painter.setRenderHint(QPainter::Antialiasing);
		painter.setRenderHint(QPainter::TextAntialiasing);
		painter.setRenderHint(QPainter::SmoothPixmapTransform);

		painter.setPen(Qt::NoPen);
		painter.setBrush(Qt::NoBrush);
		QPainterPath path(QPointF(0,0));
		path.addEllipse(0, 0, scale, scale);

        if (state == "mobile")
        {
            QPainterPath stPath(QPointF(0,0));
            stPath.addRoundRect(QRect(scale - Utils::scale_bitmap(Utils::scale_value(mobile_rect_width)), scale - Utils::scale_bitmap(Utils::scale_value(mobile_rect_height)), Utils::scale_bitmap(Utils::scale_value(mobile_rect_width)), Utils::scale_bitmap(Utils::scale_value(mobile_rect_height))), 50, 36);
            path -= stPath;
        }

		painter.setClipPath(path);
		painter.drawPixmap(0,0,img);

        if (state == "online" || state == "dnd")
        {
            QPainterPath stPath(QPointF(0,0));
            stPath.addRect(0, 0, scale, scale);
            painter.setClipPath(stPath);
            QPixmap p(Utils::parse_image_name(":/resources/cl_status_online_100.png"));
            int x = (scale - p.width());
            int y = (scale - p.height());
            painter.drawPixmap(x, y, p);
        }
        else if (state == "mobile")
        {
            QPainterPath stPath(QPointF(0,0));
            stPath.addRect(0, 0, scale, scale);
            painter.setClipPath(stPath);
            QPixmap p(Utils::parse_image_name(":/resources/cl_status_mobile_100.png"));
            int x = (scale - p.width());
            int y = (scale - p.height());
            painter.drawPixmap(x, y, p);
        }

        QPixmap pixmap = QPixmap::fromImage(imageOut);
        Utils::check_pixel_ratio(pixmap);
		return pixmap;
	}

	QPixmap DrawStatus(const QString& state, int scale)
	{
		QImage imageOut(QSize(scale, scale),QImage::Format_ARGB32);
		imageOut.fill(qRgba(255, 255, 255, 0));

		QPainter painter(&imageOut);

		painter.setRenderHint(QPainter::Antialiasing);
		painter.setRenderHint(QPainter::TextAntialiasing);
		painter.setRenderHint(QPainter::SmoothPixmapTransform);

		painter.setPen(Qt::NoPen);

		QPainterPath path(QPointF(0,0));
		path.addRect(0, 0, scale, scale);
		painter.setClipPath(path);
		if (Utils::state_equals_online(state))
            painter.setBrush(QBrush(qRgba(81, 156, 21, 1)));

		painter.drawEllipse(0, 0, scale, scale);

		return QPixmap::fromImage(imageOut);
	}

	QPixmap DrawUnreads(int size, QColor color, const QString unreads)
	{
		QPixmap pixmap(QSize(size, size));
		QPainter painter(&pixmap);

		painter.setRenderHint(QPainter::Antialiasing);
		painter.setRenderHint(QPainter::TextAntialiasing);
		painter.setRenderHint(QPainter::SmoothPixmapTransform);

        QFont font = Utils::appFont(Utils::FontsFamily::SEGOE_UI_BOLD, scale_value(12));
		painter.setFont( font );
		QPen penHText(QColor("#ffffff"));
		painter.setPen(penHText);
		pixmap.fill(color);

		drawText(painter, QPointF(size/2, size/2 - 1), Qt::AlignHCenter | Qt::AlignVCenter, unreads);

		return RoundImage(pixmap, QString());
	}

	void setPropertyToWidget(QWidget* widget, char* name, bool value)
	{
		widget->setProperty(name, value);
	}

	void applyWidgetPropChanges(QWidget* widget)
	{
		assert(widget);

		QApplication::style()->unpolish(widget);
		QApplication::style()->polish(widget);
		widget->update();
	}

    bool isValidEmailAddress(const QString &email)
    {
        QRegExp r("\\b[A-Z0-9._%+-]+@[A-Z0-9.-]+\\.[A-Z]{2,4}\\b");
        r.setCaseSensitivity(Qt::CaseInsensitive);
        r.setPatternSyntax(QRegExp::RegExp);
        return r.exactMatch(email);
    }

	int scale_value(const int _px)
	{
		return (int)(Utils::get_scale_coefficient() * (double)_px);
	}

    bool foregroundWndIsFullscreened() {
#ifdef _WIN32
        const HWND foregroundWindow = ::GetForegroundWindow();

        RECT rcScreen;
        ::GetWindowRect(GetDesktopWindow(), &rcScreen);

        RECT rcForegroundApp;
        GetWindowRect(foregroundWindow, &rcForegroundApp);

        if (foregroundWindow != ::GetDesktopWindow() && foregroundWindow != ::GetShellWindow()) {
            return rcScreen.left == rcForegroundApp.left &&
                rcScreen.top == rcForegroundApp.top &&
                rcScreen.right == rcForegroundApp.right &&
                rcScreen.bottom == rcForegroundApp.bottom;
        }
#endif
        return false;
	}

	int scale_bitmap(const int _px)
	{
		return (_px * (is_mac_retina() ? 2 : 1));
	}

	QSize scale_bitmap(const QSize &_px)
	{
		return QSize(
			scale_bitmap(_px.width()),
			scale_bitmap(_px.height())
		);
	}

	QRect scale_bitmap(const QRect &_px)
	{
		return QRect(
			_px.left(),
			_px.top(),
			scale_bitmap(_px.width()),
			scale_bitmap(_px.height())
		);
	}

	int unscale_value(const int _px)
	{
		double scale = Utils::get_scale_coefficient();
		return (int)((double) _px / (scale == 0 ? 1.0 : scale));
	}

    template <typename _T>
    void check_pixel_ratio(_T& _image)
    {
        if (is_mac_retina())
        {
            _image.setDevicePixelRatio(2);
        }
    }
    template void check_pixel_ratio<QImage>(QImage& _pixmap);
    template void check_pixel_ratio<QPixmap>(QPixmap& _pixmap);

    QString parse_image_name(const QString& _imageName)
    {
        if (is_mac_retina())
        {
            QString result(_imageName);
            result.replace("/100/", "/200/");  // e.g. for /themes/standard/100/cl/sending_mark.png format
            result.replace("_100", "_200");
            return result;
        }

        QString result(_imageName);
        double scale = Utils::get_scale_coefficient();
        QString scaleString = QString::number(scale * 100);
        scaleString.prepend("_");
        result.replace("_100", scaleString);

        scaleString = QString::number(scale * 100);
        scaleString.prepend("/");
        scaleString.append("/");
        result.replace("/100/", scaleString);
        return result;
    }

	void addShadowToWidget(QWidget* target)
	{
		QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect(target);
		shadow->setColor(QColor(0, 0, 0, 75));
		shadow->setBlurRadius(scale_value(16));
		shadow->setXOffset(scale_value(0));
		shadow->setYOffset(scale_value(2));
		target->setGraphicsEffect(shadow);
	}

	void addShadowToWindow(QWidget* target, bool enabled)
	{
		int shadowWidth = enabled ? Ui::get_gui_settings()->get_shadow_width() : 0;
		if (enabled && !target->testAttribute(Qt::WA_TranslucentBackground))
			target->setAttribute(Qt::WA_TranslucentBackground);

        auto old_margins = target->contentsMargins();
		target->setContentsMargins(QMargins(old_margins.left() + shadowWidth, old_margins.top() + shadowWidth,
            old_margins.right() + shadowWidth, old_margins.bottom() + shadowWidth));

		static QPointer<QObject> eventFilter(new ShadowWidgetEventFilter(Ui::get_gui_settings()->get_shadow_width()));

		if (enabled)
			target->installEventFilter(eventFilter);
		else
			target->removeEventFilter(eventFilter);
	}

    void setWidgetPopup(QWidget* target, bool _isPopUp)
    {
#ifdef _WIN32
        if (_isPopUp)
            target->setWindowFlags(Qt::Popup | Qt::NoDropShadowWindowHint | Qt::FramelessWindowHint);
        else
        {
            auto flags = target->windowFlags();
            /*int framelessFlag = Qt::FramelessWindowHint;*/
            /*int popupFlag = Qt::Popup;*/

            /*int isFrameless = framelessFlag & flags;*/
            /*int isPopup = popupFlag & flags;*/


         //   flags &= ~Qt::FramelessWindowHint;
            flags &= ~Qt::Popup;
            target->setWindowFlags(flags);
        }
#else
        if (_isPopUp)
            target->setWindowFlags(Qt::Popup);
        else
        {
            auto flags = target->windowFlags();
            flags &= ~Qt::Popup;
            target->setWindowFlags(flags);
        }
#endif
    }

	void grabTouchWidget(QWidget* target, bool topWidget)
	{
#ifdef _WIN32
		if (topWidget)
		{
			QScrollerProperties sp;
			sp.setScrollMetric(QScrollerProperties::DragVelocitySmoothingFactor, 0.6);
			sp.setScrollMetric(QScrollerProperties::MinimumVelocity, 0.0);
			sp.setScrollMetric(QScrollerProperties::MaximumVelocity, 0.5);
			sp.setScrollMetric(QScrollerProperties::AcceleratingFlickMaximumTime, 0.4);
			sp.setScrollMetric(QScrollerProperties::AcceleratingFlickSpeedupFactor, 1.2);
			sp.setScrollMetric(QScrollerProperties::SnapPositionRatio, 0.2);
			sp.setScrollMetric(QScrollerProperties::MaximumClickThroughVelocity, 0);
			sp.setScrollMetric(QScrollerProperties::DragStartDistance, 0.001);
			sp.setScrollMetric(QScrollerProperties::MousePressEventDelay, 0);

			QScroller* clScroller = QScroller::scroller(target);
			clScroller->grabGesture(target);
			clScroller->setScrollerProperties(sp);
		}
		else
		{
			QScroller::grabGesture(target);
		}
#endif //WIN32
	}

	void removeLineBreaks(QString& source)
	{
		if (source.isEmpty())
			return;

		bool spaceAtEnd = source.at(source.length() - 1) == QChar::Space;
		source.replace('\n', QChar::Space);
		source.remove('\r');
		if (!spaceAtEnd && source.at(source.length() - 1) == QChar::Space)
			source = source.left(source.length() - 1);
	}

    QFont appFont(const FontsFamily _fontFamily, int size)
    {
        QFont font(appFontFamily(_fontFamily));
        font.setPixelSize(size);
        font.setWeight(appFontWeight(_fontFamily));

//#ifdef __APPLE__
//        font.setLetterSpacing(QFont::SpacingType::PercentageSpacing, 106);
//        font.setWordSpacing(scale_value(0));
//#endif

        return font;
    }

    QFont::Weight appFontWeight(const FontsFamily _fontFamily)
    {
#ifdef __APPLE__
        switch (_fontFamily)
        {
            case FontsFamily::SEGOE_UI_BOLD:
                return QFont::Weight::Medium;
            case FontsFamily::SEGOE_UI_LIGHT:
                return QFont::Weight::Thin;
            case FontsFamily::SEGOE_UI_SEMIBOLD:
                return QFont::Weight::Normal;
            default:
                return QFont::Weight::Light;
//            case FontsFamily::SEGOE_UI_BOLD:
//                return QFont::Weight::Bold;
//            case FontsFamily::SEGOE_UI_LIGHT:
//                return QFont::Weight::Light;
//            case FontsFamily::SEGOE_UI_SEMIBOLD:
//                return QFont::Weight::Medium;
//            default:
//                return QFont::Weight::Normal;
        }
#else
        switch (_fontFamily)
        {
            case FontsFamily::SEGOE_UI_BOLD:
                return QFont::Weight::Bold;
            case FontsFamily::SEGOE_UI_LIGHT:
                return QFont::Weight::Light;
            case FontsFamily::SEGOE_UI_SEMIBOLD:
                return QFont::Weight::DemiBold;
            default:
                return QFont::Weight::Normal;
        }
#endif
    }

    QString appFontWeightQss(const FontsFamily _fontFamily)
    {
#ifdef __APPLE__
        switch (_fontFamily)
        {
//                Thin     = 0,    // 100
//                ExtraLight = 12, // 200
//                Light    = 25,   // 300
//                Normal   = 50,   // 400
//                Medium   = 57,   // 500
//                DemiBold = 63,   // 600
//                Bold     = 75,   // 700
//                ExtraBold = 81,  // 800
//                Black    = 87    // 900

            case FontsFamily::SEGOE_UI_BOLD:
                return "500";
            case FontsFamily::SEGOE_UI_LIGHT:
                return "100";
            case FontsFamily::SEGOE_UI_SEMIBOLD:
                return "400";
            default:
                return "300";
//            case FontsFamily::SEGOE_UI_BOLD:
//                return "700";
//            case FontsFamily::SEGOE_UI_LIGHT:
//                return "300";
//            case FontsFamily::SEGOE_UI_SEMIBOLD:
//                return "500";
//            default:
//                return "400";
        }
#else
        switch (_fontFamily)
        {
            case FontsFamily::SEGOE_UI_BOLD:
                return "600";
            case FontsFamily::SEGOE_UI_LIGHT:
                return "100";
            case FontsFamily::SEGOE_UI_SEMIBOLD:
                return "500";
            default:
                return "400";
        }
#endif
    }

    const QString& appFontFamily(const FontsFamily _fontFamily)
    {
		static std::map<FontsFamily, QString> font_family_map;
		if (font_family_map.empty())
		{
#ifndef __APPLE__
			if (QSysInfo().windowsVersion() >= QSysInfo::WV_VISTA)
			{
				font_family_map.emplace(std::make_pair(FontsFamily::SEGOE_UI, QString("Segoe UI")));
				font_family_map.emplace(std::make_pair(FontsFamily::SEGOE_UI_BOLD, QString("Segoe UI Bold")));
				font_family_map.emplace(std::make_pair(FontsFamily::SEGOE_UI_SEMIBOLD, QString("Segoe UI Semibold")));
				font_family_map.emplace(std::make_pair(FontsFamily::SEGOE_UI_LIGHT, QString("Segoe UI Light")));
			}
			else
			{
				font_family_map.emplace(std::make_pair(FontsFamily::SEGOE_UI, QString("Arial")));
				font_family_map.emplace(std::make_pair(FontsFamily::SEGOE_UI_BOLD, QString("Arial")));
				font_family_map.emplace(std::make_pair(FontsFamily::SEGOE_UI_SEMIBOLD, QString("Arial")));
				font_family_map.emplace(std::make_pair(FontsFamily::SEGOE_UI_LIGHT, QString("Arial")));
			}
#else
			if (QSysInfo().macVersion() >= QSysInfo().MV_10_11)
			{
//                font_family_map.emplace(std::make_pair(FontsFamily::SEGOE_UI, QString(".SF NS Display")));
//                font_family_map.emplace(std::make_pair(FontsFamily::SEGOE_UI_BOLD, QString(".SF NS Display")));
//                font_family_map.emplace(std::make_pair(FontsFamily::SEGOE_UI_SEMIBOLD, QString(".SF NS Display")));
//                font_family_map.emplace(std::make_pair(FontsFamily::SEGOE_UI_LIGHT, QString(".SF NS Display")));
                font_family_map.emplace(std::make_pair(FontsFamily::SEGOE_UI, QString("Helvetica Neue")));
                font_family_map.emplace(std::make_pair(FontsFamily::SEGOE_UI_BOLD, QString("Helvetica Neue")));
                font_family_map.emplace(std::make_pair(FontsFamily::SEGOE_UI_SEMIBOLD, QString("Helvetica Neue")));
                font_family_map.emplace(std::make_pair(FontsFamily::SEGOE_UI_LIGHT, QString("Helvetica Neue")));
//                font_family_map.emplace(std::make_pair(FontsFamily::SEGOE_UI, QString("Open Sans")));
//                font_family_map.emplace(std::make_pair(FontsFamily::SEGOE_UI_BOLD, QString("Open Sans")));
//                font_family_map.emplace(std::make_pair(FontsFamily::SEGOE_UI_SEMIBOLD, QString("Open Sans")));
//                font_family_map.emplace(std::make_pair(FontsFamily::SEGOE_UI_LIGHT, QString("Open Sans")));
			}
			else
			{
                font_family_map.emplace(std::make_pair(FontsFamily::SEGOE_UI, QString("Helvetica Neue")));
                font_family_map.emplace(std::make_pair(FontsFamily::SEGOE_UI_BOLD, QString("Helvetica Neue Bold")));
                font_family_map.emplace(std::make_pair(FontsFamily::SEGOE_UI_SEMIBOLD, QString("Helvetica Neue Medium")));
                font_family_map.emplace(std::make_pair(FontsFamily::SEGOE_UI_LIGHT, QString("Helvetica Neue Light")));
			}
#endif
		}

		return font_family_map[_fontFamily];
    }

	QColor getSelectionColor()
	{
		QColor color("#579e1c");
		color.setAlphaF(0.5);
		return color;
	}

    QString rgbaStringFromColor(const QColor& _color)
    {
        QString textColorString = QString("rgba(%1, %2, %3, %4%)").arg(_color.red()).arg(_color.green()).arg(_color.blue()).arg(_color.alpha()*100/255);
        return textColorString;
    }

	bool mac_retina = false;

	bool is_mac_retina()
	{
		return mac_retina;
	}

	void set_mac_retina(bool _val)
	{
		mac_retina = _val;
	}

	double scale_coefficient = 1.0;

	double get_scale_coefficient()
    {
		return scale_coefficient;
	}

	void set_scale_coefficient(double _coefficient)
	{
        if (platform::is_apple())
        {
            scale_coefficient = 1.0;
        }
        else if (_coefficient == 1.0 ||
            _coefficient == 1.25 ||
            _coefficient == 1.5 ||
            _coefficient == 2.0)
        {
            scale_coefficient = _coefficient;
        }
        else
        {
            assert(!"strange scale value");
            scale_coefficient = 1.0;
        }
	}

    namespace { double basic_scale_coefficient = 1.0; }

    double get_basic_scale_coefficient()
    {
        return basic_scale_coefficient;
    }

    void init_basic_scale_coefficient(double _coefficient)
    {
        static bool __init_basic_scale_coefficient = false;
        if (!__init_basic_scale_coefficient)
            basic_scale_coefficient = _coefficient;
        else
            assert(!"init_basic_scale_coefficient should be called once.");
    }

    void groupTaskbarIcon(bool enabled)
    {
#ifdef _WIN32
        if (QSysInfo().windowsVersion() >= QSysInfo::WV_WINDOWS7)
        {
            HMODULE lib_shell32 = ::GetModuleHandle(L"shell32.dll");
            typedef HRESULT  (__stdcall * SetCurrentProcessExplicitAppUserModelID_Type)(PCWSTR);
            SetCurrentProcessExplicitAppUserModelID_Type SetCurrentProcessExplicitAppUserModelID_Func;
            SetCurrentProcessExplicitAppUserModelID_Func = (SetCurrentProcessExplicitAppUserModelID_Type)::GetProcAddress(lib_shell32,"SetCurrentProcessExplicitAppUserModelID");
            SetCurrentProcessExplicitAppUserModelID_Func(enabled ? application_user_model_id : L"");
        }
#endif //_WIN32
    }

    bool is_start_on_startup()
    {
#ifdef _WIN32
        CRegKey key_software_run;
        if (ERROR_SUCCESS != key_software_run.Open(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", KEY_READ))
            return false;

        wchar_t buffer_path[1025];
        ULONG len = 1024;

        if (key_software_run.QueryStringValue((const wchar_t*) product_name.utf16(), buffer_path, &len) != ERROR_SUCCESS)
            return false;

#endif //_WIN32
        return true;
    }

    void set_start_on_startup(bool _start)
    {
#ifdef _WIN32

        bool current_state = is_start_on_startup();
        if (current_state == _start)
            return;

        if (_start)
        {
            CRegKey key_software_run;
            if (ERROR_SUCCESS != key_software_run.Open(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", KEY_SET_VALUE))
                return;

            wchar_t buffer[1025];
            if (!::GetModuleFileName(0, buffer, 1024))
                return;

            CAtlString exe_path = buffer;

            if (ERROR_SUCCESS != key_software_run.SetStringValue((const wchar_t*) product_name.utf16(), CAtlString("\"") + exe_path + "\""))
                return;
        }
        else
        {
            ::SHDeleteValue(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", (const wchar_t*) product_name.utf16());
        }
#endif //_WIN32
    }

#ifdef _WIN32
    HWND create_fake_parent_window()
    {
        HINSTANCE instance = (HINSTANCE) ::GetModuleHandle(0);
        HWND hwnd = 0;

        CAtlString class_name = L"fake_parent_window";
        WNDCLASSEX wc = {0};

        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = DefWindowProc;
        wc.hInstance = instance;
        wc.lpszClassName = (LPCWSTR) class_name;
        if (!::RegisterClassEx(&wc))
            return hwnd;

        hwnd = ::CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT, (LPCWSTR) class_name, 0, WS_POPUP, 0, 0, 0, 0, 0, 0, instance, 0);
        if (!hwnd)
            return hwnd;

        ::SetLayeredWindowAttributes(
            hwnd,
            0,
            0,
            LWA_ALPHA
            );

        return hwnd;
    }
#endif //WIN32

    const uint get_input_maximum_chars()
    {
        return 10000;
    }

    bool state_equals_online(const QString &state)
    {
        auto l = state.toLower();
//        return (l == "online" || l == "mobile"); // enumerate all possible states or just check if it's offline?
        return (l != "offline" && l.size());
    }

    int calc_age(const QDateTime& _birthdate)
    {
        QDate thisdate = QDateTime::currentDateTime().date();
        QDate birthdate = _birthdate.date();

        int age = thisdate.year() - birthdate.year();
        if (age < 0)
            return 0;

        if ((birthdate.month() > thisdate.month()) || (birthdate.month() == thisdate.month() && birthdate.day() > thisdate.day()))
            return (--age);

        return age;
    }

    void init_crash_handlers_in_core()
    {
#ifdef _WIN32
        QLibrary libcore(CORELIBRARY);
        if (!libcore.load())
        {
            assert(false);
        }

        typedef bool (*init_handlers_function)();
        init_handlers_function init_handlers_instance = (init_handlers_function) libcore.resolve("init_crash_handlers");
        init_handlers_instance();
#endif // _WIN32
    }

    const QString &DefaultDownloadsPath()
    {
        static QString defaultDownloadsPath;
        if (!defaultDownloadsPath.length())
        {
            //defaultDownloadsPath = QDir::toNativeSeparators(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)) + QDir::toNativeSeparators("/ICQ");
            defaultDownloadsPath = QDir::toNativeSeparators(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
            QDir().mkpath(defaultDownloadsPath);
        }
        return defaultDownloadsPath;
    }

    bool is_image_extension(const QString &ext)
    {
        assert(!ext.isEmpty());

        static QStringList imagesExtensions;

        if (imagesExtensions.isEmpty())
        {
            imagesExtensions << "jpg" << "png" << "jpeg" << "gif";
        }

        return imagesExtensions.contains(ext.toLower());
    }

    void copyFileToClipboard(const QString& path)
    {
        QMimeData* mimeData = new QMimeData();
        mimeData->setData("text/uri-list", QUrl::fromLocalFile(path).toString().toStdString().c_str());
        QApplication::clipboard()->setMimeData(mimeData);
    }

    bool saveAs(const QString& inputFilename, QString& filename, QString& directory)
    {
        static auto last_directory = QDir::toNativeSeparators(Ui::get_gui_settings()->get_value(settings_download_directory, Utils::DefaultDownloadsPath()));
        
        filename.clear();
        directory.clear();

        int dot = inputFilename.lastIndexOf('.');
        QString ext = dot != -1 ? inputFilename.mid(dot, inputFilename.length()) : QString();
        QString name = (inputFilename.contains(QRegExp("\\/:*?\"<>\\|\"")) || inputFilename.length() >= 128) ? QT_TRANSLATE_NOOP("filesharing_widget", "File") : inputFilename;
        QString full_name = QDir::toNativeSeparators(QDir(last_directory).filePath(name));
        QString destination = QFileDialog::getSaveFileName(0, QT_TRANSLATE_NOOP("context_menu", "Save as..."), full_name, "*" + ext);
        if (!destination.isEmpty())
        {
            QFileInfo info(destination);
            last_directory = info.dir().absolutePath();
            directory = info.dir().absolutePath();
            filename = info.fileName();
            if (info.suffix().isEmpty() && !ext.isEmpty())
                filename += ext;

            return true;
        }
        return false;
    }

    const std::vector<QString> get_keys_send_by_names()
    {
        std::vector<QString> names;
        names.push_back("Enter");
        names.push_back(platform::is_apple() ? "Cmd+Enter" : "Ctrl+Enter");
        names.push_back("Shift+Enter");
        // names.push_back("Enter+Enter");
        return names;
    }

    int get_key_send_by_index()
    {
         int key_1_to_send = Ui::get_gui_settings()->get_value<int>(settings_key1_to_send_message, Qt::NoModifier);
         return ((key_1_to_send == Qt::Key_Control) * 1) + ((key_1_to_send == Qt::Key_Shift) * 2) + ((key_1_to_send == Qt::Key_Enter) * 3);
    }

    void post_stats_with_settings()
    {
        core::stats::event_props_type props;
        props.emplace_back("size", std::to_string(qApp->desktop()->screenGeometry().width()) + "x" + std::to_string(qApp->desktop()->screenGeometry().height()));
        QString processorInfo = QString("Architecture:%1 \nNumber of Processors:%2 \nProcessor Identifier:%3").arg(getenv("PROCESSOR_ARCHITECTURE")).arg(getenv("NUMBER_OF_PROCESSORS")).arg(getenv("PROCESSOR_IDENTIFIER"));
        props.emplace_back("Sys_CPU", processorInfo.toStdString()); // QSysInfo::currentCpuArchitecture().toStdString());

        props.emplace_back(std::make_pair("Settings_Startup", std::to_string(Utils::is_start_on_startup())));
        props.emplace_back(std::make_pair("Settings_Taskbar", std::to_string(Ui::get_gui_settings()->get_value<bool>(settings_show_in_taskbar, true))));
        props.emplace_back(std::make_pair("Settings_Sounds", std::to_string(Ui::get_gui_settings()->get_value<bool>(settings_sounds_enabled, true))));

        auto current_download_dir = Ui::get_gui_settings()->get_value<QString>(settings_download_directory, Utils::DefaultDownloadsPath());
        props.emplace_back(std::make_pair("Settings_Download_Folder", std::to_string(current_download_dir == Utils::DefaultDownloadsPath())));

        int key_index_to_send = Utils::get_key_send_by_index();
        props.emplace_back(std::make_pair("Settings_Send_By", Utils::get_keys_send_by_names()[key_index_to_send].toStdString()));
        props.emplace_back(std::make_pair("Settings_Show_Last_Message", std::to_string(Ui::get_gui_settings()->get_value<bool>(settings_show_last_message, true))));
        props.emplace_back(std::make_pair("Settings_Previews", std::to_string(Ui::get_gui_settings()->get_value<bool>(settings_show_video_and_images, true))));
        props.emplace_back(std::make_pair("Settings_Scale", std::to_string(Utils::get_scale_coefficient())));
        props.emplace_back(std::make_pair("Settings_Language", Ui::get_gui_settings()->get_value(settings_language, QString("")).toUpper().toStdString()));

        // props.emplace_back(std::make_pair("Settings_Wallpaper_Global", std::to_string()));
        // props.emplace_back(std::make_pair("Settings_Wallpaper_Local", std::to_string()));

        props.emplace_back(std::make_pair("Settings_Sounds_Outgoing", std::to_string(Ui::get_gui_settings()->get_value<bool>(settings_outgoing_message_sound_enabled, true))));
        props.emplace_back(std::make_pair("Settings_Alerts", std::to_string(Ui::get_gui_settings()->get_value<bool>(settings_notify_new_messages, true))));

        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::client_settings, props);
    }

    QRect GetMainRect()
    {
        assert(!!Utils::InterConnector::instance().getMainWindow() && "Common.cpp (ItemLength)");
        QRect main_rect(0, 0, 0, 0);
        if (Utils::InterConnector::instance().getMainWindow())
        {
            main_rect = Utils::InterConnector::instance().getMainWindow()->geometry();
        }
        else if (auto window = qApp->activeWindow())
        {
            main_rect = window->geometry();
        }
        assert("Couldn't get rect: Common.cpp (ItemLength)");
        return main_rect;
    }
}
