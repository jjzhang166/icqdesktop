#include "stdafx.h"
#include "FFMpegPlayer.h"
#include "../../utils/utils.h"

namespace Ui
{
    const int32_t maxQueueSize(1024*1024*15);
    const int32_t minFramesCount(25);

    static ffmpeg::AVPacket flush_pkt;

    bool ThreadMessagesQueue::getMessage(ThreadMessage& _message, std::function<bool()> _isQuit, int32_t _wait_timeout)
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);

        condition_.wait_for(lock, std::chrono::milliseconds(_wait_timeout), [this, _isQuit]
        {
            return  _isQuit() || !messages_.empty();
        });

        if (_isQuit() || messages_.empty())
        {
            return false;
        }

        _message = messages_.front();

        messages_.pop_front();

        return true;
    }

    void ThreadMessagesQueue::pushMessage(const ThreadMessage& _message)
    {
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);

            messages_.push_back(_message);
        }

        condition_.notify_one();
    }



    //////////////////////////////////////////////////////////////////////////
    // PacketQueue
    //////////////////////////////////////////////////////////////////////////
    PacketQueue::PacketQueue()
        :   packets_(0),
            size_(0)
    {
    }

    PacketQueue::~PacketQueue()
    {
        free();
    }

    void PacketQueue::free()
    {
        QMutexLocker locker(&mutex_);

        for (auto iter = list_.begin(); iter != list_.end(); ++iter)
        {
            if (iter->data && iter->data != (uint8_t*) &flush_pkt)
            {
                ffmpeg::av_packet_unref(&(*iter));
            }
        }

        list_.clear();
        size_ = 0;
        packets_ = 0;
    }

    void PacketQueue::push(ffmpeg::AVPacket* _packet)
    {
        QMutexLocker locker(&mutex_);

        list_.push_back(*_packet);

        ++packets_;
        size_ += _packet->size;
    }

    bool PacketQueue::get(/*out*/ ffmpeg::AVPacket& _packet)
    {
        QMutexLocker locker(&mutex_);

        if (list_.empty())
        {
            return false;
        }

        _packet = list_.front();

        list_.pop_front();

        --packets_;
        size_ -= _packet.size;

        return true;
    }


    int32_t PacketQueue::getSize() const
    {
        return size_;
    }

    int32_t PacketQueue::getPackets() const
    {
        return packets_;
    }
    



    //////////////////////////////////////////////////////////////////////////
    // VideoContext
    //////////////////////////////////////////////////////////////////////////
    VideoContext::VideoContext()
        :   formatContext_(0),
            videoStream_(0),
            audioStream_(0),
            swsContext_(0),
            quit_(false),
            frameTimer_(0.0),
            videoClock_(0.0),
            audioClock_(0.0),
            frameLastPts_(0.0),
            frameLastDelay_(0.0),
            width_(0),
            height_(0),
            rotation_(0),
            duration_(0),
            frameRGB_(0),
            volume_(100),
            mute_(false),
            startTime_(0),
            startTimeSet_(false)
    {

    }

    void VideoContext::init()
    {
        pushVideoPacket(&flush_pkt);
        pushAudioPacket(&flush_pkt);
    }

    ffmpeg::AVStream* VideoContext::openStream(int32_t _type)
    {
        ffmpeg::AVStream* stream = 0;
        assert(formatContext_);

        int32_t streamIndex = ffmpeg::av_find_best_stream(formatContext_, (ffmpeg::AVMediaType) _type, -1, -1, NULL, 0);
        if (streamIndex < 0)
        {
            return 0;
        }

        stream = formatContext_->streams[streamIndex];

        ffmpeg::AVCodecContext* codecContext = stream->codec;

        // Find suitable codec
        ffmpeg::AVCodec* codec = ffmpeg::avcodec_find_decoder(codecContext->codec_id);

        if (!codec)
        {
            // Codec not found
            return 0;
        }

        if (avcodec_open2(codecContext, codec, 0) < 0)
        {
            // Failed to open codec
            return 0;
        }

        return stream;
    }

    void VideoContext::closeStream(ffmpeg::AVStream* _stream)
    {
        if (_stream && _stream->codec)
        {
            avcodec_close(_stream->codec);
        }
    }

    int32_t VideoContext::getVideoStreamIndex() const
    {
        return videoStream_->index;
    }

    int32_t VideoContext::getAudioStreamIndex() const
    {
        if (audioStream_)
            return audioStream_->index;

        return -1;
    }

    bool VideoContext::isQuit() const
    {
        return quit_;
    }

    void VideoContext::setQuit(bool _val)
    {
        quit_ = _val;
    }


    int32_t VideoContext::readAVPacket(/*OUT*/ffmpeg::AVPacket* _packet)
    {
        return ffmpeg::av_read_frame(formatContext_, _packet);
    }

    int32_t VideoContext::readAVPacketPause()
    {
        return ffmpeg::av_read_pause(formatContext_);
    }

    int32_t VideoContext::readAVPacketPlay()
    {
        return ffmpeg::av_read_play(formatContext_);
    }

    bool VideoContext::isEof(int32_t _error)
    {
        return (_error == AVERROR_EOF || ffmpeg::avio_feof(formatContext_->pb));
    }

    bool VideoContext::isStreamError()
    {
        if (formatContext_->pb && formatContext_->pb->error)
        {
            assert(false);
            return true;
        }

        return false;
    }

    bool VideoContext::getNextVideoFrame(/*OUT*/ffmpeg::AVFrame* _frame, ffmpeg::AVPacket* _packet, bool& _stream_finished, bool& _eof)
    {
        ffmpeg::AVCodecContext* videoCodecContext = videoStream_->codec;

        //ffmpeg::AVPacket packet;

        while (!isQuit())
        {
            if (!_stream_finished)
            {
                if (!videoQueue_.get(*_packet))
                {
                    continue;
                }
            }
            else
            {

            }

            if (_packet->data == (uint8_t*) &flush_pkt)
            {
                flushVideoBuffers();

                continue;
            }

            if (!_packet->data)
            {
                _stream_finished = true;
            }
            
            int got_frame = 0;

            int len = ffmpeg::avcodec_decode_video2(videoCodecContext, _frame, &got_frame, _packet);

            if (_packet->data)
            {
                ffmpeg::av_packet_unref(_packet);
            }
            
            if (len < 0)
            {
                return false;
            }

            if (got_frame)
            {
                return true;
            }
            else if (_stream_finished)
            {
                _eof = true;

                return false;
            }
        }

        return false;
    }

    void pushNullPacket(PacketQueue& _queue, int32_t _streamIndex)
    {
        ffmpeg::AVPacket pkt1, *pkt = &pkt1;
        ffmpeg::av_init_packet(pkt);
        pkt->data = NULL;
        pkt->size = 0;
        pkt->stream_index = _streamIndex;

        _queue.push(pkt);
    }

    void VideoContext::pushVideoPacket(ffmpeg::AVPacket* _packet)
    {
        if (!_packet)
            return pushNullPacket(videoQueue_, getVideoStreamIndex());
        
        return videoQueue_.push(_packet);
    }

    int32_t VideoContext::getVideoQueuePackets() const
    {
        return videoQueue_.getPackets();
    }

    int32_t VideoContext::getVideoQueueSize() const
    {
        return videoQueue_.getSize();
    }

    void VideoContext::pushAudioPacket(ffmpeg::AVPacket* _packet)
    {
        if (!_packet)
            return pushNullPacket(audioQueue_, getAudioStreamIndex());

        return audioQueue_.push(_packet);
    }

    int32_t VideoContext::getAudioQueuePackets() const
    {
        return audioQueue_.getPackets();
    }

    int32_t VideoContext::getAudioQueueSize() const
    {
        return audioQueue_.getSize();
    }

    bool VideoContext::openFile(const QString& _file)
    {
        int32_t err = 0;

        err = ffmpeg::avformat_open_input(&formatContext_, _file.toStdString().c_str(), 0, 0);
        if (err < 0)
        {
            return false;
        }

        // Retrieve stream information
        err = ffmpeg::avformat_find_stream_info(formatContext_, 0);
        if (err < 0)
        {
            return false;
        }

        // Open video and audio streams
        videoStream_ = openStream(ffmpeg::AVMEDIA_TYPE_VIDEO);
        if (!videoStream_)
        {
            return false;
        }

        audioStream_ = openStream(ffmpeg::AVMEDIA_TYPE_AUDIO);
        if (!audioStream_)
        {

        }

        // Create conversion context
        ffmpeg::AVCodecContext* videoCodecContext = videoStream_->codec;

        ffmpeg::AVDictionary* dictionary = videoStream_->metadata;

        if (dictionary)
        {
            ffmpeg::AVDictionaryEntry* entryRotate = ffmpeg::av_dict_get(dictionary, "rotate", 0, AV_DICT_IGNORE_SUFFIX);

            if (entryRotate && entryRotate->value && entryRotate->value[0] != '\0')
            {
                rotation_ = QString(entryRotate->value).toInt();
            }
        }

        width_ = videoCodecContext->width;
        height_ = videoCodecContext->height;
        duration_ = formatContext_->duration / (AV_TIME_BASE / 1000);

        scaledSize_.setHeight(height_);
        scaledSize_.setWidth(width_);

        resetFrameTimer();

        return true;
    }

    void VideoContext::closeFile()
    {
        closeStream(audioStream_);

        closeStream(videoStream_);

        if (formatContext_)
        {
            avformat_close_input(&formatContext_);
        }
    }

    int32_t VideoContext::getWidth() const
    {
        return width_;
    }

    int32_t VideoContext::getHeight() const
    {
        return height_;
    }

    int32_t VideoContext::getRotation() const
    {
        return rotation_;
    }

    int64_t VideoContext::getDuration() const
    {
        return duration_;
    }

    QSize VideoContext::getScaledSize() const
    {
        return scaledSize_;
    }


    double VideoContext::getVideoTimebase()
    {
        return ffmpeg::av_q2d(videoStream_->time_base);
    }

    double VideoContext::synchronizeVideo(ffmpeg::AVFrame* _frame, double _pts)
    {
        ffmpeg::AVCodecContext* videoCodecContext = videoStream_->codec;
        if (_pts >  std::numeric_limits<double>::epsilon())
        {
            /* if we have pts, set video clock to it */
            videoClock_ = _pts;
        }
        else
        {
            /* if we aren't given a pts, set it to the clock */
            _pts = videoClock_;
        }
        /* update the video clock */
        double frameDelay = ffmpeg::av_q2d(videoCodecContext->time_base);
        /* if we are repeating a frame, adjust clock accordingly */

        frameDelay += _frame->repeat_pict * (frameDelay * 0.5);
        videoClock_ += frameDelay;

        return _pts;
    }

    double VideoContext::computeDelay(double _picturePts)
    {
        double delay = _picturePts - frameLastPts_;

        if (delay <= 0.0 || delay >= 1.0) {
            // Delay incorrect - use previous one
            delay = frameLastDelay_;
        }
        // Save for next time
        frameLastPts_ = _picturePts;
        frameLastDelay_ = delay;

        // Update delay to sync to audio
    /*    double ref_clock = get_audio_clock(main_context);
        double diff = main_context->pict.pts - ref_clock;
        double sync_threshold = FFMAX(AV_SYNC_THRESHOLD, delay);
        if (fabs(diff) < AV_NOSYNC_THRESHOLD) {
            if (diff <= -sync_threshold) {
                delay = 0;
            } else if (diff >= sync_threshold) {
                delay = 2 * delay;
            }
        }*/

        frameTimer_ += delay;

        double actual_delay = frameTimer_ - (ffmpeg::av_gettime() / 1000000.0);

        if (actual_delay < 0.010)
        {
            actual_delay = 0.010;
        }

        return actual_delay;
    }


    bool VideoContext::initDecodeAudioData()
    {
        if (!enableAudio())
            return true;

        audioData_.frame_ = ffmpeg::av_frame_alloc();

        // Generate some AL Buffers for streaming
        openal::alGenBuffers(audio::num_buffers, audioData_.uiBuffers_);

        // Generate a Source to playback the Buffers
        openal::alGenSources(1, &audioData_.uiSource_);
        
        audioData_.audioCodecContext_ = audioStream_->codec;

        audioData_.layout_ = audioData_.audioCodecContext_->channel_layout;
        audioData_.channels_ = audioData_.audioCodecContext_->channels;
        audioData_.freq_ = audioData_.audioCodecContext_->sample_rate;
        

        if (audioData_.layout_ == 0 && audioData_.channels_ > 0)
        {
            if (audioData_.channels_ == 1)
                audioData_.layout_ = AV_CH_LAYOUT_MONO;
            else
                audioData_.layout_ = AV_CH_LAYOUT_STEREO;
        }

        ffmpeg::AVSampleFormat inputFormat = audioData_.audioCodecContext_->sample_fmt;

        switch (audioData_.layout_)
        {
        case AV_CH_LAYOUT_MONO:
            switch (inputFormat)
            {
            case ffmpeg::AV_SAMPLE_FMT_U8:
            case ffmpeg::AV_SAMPLE_FMT_U8P:
                audioData_.fmt_ = AL_FORMAT_MONO8;
                audioData_.sampleSize_ = 1;
                break;

            case ffmpeg::AV_SAMPLE_FMT_S16:
            case ffmpeg::AV_SAMPLE_FMT_S16P:
                audioData_.fmt_ = AL_FORMAT_MONO16;
                audioData_.sampleSize_ = sizeof(uint16_t);
                break;

            default:
                audioData_.sampleSize_ = -1;
                break;
            }
            break;

        case AV_CH_LAYOUT_STEREO:
            switch (inputFormat)
            {
            case ffmpeg::AV_SAMPLE_FMT_U8:
                audioData_.fmt_ = AL_FORMAT_STEREO8;
                audioData_.sampleSize_ = 2;
                break;

            case ffmpeg::AV_SAMPLE_FMT_S16:
                audioData_.fmt_ = AL_FORMAT_STEREO16;
                audioData_.sampleSize_ = 2 * sizeof(uint16_t);
                break;

            default:
                audioData_.sampleSize_ = -1;
                break;
            }
            break;

        default:
            audioData_.sampleSize_ = -1;
            break;
        }

        if (audioData_.freq_ != 44100 && audioData_.freq_ != 48000)
            audioData_.sampleSize_ = -1;

        if (audioData_.sampleSize_ < 0) 
        {
            audioData_.swrContext_ = ffmpeg::swr_alloc();
            if (!audioData_.swrContext_)
                return false;

            int64_t src_ch_layout = audioData_.layout_, dst_ch_layout = audio::outChannelLayout;
            audioData_.srcRate_ = audioData_.freq_;
            ffmpeg::AVSampleFormat src_sample_fmt = inputFormat, dst_sample_fmt = audio::outFormat;
            audioData_.dstRate_ = (audioData_.freq_ != 44100 && audioData_.freq_ != 48000) ? audio::outFrequency : audioData_.freq_;

            ffmpeg::av_opt_set_int(audioData_.swrContext_, "in_channel_layout", src_ch_layout, 0);
            ffmpeg::av_opt_set_int(audioData_.swrContext_, "in_sample_rate", audioData_.srcRate_, 0);
            ffmpeg::av_opt_set_sample_fmt(audioData_.swrContext_, "in_sample_fmt", src_sample_fmt, 0);
            ffmpeg::av_opt_set_int(audioData_.swrContext_, "out_channel_layout", dst_ch_layout, 0);
            ffmpeg::av_opt_set_int(audioData_.swrContext_, "out_sample_rate", audioData_.dstRate_, 0);
            ffmpeg::av_opt_set_sample_fmt(audioData_.swrContext_, "out_sample_fmt", dst_sample_fmt, 0);

            if (swr_init(audioData_.swrContext_) < 0)
                return false;

            audioData_.sampleSize_ = audio::outChannels * sizeof(uint16_t);
            audioData_.freq_ = audioData_.dstRate_;
            //!!!int64_t len_ = ffmpeg::av_rescale_rnd(Len_, DstRate_, SrcRate_, AV_ROUND_UP);
            audioData_.fmt_ = AL_FORMAT_STEREO16;

            audioData_.maxResampleSamples_ = ffmpeg::av_rescale_rnd(audio::blockSize / audioData_.sampleSize_, audioData_.dstRate_, audioData_.srcRate_, ffmpeg::AV_ROUND_UP);
            if (ffmpeg::av_samples_alloc_array_and_samples(&audioData_.outSamplesData_, 0, audio::outChannels, audioData_.maxResampleSamples_, audio::outFormat, 0) < 0)
                return false;
        }

        return true;
    }

    void VideoContext::freeDecodeAudioData()
    {
        if (audioData_.frame_)
            ffmpeg::av_frame_free(&audioData_.frame_);

        if (audioData_.swrContext_)
            ffmpeg::swr_free(&audioData_.swrContext_);

        openal::alDeleteSources(1, &audioData_.uiSource_);
        openal::alDeleteBuffers(audio::num_buffers, audioData_.uiBuffers_);
    }


    bool VideoContext::playNextAudioFrame(ffmpeg::AVPacket* _packet, /*in out*/ bool& _stream_finished, /*in out*/ bool& _eof)
    {
        if (!audioStream_)
        {
            return false;
        }

        openal::alGetSourcei(audioData_.uiSource_, AL_BUFFERS_QUEUED, &audioData_.iQueuedBuffers_);

        if (!audioData_.iQueuedBuffers_)
        {
            audioData_.queueInited_ = false;
        }

        if (audioData_.iQueuedBuffers_ < audio::num_buffers)
        {
            if (!_stream_finished)
            {
                if (!audioQueue_.get(*_packet))
                {
                    return true;
                }
            }
            else
            {

            }

            if (!_packet->data)
            {
                _stream_finished = true;
            }
            else if (_packet->data == (uint8_t*) &flush_pkt)
            {
                flushAudioBuffers();

                openal::alSourceStop(audioData_.uiSource_);

                openal::alSourcei(audioData_.uiSource_, AL_BUFFER, 0);

                return true;
            }

            // The audio packet can contain several frames
            int got_frame = 0;

            int len = avcodec_decode_audio4(audioData_.audioCodecContext_, audioData_.frame_, &got_frame, _packet);

            if (_packet->data)
            {
                ffmpeg::av_packet_unref(_packet);
            }

            if (len < 0)
            {
                return false;
            }

            if (!got_frame)
            {
                if (_stream_finished)
                {
                    _eof = true;

                    return false;
                }
            }
            else
            {
                openal::ALvoid* frameData = 0;
                openal::ALsizei frameDataSize = 0;

                if (audioData_.outSamplesData_) 
                {
                    int64_t delay = ffmpeg::swr_get_delay(audioData_.swrContext_, audioData_.srcRate_);

                    int64_t dstSamples = ffmpeg::av_rescale_rnd(delay + audioData_.frame_->nb_samples, audioData_.dstRate_, audioData_.srcRate_, ffmpeg::AV_ROUND_UP);

                    if (dstSamples > audioData_.maxResampleSamples_) 
                    {
                        audioData_.maxResampleSamples_ = dstSamples;
                        ffmpeg::av_free(audioData_.outSamplesData_[0]);

                        if (ffmpeg::av_samples_alloc(audioData_.outSamplesData_, 0, audio::outChannels, audioData_.maxResampleSamples_, audio::outFormat, 1) < 0) 
                        {
                            audioData_.outSamplesData_[0] = 0;
                            return false;
                        }
                    }
                    int32_t res = 0;
                    if ((res = ffmpeg::swr_convert(audioData_.swrContext_, audioData_.outSamplesData_, dstSamples, (const uint8_t**) audioData_.frame_->extended_data, audioData_.frame_->nb_samples)) < 0) 
                    {
                        return false;
                    }

                    qint32 resultLen = ffmpeg::av_samples_get_buffer_size(0, audio::outChannels, res, audio::outFormat, 1);

                    frameData = audioData_.outSamplesData_[0];
                    frameDataSize = resultLen;
                } 
                else 
                {
                    frameData = audioData_.frame_->extended_data[0];
                    frameDataSize = audioData_.frame_->nb_samples * audioData_.sampleSize_;
                }

                // set volume
                openal::ALfloat volume = (mute_ ? (0.0) : (float(volume_)/100.0));
                openal::alSourcef(audioData_.uiSource_, AL_GAIN, volume);

                if (!audioData_.queueInited_)
                {
                    openal::alBufferData(audioData_.uiBuffers_[audioData_.iQueuedBuffers_], audioData_.fmt_, frameData, frameDataSize, audioData_.freq_);
                    openal::alSourceQueueBuffers(audioData_.uiSource_, 1, &audioData_.uiBuffers_[audioData_.iQueuedBuffers_]);
                }
                else
                {
                    // Copy audio data to Buffer
                    openal::alBufferData(audioData_.uiBuffer_, audioData_.fmt_, frameData, frameDataSize, audioData_.freq_);
                    // Queue Buffer on the Source
                    openal::alSourceQueueBuffers(audioData_.uiSource_, 1, &audioData_.uiBuffer_);
                }

                return true;
            }
        }
        else
        {
            audioData_.queueInited_ = true;
        }

        cleanupAudioBuffers();

        // Check the status of the Source.  If it is not playing, then playback was completed,
        // or the Source was starved of audio data, and needs to be restarted.
        openal::alGetSourcei(audioData_.uiSource_, AL_SOURCE_STATE, &audioData_.iState_);
        if (audioData_.iState_ != AL_PLAYING)
        {
            // If there are Buffers in the Source Queue then the Source was starved of audio
            // data, so needs to be restarted (because there is more audio data to play)
            openal::alGetSourcei(audioData_.uiSource_, AL_BUFFERS_QUEUED, &audioData_.iQueuedBuffers_);
            if (audioData_.iQueuedBuffers_)
            {
                openal::alSourcePlay(audioData_.uiSource_);
            }
            else
            {
                // Finished playing
                return false;
            }
        }

        return true;
    }

    void VideoContext::cleanupAudioBuffers()
    {
        audioData_.buffersProcessed_ = 0;
        openal::alGetSourcei(audioData_.uiSource_, AL_BUFFERS_PROCESSED, &audioData_.buffersProcessed_);

        while (audioData_.buffersProcessed_)
        {
            audioData_.uiBuffer_ = 0;
            openal::alSourceUnqueueBuffers(audioData_.uiSource_, 1, &audioData_.uiBuffer_);

            --audioData_.buffersProcessed_;
        }
    }

    void VideoContext::suspendAudio()
    {
        openal::alSourcePause(audioData_.uiSource_);
    }

    void VideoContext::stopAudio()
    {
        openal::alSourceStop(audioData_.uiSource_);
    }

    void VideoContext::updateScaledVideoSize(const QSize& _sz)
    {
        ThreadMessage msg(thread_message_type::tmt_update_scaled_size);

        msg.x_ = _sz.width();
        msg.y_ = _sz.height();

        postVideoThreadMessage(msg);
    }

    void VideoContext::postVideoThreadMessage(const ThreadMessage& _message)
    {
        videoThreadMessagesQueue_.pushMessage(_message);
    }

    void VideoContext::postDemuxThreadMessage(const ThreadMessage& _message)
    {
        demuxThreadMessageQueue_.pushMessage(_message);
    }

    bool VideoContext::getDemuxThreadMessage(ThreadMessage& _message, int32_t _waitTimeout)
    {
        return demuxThreadMessageQueue_.getMessage(_message, [this]{return isQuit();}, _waitTimeout);
    }

    void VideoContext::postAudioThreadMessage(const ThreadMessage& _message)
    {
        audioThreadMessageQueue_.pushMessage(_message);
    }

    bool VideoContext::getAudioThreadMessage(ThreadMessage& _message, int32_t _waitTimeout)
    {
        return audioThreadMessageQueue_.getMessage(_message, [this]{return isQuit();}, _waitTimeout);
    }

    decode_thread_state VideoContext::getAudioThreadState() const
    {
        return audioData_.state_;
    }

    void VideoContext::setAudioThreadState(const decode_thread_state& _state)
    {
        audioData_.state_ = _state;
    }

    bool VideoContext::getVideoThreadMessage(ThreadMessage& _message, int32_t _waitTimeout)
    {
        return videoThreadMessagesQueue_.getMessage(_message, [this]{return isQuit();}, _waitTimeout);
    }

    bool VideoContext::updateScaleContext(const int32_t _width, const int32_t _height)
    {
        freeScaleContext();

        ffmpeg::AVCodecContext* videoCodecContext = videoStream_->codec;

        int32_t w = (int32_t) (((double) getWidth() / (double) getHeight()) * (double) _height);
        
        if (w > _width)
        {
            w = _width;

            scaledSize_.setHeight(((double) getHeight() / (double) getWidth()) * (double) _width);
        }
        else
        {
            scaledSize_.setHeight(_height);
        }

        scaledSize_.setWidth(w);

        emit videoSizeChanged(scaledSize_);

        swsContext_ = sws_getCachedContext(0, getWidth(), getHeight(), videoCodecContext->pix_fmt, 
            scaledSize_.width(), scaledSize_.height(), ffmpeg::AV_PIX_FMT_RGB24, SWS_FAST_BILINEAR/*SWS_BICUBIC*/, 0, 0, 0);

        if (!swsContext_)
        {
            return false;
        }

        frameRGB_ = ffmpeg::av_frame_alloc();
        int numBytes = ffmpeg::av_image_get_buffer_size(ffmpeg::AV_PIX_FMT_RGB24, scaledSize_.width(), scaledSize_.height(), 1);
        scaledBuffer_.resize(numBytes);
        av_image_fill_arrays(frameRGB_->data, frameRGB_->linesize, &scaledBuffer_[0], ffmpeg::AV_PIX_FMT_RGB24, scaledSize_.width(), scaledSize_.height(), 1);

        return true;
    }

    void VideoContext::freeScaleContext()
    {
        if (frameRGB_)
        {
            ffmpeg::av_frame_unref(frameRGB_);
            ffmpeg::av_frame_free(&frameRGB_);
        }

        frameRGB_ = 0;
    }



    QImage VideoContext::scale(const uint8_t* const _srcSlice[], const int _srcStride[])
    {
        ffmpeg::sws_scale(swsContext_, _srcSlice, _srcStride, 0, getHeight(), frameRGB_->data, frameRGB_->linesize);

        // Convert the frame to QImage
        QImage img(scaledSize_.width(), scaledSize_.height(), QImage::Format_RGB888);

        for (int32_t y = 0; y < scaledSize_.height(); ++y)
        {
            memcpy(img.scanLine(y), frameRGB_->data[0] + y*frameRGB_->linesize[0], scaledSize_.width()*3);
        }

        return img;

        
    }

    bool VideoContext::enableAudio() const
    {
        return !!audioStream_;
    }

    bool VideoContext::enableVideo() const
    {
        return !!videoStream_;
    }

    void VideoContext::setVolume(int32_t _volume)
    {
        volume_ = _volume;
    }

    void VideoContext::setMute(bool _mute)
    {
        mute_ = _mute;
    }

    void VideoContext::resetFrameTimer()
    {
        frameTimer_ = (double) ffmpeg::av_gettime() / 1000000.0;
    }

    bool VideoContext::seekMs(int _tsms)
    {
        int64_t frameNumber = ffmpeg::av_rescale(_tsms, videoStream_->time_base.den, videoStream_->time_base.num);
        frameNumber /= 1000;

        return seekFrame(frameNumber);
    }

    bool VideoContext::seekFrame(int64_t _frame)
    {
        if (ffmpeg::avformat_seek_file(formatContext_, videoStream_->index, INT64_MIN, startTime_ + _frame, INT64_MAX, AVSEEK_FLAG_FRAME) < 0)
        {
            return false;
        }

        videoQueue_.free();
        audioQueue_.free();

        videoQueue_.push(&flush_pkt);
        audioQueue_.push(&flush_pkt);

        return true;
    }

    void VideoContext::flushVideoBuffers()
    {
        if (videoStream_)
        {
            ffmpeg::avcodec_flush_buffers(videoStream_->codec);
        }
    }

    void VideoContext::flushAudioBuffers()
    {
        if (audioStream_)
        {
            ffmpeg::avcodec_flush_buffers(audioStream_->codec);
        }
    }

    void VideoContext::resetVideoClock()
    {
        videoClock_ = 0.0;
    }

    void VideoContext::resetAudioClock()
    {
        audioClock_ = 0.0;
    }

    void VideoContext::setStartTime(const int64_t& _startTime)
    {
        if (startTimeSet_)
            return;

        startTimeSet_ = true;
        startTime_ = _startTime;
    }

    const int64_t& VideoContext::getStartTime() const
    {
        return startTime_;
    }
    //////////////////////////////////////////////////////////////////////////
    // DemuxThread
    //////////////////////////////////////////////////////////////////////////
    DemuxThread::DemuxThread(VideoContext& _ctx)
        :   ctx_(_ctx)
    {

    }

    void DemuxThread::run()
    {
        int32_t waitMsgTimeout = 0;

        int32_t videoStreamIndex = ctx_.getVideoStreamIndex();
        int32_t audioStreamIndex = ctx_.getAudioStreamIndex();

        ffmpeg::AVPacket packet;

        bool init = true;

        decode_thread_state state = decode_thread_state::dts_playing;

        int32_t readPacketError = 0;

        bool eof = false;

        int64_t seekPosition = -1;

        ThreadMessage msg;

        while (!ctx_.isQuit())
        {
            if (ctx_.getDemuxThreadMessage(msg, waitMsgTimeout))
            {
                switch (msg.message_)
                {
                case thread_message_type::tmt_quit:
                    {
                        break;
                    }
                case thread_message_type::tmt_play:
                    {
                        state = decode_thread_state::dts_playing;
                        break;
                    }
                case thread_message_type::tmt_pause:
                    {
                        state = decode_thread_state::dts_paused;
                        break;
                    }
                case thread_message_type::tmt_seek_position:
                    {
                        seekPosition = msg.x_;
                        break;
                    }
                }
            }

            waitMsgTimeout = 0;

            if (state == decode_thread_state::dts_paused)
            {
                ctx_.readAVPacketPause();
            }
            else
            {
                ctx_.readAVPacketPlay();
            }

            if (state == decode_thread_state::dts_paused)
            {
                waitMsgTimeout = 10;
                continue;
            }

            if (seekPosition >= 0)
            {
                ctx_.seekMs(seekPosition);

                seekPosition = -1;

                eof = false;

                if (ctx_.enableAudio())
                    ctx_.pushAudioPacket(&flush_pkt);

                ctx_.pushVideoPacket(&flush_pkt);
            }

            if (ctx_.getAudioQueueSize() + ctx_.getVideoQueueSize() > maxQueueSize
                || ((ctx_.getAudioQueuePackets() > minFramesCount || !ctx_.enableAudio())
                && (ctx_.getVideoQueuePackets() > minFramesCount || !ctx_.enableVideo())))
            {
                waitMsgTimeout = 10;
                continue;
            }

            readPacketError = ctx_.readAVPacket(&packet);

            if (readPacketError < 0)
            {
                if (ctx_.isEof(readPacketError) && !eof) 
                {
                    if (ctx_.enableAudio())
                        ctx_.pushAudioPacket(0);

                    ctx_.pushVideoPacket(0);

                    eof = true;
                }

                if (ctx_.isStreamError())
                    break;

                 waitMsgTimeout = 10;

                continue;
            }
            else
            {
                eof = false;
            }

            if (packet.stream_index == videoStreamIndex)
            {
                ctx_.pushVideoPacket(&packet);
            }
            else if (packet.stream_index == audioStreamIndex)
            {
                ctx_.pushAudioPacket(&packet);
            }
            else
            {
                av_packet_unref(&packet);
            }

            if (init)
            {
                init = false;

                emit ctx_.dataReady();
            }
        }
    }



    //////////////////////////////////////////////////////////////////////////
    // VideoDecodeThread
    //////////////////////////////////////////////////////////////////////////
    VideoDecodeThread::VideoDecodeThread(VideoContext& _ctx)
        :   ctx_(_ctx)
    {

    }

    void VideoDecodeThread::run()
    {
        ffmpeg::AVFrame* frame = ffmpeg::av_frame_alloc();

        QSize scaledSize = ctx_.getScaledSize();

        ctx_.updateScaleContext(scaledSize.width(), scaledSize.height());

        std::unique_ptr<QTransform> imageTransform;

        if (ctx_.getRotation())
        {
            imageTransform.reset(new QTransform());
            imageTransform->rotate(ctx_.getRotation());
        }

        decode_thread_state current_state = decode_thread_state::dts_playing;

        bool eof = false;
        bool stream_finished = false;

        ThreadMessage msg;

        int32_t waitMsgTimeout = 60000;

        ffmpeg::AVPacket av_packet;

        while (!ctx_.isQuit())
        {
            if (ctx_.getVideoThreadMessage(msg, waitMsgTimeout))
            {
                switch (msg.message_)
                {
                    case thread_message_type::tmt_quit:
                    {
                        break;
                    }
                    case thread_message_type::tmt_update_scaled_size:
                    {
                        scaledSize = ctx_.getScaledSize();

                        QSize newSize(msg.x_, msg.y_);

                        if (scaledSize != newSize)
                        {
                            ctx_.updateScaleContext(newSize.width(), newSize.height());
                        }
                        break;
                    }
                    case thread_message_type::tmt_pause:
                    {
                        current_state = dts_paused;

                        break;
                    }
                    case thread_message_type::tmt_play:
                    {
                        current_state = dts_playing;

                        break;
                    }
                    case thread_message_type::tmt_get_next_video_frame:
                    {
                        if (current_state == decode_thread_state::dts_end_of_media)
                        {
                            break;
                        }

                        ffmpeg::av_frame_unref(frame);

                        eof = false;
                        
                        if (ctx_.getNextVideoFrame(frame, &av_packet, stream_finished, eof))
                        {
                            ctx_.setStartTime(frame->pkt_dts);

                            // Consider sync
                            double pts = frame->pkt_dts;

                            if (pts == AV_NOPTS_VALUE)
                            {
                                pts = frame->pkt_pts;
                            }
                            if (pts == AV_NOPTS_VALUE)
                            {
                                pts = 0;
                            }

                            pts *= ctx_.getVideoTimebase();
                            pts = ctx_.synchronizeVideo(frame, pts);

                            if (ctx_.isQuit())
                            {
                                break;
                            }

                            QImage lastFrame = ctx_.scale(frame->data, frame->linesize);

                            if (imageTransform)
                            {
                                lastFrame = lastFrame.transformed(*imageTransform);
                            }

                            emit ctx_.nextframeReady(lastFrame, pts, false);
                        }

                        if (eof)
                        {
                            current_state = decode_thread_state::dts_end_of_media;

                            stream_finished = false;

                            emit ctx_.nextframeReady(QImage(), 0, true);
                        }

                        break;
                    }
                    default:
                        break;
                }
            }
        }

        ctx_.freeScaleContext();

        ffmpeg::av_frame_unref(frame);
        ffmpeg::av_frame_free(&frame);

        return;
    }



    //////////////////////////////////////////////////////////////////////////
    // AudioDecodeThread
    //////////////////////////////////////////////////////////////////////////
    AudioDecodeThread::AudioDecodeThread(VideoContext& _ctx)
        :   ctx_(_ctx)
    {

    }

    void AudioDecodeThread::run()
    {
        bool eof = false;
        bool stream_finished = false;

        ffmpeg::AVPacket packet;
        
        if (ctx_.initDecodeAudioData())
        {
            while (!ctx_.isQuit())
            {
                ThreadMessage msg;

                if (ctx_.getAudioThreadMessage(msg, (ctx_.getAudioThreadState() == decode_thread_state::dts_playing) ? 5 : 60000))
                {
                    switch (msg.message_)
                    {
                        case thread_message_type::tmt_set_volume:
                        {
                            ctx_.setVolume(msg.x_);

                            break;
                        }
                        case thread_message_type::tmt_pause:
                        {
                            ctx_.setAudioThreadState(decode_thread_state::dts_paused);

                            ctx_.suspendAudio();

                            break;
                        }
                        case thread_message_type::tmt_play:
                        {
                            ctx_.setAudioThreadState(decode_thread_state::dts_playing);

                            break;
                        }
                        case thread_message_type::tmt_set_mute:
                        {
                            ctx_.setMute(msg.x_);

                            break;
                        }
                        case thread_message_type::tmt_quit:
                        {
                            break;
                        }
                    }
                }

                if (ctx_.isQuit())
                    break;

                eof = false;

                if (ctx_.getAudioThreadState() == decode_thread_state::dts_playing)
                {
                    if (!ctx_.playNextAudioFrame(&packet, stream_finished, eof))
                    {

                    }

                    if (eof)
                    {
                        ctx_.setAudioThreadState(decode_thread_state::dts_end_of_media);

                        stream_finished = false;
                    }
                }
            }

            ctx_.freeDecodeAudioData();
        }

    }

    static int lockmgr(void **mtx, enum ffmpeg::AVLockOp op)
    {
        switch(op) 
        {
            case ffmpeg::AV_LOCK_CREATE:
            {
                *mtx = new std::mutex();
                return 0;
            }
            case ffmpeg::AV_LOCK_OBTAIN:
            {
                ((std::mutex*) *mtx)->lock();
                return 0;
            }
            case ffmpeg::AV_LOCK_RELEASE:
            {
                ((std::mutex*) *mtx)->unlock();
                return 0;
            }
            case ffmpeg::AV_LOCK_DESTROY:
            {
                delete ((std::mutex*) *mtx);
                return 0;
            }
        }

        return 1;
    }


    //////////////////////////////////////////////////////////////////////////
    // FFMpegPlayer
    //////////////////////////////////////////////////////////////////////////
    FFMpegPlayer::FFMpegPlayer(QWidget* _parent)
        :   QWidget(_parent),
            demuxThread_(ctx_),
            videoDecodeThread_(ctx_),
            audioDecodeThread_(ctx_),
            state_(decode_thread_state::dts_none),
            firstFrame_(true),
            lastVideoPosition_(0)
    {
        ffmpeg::av_init_packet(&flush_pkt);
        flush_pkt.data = (uint8_t*) &flush_pkt;

        ffmpeg::av_register_all();
        ffmpeg::avcodec_register_all();
        ffmpeg::avformat_network_init();
        
        if (ffmpeg::av_lockmgr_register(lockmgr))
        {
        }

        ctx_.init();

        timer_ = new QTimer(this);

        setMouseTracking(true);
    }


    FFMpegPlayer::~FFMpegPlayer()
    {
        stop();
    }

    void FFMpegPlayer::stop()
    {
        ctx_.setQuit();

        ctx_.postDemuxThreadMessage(ThreadMessage(thread_message_type::tmt_quit));
        ctx_.postVideoThreadMessage(ThreadMessage(thread_message_type::tmt_quit));
        ctx_.postAudioThreadMessage(ThreadMessage(thread_message_type::tmt_quit));

        videoDecodeThread_.wait();
        audioDecodeThread_.wait();
        demuxThread_.wait();

        ffmpeg::av_lockmgr_register(NULL);
        ffmpeg::avformat_network_deinit();

        ctx_.closeFile();
    }

    void FFMpegPlayer::paintEvent(QPaintEvent* _e)
    {
        QPainter p(this);

        QSize imageSize = Utils::unscale_bitmap(activeImage_.size());
        QRect clientRect = geometry();

        
        int cx = (clientRect.width() - imageSize.width()) / 2;
        int cy = (clientRect.height() - imageSize.height()) / 2;

        p.drawImage(cx, cy, activeImage_);

        QWidget::paintEvent(_e);
    }

    void FFMpegPlayer::mouseMoveEvent(QMouseEvent * _e)
    {
        return QWidget::mouseMoveEvent(_e);
    }

    void FFMpegPlayer::resizeEvent(QResizeEvent* _e)
    {
        QSize sz = _e->size();

        if (ctx_.getRotation() == 90 || ctx_.getRotation() == 270)
        {
            int32_t tempWidth = sz.width();

            sz.setWidth(sz.height());
            sz.setHeight(tempWidth);
        }

        sz.setWidth(Utils::scale_bitmap(sz.width()));
        sz.setHeight(Utils::scale_bitmap(sz.height()));
        
        ctx_.updateScaledVideoSize(sz);

        QWidget::resizeEvent(_e);
    }

    void FFMpegPlayer::updateVideoPosition(const DecodedFrame& _frame)
    {
        if (_frame.eof_)
        {
            emit positionChanged(ctx_.getDuration());

            lastVideoPosition_ = 0;

            return;
        }

        qint64 videoClock = (_frame.pts_ * 1000);

        {
            lastVideoPosition_ = videoClock;

            emit positionChanged(lastVideoPosition_);
        }
    }

    void FFMpegPlayer::onTimer()
    {
        if (firstFrame_)
        {
            firstFrame_ = false;

            ctx_.resetFrameTimer();
        }

        if (ctx_.isQuit())
        {
            timer_->stop();

            return;
        }

        if (state_ != decode_thread_state::dts_playing)
        {
            return;
        }

        if (decodedFrames_.empty())
        {
            timer_->setInterval((int) 100);

            ctx_.postVideoThreadMessage(ThreadMessage(thread_message_type::tmt_get_next_video_frame));

            return;
        }

        auto& frame = decodedFrames_.front();

        updateVideoPosition(frame);

        if (frame.eof_)
        {
            decodedFrames_.pop_front();

            timer_->stop();

            emit mediaFinished();

            state_ = decode_thread_state::dts_end_of_media;

            return;
        }

        ctx_.postVideoThreadMessage(ThreadMessage(thread_message_type::tmt_get_next_video_frame));

        activeImage_ = frame.image_;
        
        // Sync video to audio
        double delay = ctx_.computeDelay(frame.pts_);

        int timeout = (int)(delay * 1000.0 + 0.5);

        timer_->setInterval(timeout);

        decodedFrames_.pop_front();

        repaint();
    }

    bool FFMpegPlayer::openMedia(const QString& _mediaPath)
    {
        return ctx_.openFile(_mediaPath);
    }

    void FFMpegPlayer::play()
    {
        if (state_ == decode_thread_state::dts_none)
        {
            demuxThread_.start();
            
            connect(&ctx_, &VideoContext::dataReady, this, [this]()
            {
                emit durationChanged(getDuration());

                audioDecodeThread_.start();
                videoDecodeThread_.start();

                connect(timer_, SIGNAL(timeout()), this, SLOT(onTimer()));
                timer_->start(100);


            }, Qt::QueuedConnection);

            connect(&ctx_, &VideoContext::nextframeReady, this, [this](QImage _image, double _pts, bool _eof)
            {
                if (!_eof)
                {
                    Utils::check_pixel_ratio(_image);
                    
                    decodedFrames_.emplace_back(_image, _pts);
                }
                else
                {
                    decodedFrames_.emplace_back(_eof);
                }

            }, Qt::QueuedConnection);

            connect(&ctx_, &VideoContext::videoSizeChanged, this, [this](QSize _sz)
            {
                if (!activeImage_.isNull())
                {
                    activeImage_ = activeImage_.scaled(_sz);

                    for (auto& _frame : decodedFrames_)
                    {
                        _frame.image_ = _frame.image_.scaled(_sz);
                    }
                }

            }, Qt::QueuedConnection);

            ctx_.postVideoThreadMessage(ThreadMessage(thread_message_type::tmt_get_next_video_frame));

            ctx_.resetFrameTimer();
        }
        else if (state_ == decode_thread_state::dts_paused)
        {
            ctx_.postDemuxThreadMessage(ThreadMessage(thread_message_type::tmt_play));
            ctx_.postVideoThreadMessage(ThreadMessage(thread_message_type::tmt_play));
            ctx_.postAudioThreadMessage(ThreadMessage(thread_message_type::tmt_play));

            ctx_.resetFrameTimer();
        }
        else if (state_ == decode_thread_state::dts_end_of_media)
        {
            setPosition(0);

            ctx_.postDemuxThreadMessage(ThreadMessage(thread_message_type::tmt_play));
            ctx_.postVideoThreadMessage(ThreadMessage(thread_message_type::tmt_play));
            ctx_.postAudioThreadMessage(ThreadMessage(thread_message_type::tmt_play));

            state_ = decode_thread_state::dts_playing;

            ctx_.resetFrameTimer();

            timer_->start(0);
        }

        state_ = decode_thread_state::dts_playing;
    }

    void FFMpegPlayer::pause()
    {
        if (state_ == decode_thread_state::dts_playing)
        {
            ctx_.postDemuxThreadMessage(ThreadMessage(thread_message_type::tmt_pause));
            ctx_.postVideoThreadMessage(ThreadMessage(thread_message_type::tmt_pause));
            ctx_.postAudioThreadMessage(ThreadMessage(thread_message_type::tmt_pause));

            state_ = decode_thread_state::dts_paused;
        }
    }

    void FFMpegPlayer::setPosition(int64_t _position)
    {
        ThreadMessage msg(thread_message_type::tmt_seek_position);
        msg.x_ = (int32_t) _position;

        ctx_.postDemuxThreadMessage(msg);
        ctx_.postVideoThreadMessage(msg);
        ctx_.postAudioThreadMessage(msg);
        ctx_.resetFrameTimer();

        lastVideoPosition_ = _position;
    }


    void FFMpegPlayer::setVolume(int32_t _volume)
    {
        ThreadMessage msg(thread_message_type::tmt_set_volume);
        msg.x_ = _volume;

        ctx_.postAudioThreadMessage(msg);
    }

    void FFMpegPlayer::setMute(bool _mute)
    {
        ThreadMessage msg(thread_message_type::tmt_set_mute);
        msg.x_ = _mute;

        ctx_.postAudioThreadMessage(msg);
    }

    QSize FFMpegPlayer::getVideoSize() const
    {
        if (ctx_.getRotation() == 0 || ctx_.getRotation() == 180)
        {
            return QSize(ctx_.getWidth(), ctx_.getHeight());
        }
        else
        {
            return QSize(ctx_.getHeight(), ctx_.getWidth());
        }
    }

    int32_t FFMpegPlayer::getVideoRotation() const
    {
        return ctx_.getRotation();
    }

    int64_t FFMpegPlayer::getDuration() const
    {
        return ctx_.getDuration();
    }
}
