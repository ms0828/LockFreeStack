#pragma once
#include <Windows.h>
#include "Log.h"

#define STALL() do { if ((__rdtsc() & 0xFF)==0) { YieldProcessor(); } } while(0)

static ULONG g_LogSequence;

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
			// 현 스레드가 스택을 반영할 때의 스택을 바라본 시각을 startTick에 저장
			//-------------------------------------------------------------
			startTick = __rdtsc();
			t = top;
			if (t == nullptr)
				_LOG(dfLOG_LEVEL_DEBUG, L"[startTick : %llu] top is null!\n", startTick);
			nextTop = t->next;

			//----------------------------------------------------------
			// 시행착오
			// - 파일 로그를 남기려 했으나 속도 저하 때문인지 문제가 발생하지 않았다.
			// - 로깅용 구조체를 만들어서 메모리에 로그를 저장 vs 문제 발생 예상 구간에 Sleep(0)으로 컨텍스트 스위칭 유발
			// - ABA 문제 예상 구간에 Sleep(0)을 넣었더니 파일 로그를 남겨도 문제가 잘 발생했다.
			//----------------------------------------------------------
			Sleep(0);
		}while(InterlockedCompareExchangePointer((void* volatile *)&top, nextTop, t) != t);
		
		//-------------------------------------------------
		// 현 스레드가 스택에 반영한 시각을 저장
		// - 이 시각 이후부터는 해당 스택이 변화된 것을 보장
		//-------------------------------------------------
		commitTick = __rdtsc();

		InterlockedDecrement(&stackSize);
		int logSeq = InterlockedIncrement(&g_LogSequence);
		T retData = t->data;
		_LOG(dfLOG_LEVEL_DEBUG, L" [POP] [startTick : %llu ~ commitTick : %llu]  LogSeq : %d / PopNodeAddress = %016llx / popNode->next = %016llx / data = %d / stackSize = %d\n", startTick, commitTick, logSeq, t, nextTop, retData, stackSize);
		 
		

		//---------------------------------------------------
		// ABA 문제 검출
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


