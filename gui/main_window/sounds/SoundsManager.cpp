#include "stdafx.h"
#include "SoundsManager.h"

#include "../../gui_settings.h"
#include "../contact_list/ContactListModel.h"

#include "MpegLoader.h"

const int IncomingMessageInterval = 3000;
const int PttCheckInterval = 500;

namespace Ui
{
    void PlayingData::init()
    {
        alGenSources(1, &Source_);
        alSourcef(Source_, AL_PITCH, 1.f);
        alSourcef(Source_, AL_GAIN, 1.f);
        alSource3f(Source_, AL_POSITION, 0, 0, 0);
        alSource3f(Source_, AL_VELOCITY, 0, 0, 0);
        alSourcei(Source_, AL_LOOPING, 0);
        alGenBuffers(1, &Buffer_);
    }

    void PlayingData::setBuffer(const QByteArray& data, qint64 freq, qint64 fmt)
    {
        alBufferData(Buffer_, fmt, data.constData(), data.size(), freq);
        alSourcei(Source_, AL_BUFFER, Buffer_);
    }

    void PlayingData::play()
    {
        if (isEmpty())
            return;

        alSourcePlay(Source_);
    }

    void PlayingData::pause()
    {
        if (isEmpty())
            return;

        alSourcePause(Source_);
    }

    void PlayingData::stop()
    {
        if (isEmpty())
            return;

        alSourceStop(Source_);
        if (alIsBuffer(Buffer_)) 
        {
            alSourcei(Source_, AL_BUFFER, 0);
            alDeleteBuffers(1, &Buffer_);
        }
    }

    void PlayingData::clear()
    {
        Buffer_ = 0;
        Source_ = 0;
        Id_ = -1;
    }
    
    void PlayingData::free()
    {
        if (!isEmpty())
        {
            stop();
            alDeleteSources(1, &Source_);
            clear();
        }
    }

    bool PlayingData::isEmpty() const
    {
        return Id_ == -1;
    }

    ALenum PlayingData::state() const
    {
        ALenum state = AL_NONE;
        if (!isEmpty())
        {
            alGetSourcei(Source_, AL_SOURCE_STATE, &state);
        }
        return state;
    }

	SoundsManager::SoundsManager()
		: QObject(0)
		, CallInProgress_(false)
		, CanPlayIncoming_(true)
		, Timer_(new QTimer(this))
        , PttTimer_(new QTimer(this))
        , AlId(-1)
        , AlAudioDevice_(0)
        , AlAudioContext_(0)
        , AlInited_(false)
	{
        initIncomig();
        initOutgoing();
		Timer_->setInterval(IncomingMessageInterval);
		Timer_->setSingleShot(true);
		connect(Timer_, SIGNAL(timeout()), this, SLOT(timedOut()), Qt::QueuedConnection);

        PttTimer_->setInterval(PttCheckInterval);
        PttTimer_->setSingleShot(true);
        connect(PttTimer_, SIGNAL(timeout()), this, SLOT(checkPttState()), Qt::QueuedConnection);

        connect(Logic::GetContactListModel(), SIGNAL(selectedContactChanged(QString)), this, SLOT(contactChanged(QString)), Qt::QueuedConnection);
	}

	SoundsManager::~SoundsManager()
	{
        CurPlay_.free();
        PrevPlay_.free();
        Incoming_.free();
        Outgoing_.free();
        if (AlInited_)
            shutdownOpenAl();
	}

	void SoundsManager::timedOut()
	{
		CanPlayIncoming_ = true;
	}

    void SoundsManager::checkPttState()
    {
        if (CurPlay_.Source_ != 0)
        {
            ALenum state;
            alGetSourcei(CurPlay_.Source_, AL_SOURCE_STATE, &state);
            if (state == AL_PLAYING || state == AL_INITIAL)
            {
                PttTimer_->start();
            }
            else if (state == AL_PAUSED)
            {
                emit pttPaused(CurPlay_.Id_);
            }
            else if (state == AL_STOPPED)
            {
                emit pttFinished(CurPlay_.Id_);
                if (!PrevPlay_.isEmpty())
                {
                    CurPlay_.stop();
                    CurPlay_.clear();
                    CurPlay_ = PrevPlay_;
                    PrevPlay_.clear();
                }
            }
        }
    }

    void SoundsManager::contactChanged(QString)
    {
        if (CurPlay_.state() == AL_PLAYING)
        {
            CurPlay_.stop();
            emit pttPaused(CurPlay_.Id_);
        }
    }

	void SoundsManager::playIncomingMessage()
	{
        if (CurPlay_.state() == AL_PLAYING)
            return;

		if (get_gui_settings()->get_value<bool>(settings_sounds_enabled, true) && CanPlayIncoming_ && !CallInProgress_)
		{
			CanPlayIncoming_ = false;
            Incoming_.play();
			Timer_->start();
		}
	}

	void SoundsManager::playOutgoingMessage()
	{
        if (CurPlay_.state() == AL_PLAYING)
            return;

		if (get_gui_settings()->get_value(settings_outgoing_message_sound_enabled, false) && !CallInProgress_)
        {
            Outgoing_.play();
        }
	}

