#pragma once
#include <Windows.h>
#include "Log.h"

#define STALL() do { if ((__rdtsc() & 0xFF)==0) { YieldProcessor(); } } while(0)

//--------------------------------------------
// �׽�Ʈ�� ���� ���ɶ�
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
// �� ���� ���� ����ü
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
			// t�� nullptr �� ��Ȳ�� ������ t->next�� null�̾���, ABA�������� �Ʒ� ���Ͷ��� �������Դ�.
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


