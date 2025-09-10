#include <iostream>
#include <process.h>
#include "LockFreeStack.h"

using namespace std;

#define dfTestNum 1000000
#define dfThreadNum 4

CLockFreeStack<ULONGLONG> g_Stack;

HANDLE g_TestStartEvent;
HANDLE g_PushEndEvents[dfThreadNum];
HANDLE g_PopEndEvents[dfThreadNum];
HANDLE g_ResetEvents[dfThreadNum];

struct ThreadArg
{
	HANDLE pushEndEvent;
	HANDLE popEndEvent;
	HANDLE resetEvent;
};

//-------------------------------------------------------------
// �׽�Ʈ 1��
// 1. ��� �����尡 �ڽ��� TestNum ������ŭ Push�� ���� �� �ڽ��� Push�� ��ŭ Pop ����
//    - ������ �� ������ �ݺ�
//--------------------------------------------------------------
unsigned int PushAndPopProc1(void* arg)
{
	srand(time(nullptr));

	WaitForSingleObject(g_TestStartEvent, INFINITE);

	while (1)
	{
		for (int i = 0; i < dfTestNum; ++i)
		{
			ULONGLONG pushValue = rand() % 100;
			g_Stack.Push(pushValue);
		}

		for (int i = 0; i < dfTestNum; ++i)
		{
			g_Stack.Pop();
		}
	}

	return 0;
}


//-------------------------------------------------------------
// �׽�Ʈ 2��
// 1. ��� �����尡 ���ÿ� �ڽ��� TestNum ������ŭ Push�� ����
// 2. ��� �������� Push�� ������ ���ÿ� Pop ����
//--------------------------------------------------------------
unsigned int PushAndPopProc2(void* arg)
{
	ThreadArg* threadArg = (ThreadArg*)arg;
	HANDLE pushEndEvent = threadArg->pushEndEvent;
	HANDLE popEndEvent = threadArg->popEndEvent;
	HANDLE resetEvent = threadArg->resetEvent;
	WaitForSingleObject(g_TestStartEvent, INFINITE);


	while (1)
	{
		//--------------------------------------------------
		// ������ Push ����
		//--------------------------------------------------
		for (int i = 0; i < dfTestNum; ++i)
		{
			ULONGLONG pushValue = rand() % 100;
			g_Stack.Push(pushValue);
		}
		_LOG(dfLOG_LEVEL_DEBUG, L"[Check] A Thread Complete Push \n");
		SetEvent(pushEndEvent);



		//---------------------------------------------------
		// ��� ������ Push�� ������ ��ٸ���
		//---------------------------------------------------
		WaitForMultipleObjects(dfThreadNum, g_PushEndEvents, true, INFINITE);


		//--------------------------------------------------
		// ��� �����尡 Pop ����
		//--------------------------------------------------
		for (int i = 0; i < dfTestNum; ++i)
		{
			g_Stack.Pop();
		}
		SetEvent(popEndEvent);
		_LOG(dfLOG_LEVEL_DEBUG, L"[Check] A Thread Complete Pop \n");

		//---------------------------------------------------
		// ��� ������ Pop ������ ��ٸ���
		//---------------------------------------------------
		WaitForMultipleObjects(dfThreadNum, g_PopEndEvents, true, INFINITE);


		//---------------------------------------------------
		// StackSize ���� 
		//---------------------------------------------------
		//_LOG(dfLOG_LEVEL_DEBUG, L"[Check] Stack Size = %lld \n", g_Stack.stackSize);


		//---------------------------------------------------
		// �̺�Ʈ �ʱ�ȭ
		//---------------------------------------------------
		ResetEvent(pushEndEvent);
		ResetEvent(popEndEvent);
		//SetEvent(resetEvent);
		//WaitForMultipleObjects(5, g_ResetEvents, true, INFINITE);
		//ResetEvent(resetEvent);
	}
	


	return 0;
}

void Test1()
{
	g_TestStartEvent = CreateEvent(nullptr, true, false, nullptr);
	for (int i = 0; i < dfThreadNum; i++)
	{
		HANDLE testTh = (HANDLE)_beginthreadex(nullptr, 0, PushAndPopProc1, nullptr, 0, nullptr);
	}
	SetEvent(g_TestStartEvent);

	printf("Test Start! \n");

	while (1)
	{

	}
}
void Test2()
{
	g_TestStartEvent = CreateEvent(nullptr, true, false, nullptr);
	ThreadArg threadArg[dfThreadNum];
	for (int i = 0; i < dfThreadNum; i++)
	{
		g_PushEndEvents[i] = CreateEvent(nullptr, true, false, nullptr);
		g_PopEndEvents[i] = CreateEvent(nullptr, true, false, nullptr);
		g_ResetEvents[i] = CreateEvent(nullptr, true, false, nullptr);
		threadArg[i].pushEndEvent = g_PushEndEvents[i];
		threadArg[i].popEndEvent = g_PopEndEvents[i];
		threadArg[i].resetEvent = g_ResetEvents[i];
	}

	//---------------------------------------------------
	// �׽�Ʈ 2��
	//---------------------------------------------------
	for (int i = 0; i < dfThreadNum; i++)
	{
		HANDLE testTh = (HANDLE)_beginthreadex(nullptr, 0, PushAndPopProc2, (void*)&threadArg[i], 0, nullptr);
	}
	SetEvent(g_TestStartEvent);
	printf("Test Start! \n");


	while (1)
	{

	}
}


int main()
{
	InitLog(dfLOG_LEVEL_DEBUG, ELogMode::CONSOLE);

	Test1();

	return 0;
}