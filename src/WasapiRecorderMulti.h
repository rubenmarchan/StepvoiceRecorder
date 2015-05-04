#ifndef WASAPI_RECORDER_MULTI_H
#define WASAPI_RECORDER_MULTI_H
#pragma once

#include "Filter.h"
#include "WasapiHelpers.h" //for a DevicesArray
#include "SyncObjects.h"

class CWasapiRecorderStream;

/////////////////////////////////////////////////////////////////////////////

class CWasapiRecorderMulti : public Filter
{
public:
	CWasapiRecorderMulti(WasapiHelpers::DevicesArray devices, DWORD freq, DWORD chans);
	virtual ~CWasapiRecorderMulti();

	DWORD GetActualFrequency() const;
	DWORD GetActualChannelCount() const;

	BOOL Start();
	BOOL Pause();
	BOOL Stop();
	BOOL IsStarted() const;
	BOOL IsPaused() const;
	BOOL IsStopped() const;

	float GetVolume() const;
	BOOL  SetVolume(float volume); //0..1

	float GetPeakLevel(int channel) const; //0 = first channel, -1 = all channels
	DWORD GetData(void* buffer, DWORD lengthBytes) const; //returns -1 if error

	//For compatibility with GraphWnd callback
	DWORD GetChannelData(int channel, float* buffer, int bufferSize);

private:
	std::vector<CWasapiRecorderStream*> m_recorderStreams;
	DWORD m_actualFreq;
	DWORD m_actualChans;

	typedef DWORD HSTREAM; //Sample stream handle (from bass.h).
	HSTREAM m_mixerStream;

	static DWORD WINAPI ReadDataFromStreamProc(LPVOID lpParam);
	HANDLE m_streamEvent;
	HANDLE m_streamThread;
	bool m_exitThread;

	CMyCriticalSection m_sync_object;

	enum RecorderState { eStopped, ePaused, eStarted} m_recorderState;
};

/////////////////////////////////////////////////////////////////////////////

#endif // WASAPI_RECORDER_MULTI_H
