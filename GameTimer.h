// �ð� ������ Ŭ����
// QueryPerformanceCounter�� ��� ���� ���μ��� ��� �� 
// � ���μ������� ����Ǵ����� ���� ����� �޶��� �� ����.
// SetThreadAffinityMask�Լ��� ���� ���� ���α׷��� �� �����尡 
// �ٸ� ���μ����� ��ȯ�Ǵ� ���� ��������.

#pragma once

#include <Windows.h>

class GameTimer
{
public:

	GameTimer();

	float TotalTime()const;	// �� ����
	float DeltaTime()const;	// �� ����

	void Reset();			// �޼������� ������ ȣ��
	void Start();			// Ÿ�̸� ���� �Ǵ� �簳
	void Stop();			// Ÿ�̸� ����
	void Tick();			// �� �����Ӹ��� ȣ��

private:

	double mSecondsPerCount;
	double mDeltaTime;

	__int64 mBaseTime;		// Ÿ�̸Ӱ� ���۵� �ð�
	__int64 mPausedTime;	// �Ͻ������� �ð�
	__int64 mStopTime;		// �Ͻ������� ���۵� �ð�
	__int64 mPrevTime;		
	__int64 mCurrTime;		

	bool mStopped;
};

