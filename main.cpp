#include <iostream>
#include <process.h>
#include "LockFreeStack.h"

using namespace std;

#define dfTestNum 3
#define dfThreadNum 4


struct TestST
{
	char ch[4096];
};

CLockFreeStack<int> g_Stack;

HANDLE g_TestStartEvent;
HANDLE g_PushEndEvents[dfThreadNum];
HANDLE g_PopEndEvents[dfThreadNum];
HANDLE g_CycleEndEvents[dfThreadNum];

struct ThreadArg
{
	HANDLE pushEndEvent;
	HANDLE popEndEvent;
	HANDLE cycleEndEvent;
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
			int a = rand() % 100;
			//TestST a{ 0, };
			g_Stack.Push(a);
		}

		for (int i = 0; i < dfTestNum; ++i)
		{
			int ret = g_Stack.Pop();
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
	HANDLE cycleEndEvent = threadArg->cycleEndEvent;
	
	while (1)
	{
		WaitForSingleObject(g_TestStartEvent, INFINITE);

		//--------------------------------------------------
		// 스레드 Push 시작
		//--------------------------------------------------
		for (int i = 0; i < dfTestNum; ++i)
		{
			int a = rand() % 100;
			//TestST a{ 0, };
			g_Stack.Push(a);
		}
		_LOG(dfLOG_LEVEL_DEBUG, L"[Check] A Thread Complete Push \n");
		SetEvent(pushEndEvent);



		//---------------------------------------------------
		// 모든 스레드 Push가 끝나기 기다리기
		//---------------------------------------------------
		WaitForMultipleObjects(dfThreadNum, g_PushEndEvents, true, INFINITE);
		
		// 모든 스레드가 시작을 보장, 끝나기 이전을 보장하는 구간에 StartEvent 리셋
		ResetEvent(g_TestStartEvent);

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
		// 락 프리 스택의 노드 풀 카운트 검증 
		//---------------------------------------------------
		if (g_Stack.nodePool.GetPoolCnt() != dfTestNum * dfThreadNum)
		{
			_LOG(dfLOG_LEVEL_DEBUG, L"[Error] Node Pool Count = %ld \n", g_Stack.nodePool.GetPoolCnt());
			exit(1);
		}

		SetEvent(cycleEndEvent);
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
		g_CycleEndEvents[i] = CreateEvent(nullptr, true, false, nullptr);
		threadArg[i].pushEndEvent = g_PushEndEvents[i];
		threadArg[i].popEndEvent = g_PopEndEvents[i];
		threadArg[i].cycleEndEvent = g_CycleEndEvents[i];
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
		//---------------------------------------------------
		// 모든 스레드가 Cycle을 완료하면 이벤트 초기화 및 모든 스레드 싸이클 재실행 이벤트 발동
		//---------------------------------------------------
		WaitForMultipleObjects(dfThreadNum, g_CycleEndEvents, true, INFINITE);
		for (int i = 0; i < dfThreadNum; ++i)
		{
			ResetEvent(g_PushEndEvents[i]);
			ResetEvent(g_PopEndEvents[i]);
			ResetEvent(g_CycleEndEvents[i]);
		}
		SetEvent(g_TestStartEvent);
	}
}


int main()
{
	InitLog(dfLOG_LEVEL_DEBUG, ELogMode::CONSOLE);

	Test1();

	return 0;
}