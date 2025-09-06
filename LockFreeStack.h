#pragma once
#include <Windows.h>
#include "Log.h"


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
		} while (InterlockedCompareExchangePointer((void* volatile*)&top, newNode, t) != t);
		//InterlockedIncrement(&stackSize);
		//_LOG(dfLOG_LEVEL_DEBUG, L"Push : insertNodeAddress = %016llx / data = %d \n", newNode, data);
		SpinLock();
		//printf("Push : insertNodeAddress = %016llx / data = %d \n", newNode, data);
		SpinUnlock();
	}
	
	T Pop()
	{
		Node* t = nullptr;
		Node* nextTop = nullptr;
		do
		{
			t = top;
			nextTop = t->next;
			// t�� nullptr �� ��Ȳ�� ������ t->next�� null�̾���, ABA�������� �Ʒ� ���Ͷ��� �������Դ�.
		}while(InterlockedCompareExchangePointer((void* volatile *)&top, nextTop, t) != t);
		//InterlockedDecrement(&stackSize);
		//_LOG(dfLOG_LEVEL_DEBUG, L"POP : popNodeAddress = %016llx / popNode->next = %016llx\n", t, nextTop);
		SpinLock();
		//printf("POP : popNodeAddress = %016llx / popNode->next = %016llx\n", t, nextTop);
		SpinUnlock();


		T retData = t->data;
		delete t;
		return retData;
	}

public:
	Node* top;

	ULONG stackSize;
};


