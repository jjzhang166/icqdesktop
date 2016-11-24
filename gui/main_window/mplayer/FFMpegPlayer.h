#pragma once

#include "ffmpeg.h"

namespace Ui
{
    namespace audio
    {
        const int32_t num_buffers = 4;
        const int32_t outFrequency = 48000;
        const int32_t blockSize = 4096;
        const ffmpeg::AVSampleFormat outFormat = ffmpeg::AV_SAMPLE_FMT_S16;
        const int64_t outChannelLayout = AV_CH_LAYOUT_STEREO;
        const qint32 outChannels = 2;
    }

    enum decode_thread_state
    {
        dts_none = 0,
        dts_playing = 1,
        dts_paused = 2,
        dts_end_of_media = 3,
        dts_seeking = 4,
        dts_failed = 5
    };

    //////////////////////////////////////////////////////////////////////////
    // ThreadMessage
    //////////////////////////////////////////////////////////////////////////
    enum thread_message_type
    {
        tmt_unknown = 0,
        tmt_update_scaled_size = 1,
        tmt_quit = 3,
        tmt_set_volume = 4,
        tmt_set_mute = 5,
        tmt_pause = 6,
        tmt_play = 7,
        tmt_get_next_video_frame = 8,
        tmt_seek_position = 9,
        tmt_stream_seeked = 10
    };

    struct ThreadMessage
    {
        thread_message_type message_;

        int32_t x_;
        int32_t y_;

        ThreadMessage(const thread_message_type _message = thread_message_type::tmt_unknown)
            :   message_(_message),
                x_(0),
                y_(0)
        {
        }
    };


    //////////////////////////////////////////////////////////////////////////
    // ThreadMessagesQueue
    //////////////////////////////////////////////////////////////////////////
    class ThreadMessagesQueue
    {
        std::mutex queue_mutex_;

        std::condition_variable condition_;

        std::list<ThreadMessage> messages_;

    public:

        bool getMessage(ThreadMessage& _message, std::function<bool()> _isQuit, int32_t _wait_timeout);
        void pushMessage(const ThreadMessage& _message);
    };


    //////////////////////////////////////////////////////////////////////////
    // PacketQueue
    //////////////////////////////////////////////////////////////////////////
    class PacketQueue
    {
        QMutex mutex_;

        std::list<ffmpeg::AVPacket> list_;

        std::atomic<int32_t> packets_;
        std::atomic<int32_t> size_;

    public:

        PacketQueue();
        ~PacketQueue();

        void free();

        void push(ffmpeg::AVPacket* _packet);
        bool get(ffmpeg::AVPacket& _packet);

        int32_t getSize() const;
        int32_t getPackets() const;
    };


    //////////////////////////////////////////////////////////////////////////
    // DecodeAudioData
    //////////////////////////////////////////////////////////////////////////
    struct DecodeAudioData
    {
        ffmpeg::AVFrame* frame_;

        uint8_t** outSamplesData_;

        ffmpeg::SwrContext* swrContext_;

        openal::ALuint uiBuffers_[audio::num_buffers];
        openal::ALuint uiSource_;
        openal::ALuint uiBuffer_;
        openal::ALint buffersProcessed_;
        openal::ALint iState_;
        openal::ALint iQueuedBuffers_;

        int iloop_;

        uint64_t layout_;
        int32_t channels_;
        int32_t fmt_;
        int32_t sampleSize_;
        int32_t freq_;
        int32_t maxResampleSamples_;
        int32_t srcRate_;
        int32_t dstRate_;
        //ffmpeg::AVPacket packet_;

        bool queueInited_;
                
        ffmpeg::AVCodecContext* audioCodecContext_;
        decode_thread_state state_;

        DecodeAudioData()
            :   frame_(0),
                outSamplesData_(0),
                swrContext_(0),
                uiSource_(0),
                uiBuffer_(0),
                buffersProcessed_(0),
                iState_(0),
                iQueuedBuffers_(0),
                layout_(0),
                channels_(0),
                fmt_(AL_FORMAT_STEREO16),
                sampleSize_(2 * sizeof(uint16_t)),
                freq_(0),
                maxResampleSamples_(1024),
                srcRate_(audio::outFrequency),
                dstRate_(audio::outFrequency),
                audioCodecContext_(0),
                state_(decode_thread_state::dts_playing),
                queueInited_(false),
                iloop_(0)
        {

        }
    };

    //////////////////////////////////////////////////////////////////////////
    // VideoContext
    //////////////////////////////////////////////////////////////////////////
    class VideoContext : public QObject
    {
        Q_OBJECT

