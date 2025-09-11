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
// �׽�Ʈ 2��
// 1. ��� �����尡 ���ÿ� �ڽ��� TestNum ������ŭ Push�� ����
// 2. ��� �������� Push�� ������ ���ÿ� Pop ����
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
		// ������ Push ����
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
		// ��� ������ Push�� ������ ��ٸ���
		//---------------------------------------------------
		WaitForMultipleObjects(dfThreadNum, g_PushEndEvents, true, INFINITE);
		
		// ��� �����尡 ������ ����, ������ ������ �����ϴ� ������ StartEvent ����
		ResetEvent(g_TestStartEvent);

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
		// �� ���� ������ ��� Ǯ ī��Ʈ ���� 
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
		//---------------------------------------------------
		// ��� �����尡 Cycle�� �Ϸ��ϸ� �̺�Ʈ �ʱ�ȭ �� ��� ������ ����Ŭ ����� �̺�Ʈ �ߵ�
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