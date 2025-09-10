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
// 테스트 1번
// 1. 모든 스레드가 자신의 TestNum 개수만큼 Push를 수행 후 자신이 Push한 만큼 Pop 수행
//    - 끝나면 위 과정을 반복
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
// 테스트 2번
// 1. 모든 스레드가 동시에 자신의 TestNum 개수만큼 Push을 수행
// 2. 모든 스레드의 Push가 끝나면 동시에 Pop 수행
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
		// 스레드 Push 시작
		//--------------------------------------------------
		for (int i = 0; i < dfTestNum; ++i)
		{
			ULONGLONG pushValue = rand() % 100;
			g_Stack.Push(pushValue);
		}
		_LOG(dfLOG_LEVEL_DEBUG, L"[Check] A Thread Complete Push \n");
		SetEvent(pushEndEvent);



		//---------------------------------------------------
		// 모든 스레드 Push가 끝나기 기다리기
		//---------------------------------------------------
		WaitForMultipleObjects(dfThreadNum, g_PushEndEvents, true, INFINITE);


		//--------------------------------------------------
		// 모든 스레드가 Pop 시작
		//--------------------------------------------------
		for (int i = 0; i < dfTestNum; ++i)
		{
			g_Stack.Pop();
		}
		SetEvent(popEndEvent);
		_LOG(dfLOG_LEVEL_DEBUG, L"[Check] A Thread Complete Pop \n");

		//---------------------------------------------------
		// 모든 스레드 Pop 끝나기 기다리기
		//---------------------------------------------------
		WaitForMultipleObjects(dfThreadNum, g_PopEndEvents, true, INFINITE);


		//---------------------------------------------------
		// StackSize 검증 
		//---------------------------------------------------
		//_LOG(dfLOG_LEVEL_DEBUG, L"[Check] Stack Size = %lld \n", g_Stack.stackSize);


		//---------------------------------------------------
		// 이벤트 초기화
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
	// 테스트 2번
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