    int SoundsManager::playPtt(const QString& file, int id)
    {
        if (!AlInited_)
            initOpenAl();

        if (!CurPlay_.isEmpty())
        {
            if (!PrevPlay_.isEmpty())
            {
                if (PrevPlay_.state() == AL_PAUSED && PrevPlay_.Id_ == id)
                {
                    if (CurPlay_.state() == AL_PLAYING)
                    {
                        CurPlay_.pause();
                        emit pttPaused(CurPlay_.Id_);
                    }

                    PlayingData exchange;
                    exchange = PrevPlay_;
                    PrevPlay_ = CurPlay_;
                    CurPlay_ = exchange;

                    CurPlay_.play();
                    PttTimer_->start();
                    return CurPlay_.Id_;
                }
            }
            if (CurPlay_.state() == AL_PLAYING)
            {
                if (!PrevPlay_.isEmpty())
                {
                    PrevPlay_.stop();
                    emit pttFinished(PrevPlay_.Id_);
                    PrevPlay_.clear();
                }
                
                CurPlay_.pause();
                emit pttPaused(CurPlay_.Id_);
                PrevPlay_ = CurPlay_;
            }
            else if (CurPlay_.state() == AL_PAUSED)
            {
                if (CurPlay_.Id_ == id)
                {
                    CurPlay_.play();
                    PttTimer_->start();
                    return CurPlay_.Id_;
                }
                
                if (!PrevPlay_.isEmpty())
                {
                    PrevPlay_.stop();
                    emit pttFinished(PrevPlay_.Id_);
                    PrevPlay_.clear();
                }

                PrevPlay_ = CurPlay_;
            }
            CurPlay_.clear();
        }

        CurPlay_.init();

        MpegLoader l(file, false);
        l.open();

        QByteArray result;
        qint64 samplesAdded = 0, frequency = l.frequency(), format = l.format();
        while (1) 
        {
            int res = l.readMore(result, samplesAdded);
            if (res < 0)
                break;
        }     

        CurPlay_.setBuffer(result, frequency, format);

        int err;
        if ((err = alGetError()) == AL_NO_ERROR)
        {
            CurPlay_.Id_ = ++AlId;
            CurPlay_.play();
            PttTimer_->start();
            return CurPlay_.Id_;
        }

        return -1;
    }

    void SoundsManager::pausePtt(int id)
    {
        if (CurPlay_.Id_ == id)
            CurPlay_.pause();
    }

	void SoundsManager::callInProgress(bool value)
	{
		CallInProgress_ = value;
	}

    void SoundsManager::reinit()
    {
        if (AlInited_)
            shutdownOpenAl();
        initIncomig();
        initOutgoing();
    }

    void SoundsManager::initOpenAl()
    {
        AlAudioDevice_ = alcOpenDevice(NULL);
        AlAudioContext_ = alcCreateContext(AlAudioDevice_, NULL);
        alcMakeContextCurrent(AlAudioContext_);

        ALfloat v[] = { 0.f, 0.f, -1.f, 0.f, 1.f, 0.f };
        alListener3f(AL_POSITION, 0.f, 0.f, 0.f);
        alListener3f(AL_VELOCITY, 0.f, 0.f, 0.f);
        alListenerfv(AL_ORIENTATION, v);

        alDistanceModel(AL_NONE);

        if (alGetError() == AL_NO_ERROR)
            AlInited_ = true;
    }
    
    void SoundsManager::initIncomig()
    {
        if (!AlInited_)
            initOpenAl();

        if (!Incoming_.isEmpty())
            return;
        
        Incoming_.init();
        
        MpegLoader l(":/sounds/incoming", true);
        l.open();
        
        QByteArray result;
        qint64 samplesAdded = 0, frequency = l.frequency(), format = l.format();
        while (1)
        {
            int res = l.readMore(result, samplesAdded);
            if (res < 0)
                break;
        }
        
        Incoming_.setBuffer(result, frequency, format);
        
        int err;
        if ((err = alGetError()) == AL_NO_ERROR)
        {
            Incoming_.Id_ = ++AlId;
        }
    }
    
    void SoundsManager::initOutgoing()
    {
        if (!AlInited_)
            initOpenAl();

        if (!Outgoing_.isEmpty())
            return;
        
        Outgoing_.init();
        
        MpegLoader l(":/sounds/outgoing", true);
        l.open();
        
        QByteArray result;
        qint64 samplesAdded = 0, frequency = l.frequency(), format = l.format();
        while (1)
        {
            int res = l.readMore(result, samplesAdded);
            if (res < 0)
                break;
        }
        
        Outgoing_.setBuffer(result, frequency, format);
        
        int err;
        if ((err = alGetError()) == AL_NO_ERROR)
        {
            Outgoing_.Id_ = ++AlId;
        }
    }

    void SoundsManager::shutdownOpenAl()
    {
        emit pttPaused(CurPlay_.Id_);
        emit pttFinished(CurPlay_.Id_);
        emit pttPaused(PrevPlay_.Id_);
        emit pttFinished(PrevPlay_.Id_);

        PrevPlay_.stop();
        PrevPlay_.free();
        PrevPlay_.clear();

        CurPlay_.stop();
        CurPlay_.free();
        CurPlay_.clear();

        Incoming_.stop();
        Incoming_.free();
        Incoming_.clear();

        Outgoing_.stop();
        Outgoing_.free();
        Outgoing_.clear();

        if (AlAudioContext_)
        {
            alcMakeContextCurrent(NULL);
            alcDestroyContext(AlAudioContext_);
            AlAudioContext_ = 0;
        }

        if (AlAudioDevice_)
        {
            alcCloseDevice(AlAudioDevice_);
            AlAudioDevice_ = 0;
        }

        AlInited_ = false;
    }

	SoundsManager* GetSoundsManager()
	{
		static std::unique_ptr<SoundsManager> manager(new SoundsManager());
		return manager.get();
	}
}
