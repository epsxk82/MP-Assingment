#include "stdafx.h"
#include "PerformanceTimer.h"

//�������� Ÿ�̸� �⺻ ������
PerformanceTimer::PerformanceTimer() : _Started(false)
{
}

//Ÿ�̸Ӹ� ����
void PerformanceTimer::Start()
{
	QueryPerformanceFrequency(&_Frequency);
	QueryPerformanceCounter(&_StartTime);
	_Started = true;
}

//Ÿ�̸Ӹ� �����ϰ� ��� �ð�(����ũ����)�� ��ȯ
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
