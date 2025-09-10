#include <iostream>
#include <process.h>
#include "LockFreeStack.h"


using namespace std;

#define dfDataNum 4

CLockFreeStack<int> g_Stack;

HANDLE startEvent;
HANDLE popStartEvent;
HANDLE pushEndEvents[5];

unsigned int TestLockFreeStackProc(void* arg)
{
	srand(time(nullptr));

	WaitForSingleObject(startEvent, INFINITE);

	while (1)
	{
		for (int i = 0; i < dfDataNum; ++i)
		{
			int pushValue = rand() % 100;
			g_Stack.Push(pushValue);
		}

		for (int i = 0; i < dfDataNum; ++i)
		{
			int ret = g_Stack.Pop();
		}
	}

	return 0;
}

unsigned int PushProc(void* arg)
{
	srand(time(nullptr));

	HANDLE pushEndEvent = (HANDLE)arg;

	WaitForSingleObject(startEvent, INFINITE);

	for (int i = 0; i < dfDataNum; ++i)
	{
		int pushValue = rand() % 100;
		g_Stack.Push(pushValue);
	}

	SetEvent(pushEndEvent);
	return 0;
}

unsigned int PopProc(void* arg)
{
	srand(time(nullptr));

	WaitForSingleObject(popStartEvent, INFINITE);

	for (int i = 0; i < dfDataNum; ++i)
	{
		int ret = g_Stack.Pop();
	}

	printf("A Thread pop end!\n");
	return 0;
}


int main()
{
	InitLog(dfLOG_LEVEL_DEBUG, ELogMode::FILE_DIRECT);

	HANDLE testTh1 = (HANDLE)_beginthreadex(nullptr, 0, TestLockFreeStackProc, nullptr, 0, nullptr);
	HANDLE testTh2 = (HANDLE)_beginthreadex(nullptr, 0, TestLockFreeStackProc, nullptr, 0, nullptr);
	HANDLE testTh3 = (HANDLE)_beginthreadex(nullptr, 0, TestLockFreeStackProc, nullptr, 0, nullptr);
	HANDLE testTh4 = (HANDLE)_beginthreadex(nullptr, 0, TestLockFreeStackProc, nullptr, 0, nullptr);
	HANDLE testTh5 = (HANDLE)_beginthreadex(nullptr, 0, TestLockFreeStackProc, nullptr, 0, nullptr);
	startEvent = CreateEvent(nullptr, true, false, nullptr);
	SetEvent(startEvent);
	


	/*
	// --- Push pop ºÐ¸® ---
	for (int i = 0; i < 5; i++)
	{
		pushEndEvents[i] = CreateEvent(nullptr, false, false, nullptr);
	}
	HANDLE testTh1 = (HANDLE)_beginthreadex(nullptr, 0, PushProc, pushEndEvents[0], 0, nullptr);
	HANDLE testTh2 = (HANDLE)_beginthreadex(nullptr, 0, PushProc, pushEndEvents[1], 0, nullptr);
	HANDLE testTh3 = (HANDLE)_beginthreadex(nullptr, 0, PushProc, pushEndEvents[2], 0, nullptr);
	HANDLE testTh4 = (HANDLE)_beginthreadex(nullptr, 0, PushProc, pushEndEvents[3], 0, nullptr);
	HANDLE testTh5 = (HANDLE)_beginthreadex(nullptr, 0, PushProc, pushEndEvents[4], 0, nullptr);
	startEvent = CreateEvent(nullptr, true, false, nullptr);
	SetEvent(startEvent);

	WaitForMultipleObjects(5, pushEndEvents, true, INFINITE);
	HANDLE popTh1 = (HANDLE)_beginthreadex(nullptr, 0, PopProc, nullptr, 0, nullptr);
	HANDLE popTh2 = (HANDLE)_beginthreadex(nullptr, 0, PopProc, nullptr, 0, nullptr);
	HANDLE popTh3 = (HANDLE)_beginthreadex(nullptr, 0, PopProc, nullptr, 0, nullptr);
	HANDLE popTh4 = (HANDLE)_beginthreadex(nullptr, 0, PopProc, nullptr, 0, nullptr);
	HANDLE popTh5 = (HANDLE)_beginthreadex(nullptr, 0, PopProc, nullptr, 0, nullptr);
	popStartEvent = CreateEvent(nullptr, true, false, nullptr);
	SetEvent(popStartEvent);
	printf("pop start\n");*/


	while (1)
	{
		//printf("Pop end!\n");
	}

	return 0;
}