    public:

    Q_SIGNALS:

        void dataReady();
        void nextframeReady(QImage _frame, double _pts, bool _eof);
        void seeked();
        void videoSizeChanged(QSize _sz);

    private:

        bool quit_;
        
        ffmpeg::AVFormatContext* formatContext_;

        ffmpeg::AVStream* videoStream_;
        ffmpeg::AVStream* audioStream_;

        PacketQueue videoQueue_;
        PacketQueue audioQueue_;

        ffmpeg::SwsContext* swsContext_;

        ffmpeg::AVStream* openStream(int32_t _type);
        void closeStream(ffmpeg::AVStream* _stream);

        int32_t width_;
        int32_t height_;
        int32_t rotation_;
        int64_t duration_;
        int64_t seek_position_;

        QSize scaledSize_;
        
        double frameTimer_;

        bool startTimeVideoSet_;
        bool startTimeAudioSet_;
        int64_t startTimeVideo_;
        int64_t startTimeAudio_;

        double videoClock_;
        double audioClock_;

        double frameLastPts_;

        double frameLastDelay_;

        bool mute_;
        int32_t volume_;

        ffmpeg::AVFrame* frameRGB_;
        std::vector<uint8_t> scaledBuffer_;

        DecodeAudioData audioData_;
        
        ThreadMessagesQueue videoThreadMessagesQueue_;
        ThreadMessagesQueue demuxThreadMessageQueue_;
        ThreadMessagesQueue audioThreadMessageQueue_;

        void writeRingBuffer(void* _data, int32_t _len, int32_t _block);


    public:

        VideoContext();

        void init();

        int32_t getVideoStreamIndex() const;
        int32_t getAudioStreamIndex() const;

        int32_t readAVPacket(/*OUT*/ffmpeg::AVPacket* _packet);
        int32_t readAVPacketPause();
        int32_t readAVPacketPlay();
        bool isEof(int32_t _error);
        bool isStreamError();

        bool getNextVideoFrame(/*OUT*/ffmpeg::AVFrame* _frame, ffmpeg::AVPacket* _packet, bool& _stream_finished, bool& _eof);

        void pushVideoPacket(ffmpeg::AVPacket* _packet);
        int32_t getVideoQueuePackets() const;
        int32_t getVideoQueueSize() const;
        void pushAudioPacket(ffmpeg::AVPacket* _packet);
        int32_t getAudioQueuePackets() const;
        int32_t getAudioQueueSize() const;

        bool isQuit() const;
        void setQuit(bool _val = true);

        bool openFile(const QString& _file);
        void closeFile();

        
        int32_t getWidth() const;
        int32_t getHeight() const;
        int32_t getRotation() const;
        int64_t getDuration() const;

        QSize getScaledSize() const;
        bool enableAudio() const;
        bool enableVideo() const;
        
        QImage scale(const uint8_t* const _srcSlice[], const int _srcStride[], int _height);

        double getVideoTimebase();
        double synchronizeVideo(ffmpeg::AVFrame* _frame, double _pts);
        double computeDelay(double _picturePts);

        bool initDecodeAudioData();
        void freeDecodeAudioData();
        bool readFrameAudio(ffmpeg::AVPacket* _packet, bool& _stream_finished, bool& _eof, openal::ALvoid** _frameData, openal::ALsizei& _frameDataSize);
        bool playNextAudioFrame(ffmpeg::AVPacket* _packet, /*in out*/ bool& _stream_finished, /*in out*/ bool& _eof);
        void cleanupAudioBuffers();
        void suspendAudio();
        void stopAudio();
        
        void updateScaledVideoSize(const QSize& _sz);

        void postVideoThreadMessage(const ThreadMessage& _message);
        bool getVideoThreadMessage(ThreadMessage& _message, int32_t _waitTimeout);

        void postDemuxThreadMessage(const ThreadMessage& _message);
        bool getDemuxThreadMessage(ThreadMessage& _message, int32_t _waitTimeout);

        void postAudioThreadMessage(const ThreadMessage& _message);
        bool getAudioThreadMessage(ThreadMessage& _message, int32_t _waitTimeout);

        decode_thread_state getAudioThreadState() const;
        void setAudioThreadState(const decode_thread_state& _state);

        bool updateScaleContext(const int32_t _width, const int32_t _height);
        void freeScaleContext();

        void setVolume(int32_t _volume);
        void setMute(bool _mute);

        void resetFrameTimer();
        bool seekMs(int _tsms);
        bool seekFrame(int64_t _ts_video, int64_t _ts_audio);

