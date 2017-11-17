#include "stdafx.h"
#include "PerformanceTimer.h"

//성능측정 타이머 기본 생성자
PerformanceTimer::PerformanceTimer() : _Started(false)
{
}

//타이머를 시작
void PerformanceTimer::Start()
{
	QueryPerformanceFrequency(&_Frequency);
	QueryPerformanceCounter(&_StartTime);
	_Started = true;
}

//타이머를 종료하고 경과 시간(마이크로초)을 반환
__int64 PerformanceTimer::Stop()
{
	if(_Started == true)
	{
		QueryPerformanceCounter(&_EndTime);
		return (_EndTime.QuadPart - _StartTime.QuadPart) / (_Frequency.QuadPart / 1000000);
	}
	else
	{
		return 0;
	}
}
