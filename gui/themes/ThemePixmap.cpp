#include "stdafx.h"

#include "../utils/utils.h"

#include "Themes.h"
#include "ResourceIds.h"

#include "ThemePixmap.h"

using namespace Themes;

namespace
{
	class ThemePixmap : public IThemePixmap
	{
	public:
		ThemePixmap(const ThemeId themeId, const PixmapResourceId resourceId);

		virtual ~ThemePixmap() override;

		virtual void Draw(QPainter &p, const int32_t x, const int32_t y) const override;

		virtual void Draw(QPainter &p, const int32_t x, const int32_t y, const int32_t w, const int32_t h) const override;

		virtual void Draw(QPainter &p, const QRect &r) const override;

		virtual int32_t GetWidth() const override;

		virtual int32_t GetHeight() const override;

		virtual QRect GetRect() const override;

        virtual QSize GetSize() const override;

	private:
		void PreparePixmap() const;

		const PixmapResourceId ResourceId_;

		mutable ThemeId ThemeId_;

		mutable QPixmap Pixmap_;

	};

	std::unordered_map<int, QString> ResourcePaths_;

	std::unordered_map<int, IThemePixmapSptr> ResourcePixmaps_;

	void InitResourcePaths();
}

namespace Themes
{
	IThemePixmap::~IThemePixmap()
	{
	}

	const IThemePixmapSptr& GetPixmap(const PixmapResourceId id)
	{
		assert(id > PixmapResourceId::Min);
		assert(id < PixmapResourceId::Max);

		const auto iterCache = ResourcePixmaps_.find((int)id);
		if (iterCache != ResourcePixmaps_.end())
		{
			return iterCache->second;
		}

		auto themePixmap = std::make_shared<ThemePixmap>(GetCurrentThemeId(), id);

		const auto result = ResourcePixmaps_.emplace((int)id, themePixmap);
		assert(result.second);

		return result.first->second;
	}
}

namespace
{
	ThemePixmap::ThemePixmap(const ThemeId themeId, const PixmapResourceId resourceId)
		: ThemeId_(themeId)
		, ResourceId_(resourceId)
	{
		assert(ThemeId_ > ThemeId::Min);
		assert(ThemeId_ < ThemeId::Max);
		assert(ResourceId_ > PixmapResourceId::Min);
		assert(ResourceId_ < PixmapResourceId::Max);
	}

	ThemePixmap::~ThemePixmap()
	{
	}

	void ThemePixmap::Draw(QPainter &p, const int32_t x, const int32_t y) const
	{
		PreparePixmap();

		p.drawPixmap(x, y, Pixmap_);
	}

	void ThemePixmap::Draw(QPainter &p, const int32_t x, const int32_t y, const int32_t w, const int32_t h) const
	{
		assert(w > 0);
		assert(h > 0);

		PreparePixmap();

		p.drawPixmap(x, y, w, h, Pixmap_);
	}

	void ThemePixmap::Draw(QPainter &p, const QRect &r) const
	{
		assert(r.width() > 0);
		assert(r.height() > 0);
		PreparePixmap();
		p.drawPixmap(r, Pixmap_);
	}

	int32_t ThemePixmap::GetWidth() const
	{
		PreparePixmap();

		return Pixmap_.width() / Pixmap_.devicePixelRatio();
	}

	int32_t ThemePixmap::GetHeight() const
	{
		PreparePixmap();

		return Pixmap_.height() / Pixmap_.devicePixelRatio();
	}

	QRect ThemePixmap::GetRect() const
	{
		PreparePixmap();
#ifdef __APPLE__
        return QRect(0, 0, Pixmap_.width() / Pixmap_.devicePixelRatio(), Pixmap_.height() / Pixmap_.devicePixelRatio());
#else
        return Pixmap_.rect();
#endif
	}

    QSize ThemePixmap::GetSize() const
    {
        PreparePixmap();

        return Pixmap_.size();
    }

	void ThemePixmap::PreparePixmap() const
	{
		const auto isThemeChanged = (ThemeId_ != GetCurrentThemeId());
		const auto isPixmapNotLoaded = Pixmap_.isNull();
		const auto shouldPreparePixmap = (isThemeChanged || isPixmapNotLoaded);
		if (!shouldPreparePixmap)
		{
			return;
		}

		InitResourcePaths();

		const auto pathIter = ResourcePaths_.find((int)ResourceId_);
		if (pathIter == ResourcePaths_.end())
		{
			assert(!"unknown resource identifier");
			return;
		}

		ThemeId_ = GetCurrentThemeId();

		auto resPath = pathIter->second;
		resPath = resPath.arg(GetThemeNameById(ThemeId_));
		if (!Pixmap_.load(resPath))
		{
			assert(!"failed to load a pixmap from resources");
		}

        Utils::check_pixel_ratio(Pixmap_);

		assert(!Pixmap_.isNull());
	}

	void InitResourcePaths()
	{
		if (!ResourcePaths_.empty())
		{
			return;
		}

		static const std::tuple<PixmapResourceId, const char*, const char*> index[] =
		{
			#include "ThemePixmapDb.inc"
		};

		const auto scale = QString::number(Utils::scale_bitmap(Utils::scale_value(100)));

		QString resPath;
		resPath.reserve(256);

		for (const auto &pair : index)
		{
			const auto resId = std::get<0>(pair);
			const auto resDir = std::get<1>(pair);
			const auto resFilename = std::get<2>(pair);

			resPath.resize(0);
			resPath += ":/themes/%1/";
			resPath += scale;
			resPath += "/";
			resPath += resDir;
			resPath += "/";
			resPath += resFilename;
			resPath += ".png";

			const auto emplaceResult = ResourcePaths_.emplace((int)resId, resPath);
			assert(emplaceResult.second);
		}
	}
}