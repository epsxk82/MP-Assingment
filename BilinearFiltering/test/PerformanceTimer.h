#pragma once

#include <Windows.h>

//���� ���� Ÿ�̸�
class PerformanceTimer
{
public :
	PerformanceTimer();
	void Start();
	__int64 Stop();

private:
	//���� ���� Ÿ�̸ӿ� ���� ���ڰ�
	LARGE_INTEGER _StartTime;
	LARGE_INTEGER _EndTime;
	LARGE_INTEGER _Frequency;
	bool _Started;
};

