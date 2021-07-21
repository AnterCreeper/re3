#pragma once

#ifdef AUDIO_OAL
#include <AL/al.h>

#define NUM_STREAMBUFFERS 8

class IDecoder
{
public:
	virtual ~IDecoder() { }
	
	virtual bool   IsOpened() = 0;
	virtual void   FileOpen() = 0;
	
	virtual uint32 GetSampleSize() = 0;
	virtual uint32 GetSampleCount() = 0;
	virtual uint32 GetSampleRate() = 0;
	virtual uint32 GetChannels() = 0;
	
	uint32 GetAvgSamplesPerSec()
	{
		return GetChannels() * GetSampleRate();
	}
	
	uint32 ms2samples(uint32 ms)
	{
		return float(ms) / 1000.0f * float(GetSampleRate());
	}
	
	uint32 samples2ms(uint32 sm)
	{
		return float(sm) * 1000.0f / float(GetSampleRate());
	}
	
	uint32 GetBufferSamples()
	{
		//return (GetAvgSamplesPerSec() >> 2) - (GetSampleCount() % GetChannels());
		return (GetAvgSamplesPerSec() / 4); // 250ms
	}
	
	uint32 GetBufferSize()
	{
		return GetBufferSamples() * GetSampleSize();
	}
	
	virtual void   Seek(uint32 milliseconds) = 0;
	virtual uint32 Tell() = 0;
	
	uint32 GetLength()
	{
		FileOpen(); // abort deferred init, we need length now - game has to cache audio file sizes
		return float(GetSampleCount()) * 1000.0f / float(GetSampleRate());
	}
	
	virtual uint32 Decode(void *buffer) = 0;
};
#ifdef MULTITHREADED_AUDIO
template <typename T> class tsQueue
{
public:
	tsQueue() : count(0) {
		pthread_mutex_init(&m_mutex, NULL);
	}
	
	void push(const T &value)
	{
		pthread_mutex_lock(&m_mutex);
		m_queue.push(value);
		count++;
		pthread_mutex_unlock(&m_mutex);
	}

	bool peekPop(T *retVal)
	{
		pthread_mutex_lock(&m_mutex);
		if (count == 0) {
			pthread_mutex_unlock(&m_mutex);
			return false;
		}

		*retVal = m_queue.front();
		m_queue.pop();
		count--;
		pthread_mutex_unlock(&m_mutex);
		return true;
	}

	void swapNts(tsQueue<T> &replaceWith)
	{
		m_queue.swap(replaceWith.m_queue);
		replaceWith.count = count;
	}

	/*
	void swapTs(tsQueue<T> &replaceWith)
	{
		pthread_mutex_lock(&m_mutex);
		pthread_mutex_lock(&replaceWith.m_mutex);
		swapNts(replaceWith);
		pthread_mutex_unlock(&replaceWith.m_mutex);
		pthread_mutex_unlock(&m_mutex);
	}
	*/

	bool emptyNts()
	{
		return count == 0;
	}

	/*
	bool emptyTs()
	{
		pthread_mutex_lock(&m_mutex);
		bool isEmpty = emptyNts();
		pthread_mutex_unlock(&m_mutex);
		return isEmpty;
	}
	*/
	
	std::queue<T> m_queue;
	int count;
	pthread_mutex_t m_mutex;
};
#endif
class CStream
{
	char     m_aFilename[128];
	ALuint  *m_pAlSources;
	ALuint (&m_alBuffers)[NUM_STREAMBUFFERS];
	
	bool     m_bPaused;
	bool     m_bActive;
	
public:
#ifdef MULTITHREADED_AUDIO
	pthread_mutex_t m_mutex;
	std::queue<std::pair<ALuint, ALuint>> m_fillBuffers; // left and right buffer
	tsQueue<std::pair<ALuint, ALuint>> m_queueBuffers;
	bool     m_bDoSeek;
	uint32   m_SeekPos;
	bool	 m_bIExist;
#endif

	void    *m_pBuffer;
	
	bool     m_bReset;
	uint32   m_nVolume;
	uint8    m_nPan;
	uint32   m_nPosBeforeReset;
	int32   m_nLoopCount;
	
	IDecoder *m_pSoundFile;

	void BuffersShouldBeFilled(); // all
	bool BufferShouldBeFilledAndQueued(std::pair<ALuint, ALuint>*); // two (left-right)
#ifdef MULTITHREADED_AUDIO
	void FlagAsToBeProcessed(bool close = false);
	bool QueueBuffers();
#endif

	bool HasSource();
	void SetPosition(int i, float x, float y, float z);
	void SetPitch(float pitch);
	void SetGain(float gain);
	void   Pause();
	void   SetPlay(bool state);
	
	bool   FillBuffer(ALuint *alBuffer);
	int32	FillBuffers();
	void   ClearBuffers();
//public:
	static void Initialise();
	static void Terminate();
	
	CStream(ALuint *sources, ALuint (&buffers)[NUM_STREAMBUFFERS]);
	~CStream();
	void   Delete();
	bool   Open(const char *filename, uint32 overrideSampleRate = 32000);
	void   Close();
	
	bool   IsOpened();
	bool   IsPlaying();
	void   SetPause (bool bPause);
	void   SetVolume(uint32 nVol);
	void   SetPan   (uint8 nPan);
	void   SetPosMS (uint32 nPos); 
	uint32 GetPosMS();
	uint32 GetLengthMS();
	
	bool Setup(bool imSureQueueIsEmpty = false, bool lock = true);
	void Start();
	void Stop();
	void Update(void);
	void SetLoopCount(int32);
	
	void ProviderInit();
	void ProviderTerm();
};

#endif
