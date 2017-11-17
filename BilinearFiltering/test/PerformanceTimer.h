#pragma once

#include <Windows.h>

//성능 측정 타이머
class PerformanceTimer
{
public :
	PerformanceTimer();
	void Start();
	__int64 Stop();

private:
	//성능 측정 타이머에 사용될 인자값
	LARGE_INTEGER _StartTime;
	LARGE_INTEGER _EndTime;
	LARGE_INTEGER _Frequency;
	bool _Started;
};

