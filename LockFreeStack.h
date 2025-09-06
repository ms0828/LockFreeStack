#pragma once
#include <Windows.h>
#include "Log.h"

#define STALL() do { if ((__rdtsc() & 0xFF)==0) { YieldProcessor(); } } while(0)

//--------------------------------------------
// 테스트를 위한 스핀락
//--------------------------------------------
ULONG lock = 0;
void SpinLock()
{
	while (InterlockedExchange(&lock, 1) == 1)
	{
		YieldProcessor();
	}
}

void SpinUnlock()
{
	InterlockedExchange(&lock, 0);
}




//--------------------------------------------
// 락 프리 스택 구현체
//--------------------------------------------
template<typename T>
class CLockFreeStack
{
public:
	class Node
	{
	public:
		Node()
		{
			data = -1;
			next = nullptr;
		};

	public:
		T data;
		Node* next;
	};



	struct st_LogData
	{
		Node* nodePtr;
		Node* nextNodePtr;
		int data;
	};


public:
	CLockFreeStack()
	{
		stackSize = 0;
		top = nullptr;
	}

	~CLockFreeStack()
	{
		while (top)
		{
			Node* deleteNode = top;
			top = top->next;
			delete deleteNode;
		}
	}

	void Push(T& data)
	{
		Node* t = nullptr;
		Node* newNode = new Node;
		newNode->data = data;
		do
		{
			t = top;
			newNode->next = t;
			STALL();
		} while (InterlockedCompareExchangePointer((void* volatile*)&top, newNode, t) != t);
		InterlockedIncrement(&stackSize);

		/*st_LogData logData;
		logData.nodePtr = newNode;
		logData.nextNodePtr = t;
		logData.data = data;
		gt_LogQ.Enqueue((char*)&logData, sizeof(st_LogData));*/

		//_LOG(dfLOG_LEVEL_DEBUG, L"Push : InsertNodeAddress = %016llx / data = %d \n", newNode, data);
		//SpinLock();
		printf("Push : insertNodeAddress = %016llx / data = %d \n", newNode, data);
		//SpinUnlock();
	}
	
	T Pop()
	{
		Node* t = nullptr;
		Node* nextTop = nullptr;
		do
		{
			t = top;
			nextTop = t->next;
			STALL();
			// t가 nullptr 인 상황은 이전에 t->next가 null이었고, ABA문제에서 아래 인터락을 빠져나왔다.
		}while(InterlockedCompareExchangePointer((void* volatile *)&top, nextTop, t) != t);
		
		T retData = t->data;
		InterlockedDecrement(&stackSize);

		/*st_LogData logData;
		logData.nodePtr = t;
		logData.nextNodePtr = nextTop;
		logData.data = retData;
		gt_LogQ.Enqueue((char*)&logData, sizeof(st_LogData));*/

		//_LOG(dfLOG_LEVEL_DEBUG, L"POP : PopNodeAddress = %016llx / popNode->next = %016llx / data = %d\n", t, nextTop, retData);
		//SpinLock();
		printf("POP : popNodeAddress = %016llx / popNode->next = %016llx\n", t, nextTop);
		//SpinUnlock();


		
		delete t;
		return retData;
	}

public:
	Node* top;

	ULONG stackSize;
};


