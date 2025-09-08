#include <iostream>
#include <process.h>
#include "LockFreeStack.h"


using namespace std;

#define dfDataNum 50000

CLockFreeStack<int> g_Stack;

HANDLE startEvent;

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


int main()
{
	InitLog(dfLOG_LEVEL_DEBUG, ELogMode::FILE_DIRECT);

	HANDLE testTh1 = (HANDLE)_beginthreadex(nullptr, 0, TestLockFreeStackProc, nullptr, 0, nullptr);
	HANDLE testTh2 = (HANDLE)_beginthreadex(nullptr, 0, TestLockFreeStackProc, nullptr, 0, nullptr);
	HANDLE testTh3 = (HANDLE)_beginthreadex(nullptr, 0, TestLockFreeStackProc, nullptr, 0, nullptr);
	HANDLE testTh4 = (HANDLE)_beginthreadex(nullptr, 0, TestLockFreeStackProc, nullptr, 0, nullptr);
	HANDLE testTh5 = (HANDLE)_beginthreadex(nullptr, 0, TestLockFreeStackProc, nullptr, 0, nullptr);
	/*HANDLE testTh6 = (HANDLE)_beginthreadex(nullptr, 0, TestLockFreeStackProc, nullptr, 0, nullptr);
	HANDLE testTh7 = (HANDLE)_beginthreadex(nullptr, 0, TestLockFreeStackProc, nullptr, 0, nullptr);
	HANDLE testTh8 = (HANDLE)_beginthreadex(nullptr, 0, TestLockFreeStackProc, nullptr, 0, nullptr);
	HANDLE testTh9 = (HANDLE)_beginthreadex(nullptr, 0, TestLockFreeStackProc, nullptr, 0, nullptr);
	HANDLE testTh10 = (HANDLE)_beginthreadex(nullptr, 0, TestLockFreeStackProc, nullptr, 0, nullptr);*/


	startEvent = CreateEvent(nullptr, true, false, nullptr);
	SetEvent(startEvent);

	//CLockFreeStack<int>::Node* testNode = new CLockFreeStack<int>::Node;
	//CLockFreeStack<int>::Node* testNode2 = new CLockFreeStack<int>::Node;
	//testNode->data = 5;
	//testNode->next = nullptr;
	//
	//CLockFreeStack<int>::Node* top;
	//CLockFreeStack<int>::Node* t;
	//top = testNode;
	//t = top;


	////delete testNode;
	//testNode = testNode2;
	//InterlockedCompareExchangePointer((void* volatile*)&top, testNode, top);
	//int a = 0;

	while (1)
	{

	}

	return 0;
}