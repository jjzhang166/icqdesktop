#include "stdafx.h"

#include "ImageCache.h"

#include "ImageIterator.h"

Previewer::ImageIterator::ImageIterator(ImageCache* _imageCache, const Data::Image& _image)
    : QObject(nullptr)
    , imageCache_(_imageCache)
    , image_(_image)
    , number_(-1)
    , passed_(0)
{
    connect(imageCache_, &ImageCache::imagesGetted, this, &ImageIterator::onImagesGetted, Qt::QueuedConnection);
}

bool Previewer::ImageIterator::hasPrev() const
{
    return hasRelative(-1);
}

void Previewer::ImageIterator::prev()
{
    assert(hasPrev());
    ++number_;
}

bool Previewer::ImageIterator::hasNext() const
{
    return hasRelative(1);
}

void Previewer::ImageIterator::next()
{
    assert(hasNext());
    --number_;
}

bool Previewer::ImageIterator::hasRelative(int _n) const
{
    if (number_ == -1)
        return false;

    const int size = imageCache_->images_.size();
    const int index = getIndex(_n);
    return index >= 0 && index < size;
}

const Data::Image& Previewer::ImageIterator::peekImageRelative(int _n) const
{
    assert(hasRelative(_n));
    return imageCache_->images_[getIndex(_n)];
}

int Previewer::ImageIterator::getNumber() const
{
    std::lock_guard<std::mutex> lock(imageCache_->mutex_);

    return number_ == -1 ? 1 : number_;
}

int Previewer::ImageIterator::getTotal() const
{
    std::lock_guard<std::mutex> lock(imageCache_->mutex_);
    int size = imageCache_->images_.size();
    return size == 0 ? 1 : size;
}

void Previewer::ImageIterator::onImagesGetted(bool isCacheLoaded)
{
    if (number_ == -1)
        findImageNumber();

    if (isCacheLoaded)
    {
        if (number_ == -1)
        {
            // can't find image so need to rebuild cache
            imageCache_->repair();
        }
        else
        {
            emit cacheLoaded();
        }
    }
    else
    {
        emit iteratorUpdated();
    }
}

int Previewer::ImageIterator::getIndex(int _n) const
{
    const int size = imageCache_->images_.size();
    return size - number_ + _n;
}

void Previewer::ImageIterator::findImageNumber()
{
    std::lock_guard<std::mutex> lock(imageCache_->mutex_);

    for (int i = imageCache_->images_.size() - passed_ - 1; i >= 0; --i)
    {
        if (imageCache_->images_[i] == image_)
        {
            number_ = imageCache_->images_.size() - i;
            emit numberFound();
            return;
        }
    }
}
