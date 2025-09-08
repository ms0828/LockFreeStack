#pragma once
#include <Windows.h>
#include "Log.h"

#define STALL() do { if ((__rdtsc() & 0xFF)==0) { YieldProcessor(); } } while(0)

static ULONG g_LogSequence;

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
		__int64 startTick = 0;
		__int64 commitTick = 0;

		do
		{
			startTick = __rdtsc();
			t = top;
			newNode->next = t;
		} while (InterlockedCompareExchangePointer((void* volatile*)&top, newNode, t) != t);
		commitTick = __rdtsc();
		InterlockedIncrement(&stackSize);
		int logSeq = InterlockedIncrement(&g_LogSequence);
		_LOG(dfLOG_LEVEL_DEBUG, L" [Push] [startTick : %llu ~ commitTick : %llu]  LogSeq : %d / InsertNodeAddress = %016llx / newNode->next = %016llx / data = %d / stackSize = %d\n", startTick, commitTick, logSeq, newNode, newNode->next, data, stackSize);
	}
	
	T Pop()
	{
		Node* t = nullptr;
		Node* nextTop = nullptr;
		__int64 startTick = 0;
		__int64 commitTick = 0;

		do
		{
			//-------------------------------------------------------------
			// �� �����尡 ������ �ݿ��� ���� ������ �ٶ� �ð��� startTick�� ����
			//-------------------------------------------------------------
			startTick = __rdtsc();
			t = top;
			if (t == nullptr)
				_LOG(dfLOG_LEVEL_DEBUG, L"[startTick : %llu] top is null!\n", startTick);
			nextTop = t->next;

			//----------------------------------------------------------
			// ��������
			// - ���� �α׸� ����� ������ �ӵ� ���� �������� ������ �߻����� �ʾҴ�.
			// - �α�� ����ü�� ���� �޸𸮿� �α׸� ���� vs ���� �߻� ���� ������ Sleep(0)���� ���ؽ�Ʈ ����Ī ����
			// - ABA ���� ���� ������ Sleep(0)�� �־����� ���� �α׸� ���ܵ� ������ �� �߻��ߴ�.
			//----------------------------------------------------------
			Sleep(0);
		}while(InterlockedCompareExchangePointer((void* volatile *)&top, nextTop, t) != t);
		
		//-------------------------------------------------
		// �� �����尡 ���ÿ� �ݿ��� �ð��� ����
		// - �� �ð� ���ĺ��ʹ� �ش� ������ ��ȭ�� ���� ����
		//-------------------------------------------------
		commitTick = __rdtsc();

		InterlockedDecrement(&stackSize);
		int logSeq = InterlockedIncrement(&g_LogSequence);
		T retData = t->data;
		_LOG(dfLOG_LEVEL_DEBUG, L" [POP] [startTick : %llu ~ commitTick : %llu]  LogSeq : %d / PopNodeAddress = %016llx / popNode->next = %016llx / data = %d / stackSize = %d\n", startTick, commitTick, logSeq, t, nextTop, retData, stackSize);
		 
		

		//---------------------------------------------------
		// ABA ���� ����
		//---------------------------------------------------
		if (t && t->data == 0xdddddddd)
			_LOG(dfLOG_LEVEL_DEBUG, L"LogSeq : %d / current T is Double Free\n", logSeq);

		delete t;
		return retData;
	}

public:
	Node* top;
	ULONG stackSize;
};