        void flushVideoBuffers();
        void flushAudioBuffers();

        void resetVideoClock();
        void resetAudioClock();

        void setStartTimeVideo(const int64_t& _startTime);
        const int64_t& getStartTimeVideo() const;
        void setStartTimeAudio(const int64_t& _startTime);
        const int64_t& getStartTimeAudio() const;
    };


    //////////////////////////////////////////////////////////////////////////
    // DemuxThread
    //////////////////////////////////////////////////////////////////////////
    class DemuxThread : public QThread
    {
        VideoContext& ctx_;

    protected:

        virtual void run() override;

    public:

        DemuxThread(VideoContext& _ctx);
    };


    //////////////////////////////////////////////////////////////////////////
    // VideoDecodeThread
    //////////////////////////////////////////////////////////////////////////
    class VideoDecodeThread : public QThread
    {

        VideoContext& ctx_;

    protected:

        virtual void run() override;

    public:

        VideoDecodeThread(VideoContext& _ctx);
    };



    //////////////////////////////////////////////////////////////////////////
    // AudioDecodeThread
    //////////////////////////////////////////////////////////////////////////
    class AudioDecodeThread : public QThread
    {
        VideoContext& ctx_;

    protected:

        virtual void run() override;

    public:

        AudioDecodeThread(VideoContext& _ctx);
    };

    class FrameRenderer
    {
        QImage activeImage_;

    protected:

        void renderFrame(QPainter& _painter, const QRect& _clientRect);

    public:

        void updateFrame(QImage _image);

        bool isActiveImageNull() const;

        virtual QWidget* getWidget() = 0;

        virtual void redraw() = 0;

        virtual void filterEvents(QWidget* _parent) = 0;
    };

    class GDIRenderer : public QWidget, public FrameRenderer
    {
        virtual QWidget* getWidget() override;

        virtual void redraw() override;

        virtual void paintEvent(QPaintEvent* _e) override;

        virtual void filterEvents(QWidget* _parent) override;

    public:

        GDIRenderer(QWidget* _parent);
    };

    class OpenGLRenderer : 
#ifdef __linux__
public QWidget, public FrameRenderer
#else
public QOpenGLWidget, public FrameRenderer
#endif //__linux__
    {
        virtual QWidget* getWidget()
#ifndef __linux__
 override
#endif __linux__
;
        virtual void redraw()
#ifndef __linux__
 override
#endif __linux__
;

        virtual void paintEvent(QPaintEvent* _e) override;

        virtual void filterEvents(QWidget* _parent)
#ifndef __linux__
 override
#endif __linux__
;

    public:
        OpenGLRenderer(QWidget* _parent);

    };



    //////////////////////////////////////////////////////////////////////////
    // FFMpegPlayer
    //////////////////////////////////////////////////////////////////////////
    class FFMpegPlayer : public QWidget
    {
        Q_OBJECT

        VideoContext ctx_;

        DemuxThread demuxThread_;

        VideoDecodeThread videoDecodeThread_;
        AudioDecodeThread audioDecodeThread_;

        QTimer* timer_;

        FrameRenderer* renderer_;

        bool isFirstFrame_;

        struct DecodedFrame
        {
            QImage image_;

            double pts_;

            bool eof_;

            DecodedFrame(const QImage& _image, const double _pts) : image_(_image), pts_(_pts), eof_(false) {}
            DecodedFrame(const bool& _eof) : eof_(_eof) {}
        };

        std::unique_ptr<DecodedFrame> firstFrame_;

        std::list<DecodedFrame> decodedFrames_;

        double computeDelay();

        decode_thread_state state_;

        qint64 lastVideoPosition_;
        
        void makeObject();

        std::chrono::system_clock::time_point lastEmitMouseMove_;

    private:

        void updateVideoPosition(const DecodedFrame& _frame);
        bool canPause() const;

    public:

        Q_SIGNALS:

        void durationChanged(qint64 _duration);
        void positionChanged(qint64 _position);
        void mediaFinished();
        void mouseMoved();
        void mouseLeaveEvent();

        private Q_SLOTS:

        void onTimer();

        void stop();

    public:

        FFMpegPlayer(QWidget* _parent);
        virtual ~FFMpegPlayer();

        bool openMedia(const QString& _mediaPath);

        void play();
        void pause();

        void setPosition(int64_t _position);

        void setVolume(int32_t _volume);
        void setMute(bool _mute);

        QSize getVideoSize() const;
        int32_t getVideoRotation() const;
        int64_t getDuration() const;

    protected:

        virtual bool eventFilter(QObject* _obj, QEvent* _event) override;
    };

}
