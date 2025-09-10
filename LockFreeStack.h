#pragma once
#include <Windows.h>
#include "Log.h"

// 디버깅을 위한 함수
// 64비트 값을 64자리 이진 wide 문자열로 변환 (선행 0 포함)
inline void to_bin64(uint64_t v, wchar_t out[65]) 
{
	for (int i = 63; i >= 0; --i)
		out[63 - i] = (v & (1ULL << i)) ? L'1' : L'0';
	out[64] = L'\0';
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


public:
	CLockFreeStack()
	{
		stackSize = 0;
		//nodeSequence = (1 << 17) - 2; // 테스트용
		nodeSequence = 0;
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
		Node* maskedInsertNode = nullptr;
		newNode->data = data;

		//--------------------------------------------------
		// newNode(유저 영역의 메모리 주소)의 상위 17비트를 사용하여 노드 ID 부여
		//--------------------------------------------------
		ULONGLONG nodeID = InterlockedIncrement(&nodeSequence) % (1 << 17);
		newNode = (Node*)((nodeID << 47) | (ULONGLONG)newNode);


		// 디버깅을 위한 코드
		//wchar_t maskedStr[65], insertStr[65], nextStr[65];
		do
		{
			t = top;
			maskedInsertNode = (Node*)((ULONGLONG)newNode & nodeMask);
			maskedInsertNode->next = t;

			// 디버깅을 위한 코드
			//to_bin64(reinterpret_cast<uint64_t>(maskedInsertNode), maskedStr);
			//to_bin64(reinterpret_cast<uint64_t>(newNode), insertStr);
			//to_bin64(reinterpret_cast<uint64_t>(maskedInsertNode->next), nextStr);

		} while (InterlockedCompareExchangePointer((void* volatile*)&top, newNode, t) != t);
		//_LOG(dfLOG_LEVEL_DEBUG, L" [Push] MaskedNode = %016llx / InsertNodeAddress = %016llx / nextNode = %016llx / data = %d\n", maskedInsertNode, newNode, maskedInsertNode->next, data);
		//_LOG(dfLOG_LEVEL_DEBUG, L" [Push] MaskedNode = %ls \n / InsertNodeAddress = %ls \n / nextNode = %ls \n / data = %d\n", maskedStr, insertStr, nextStr, data);
	}
	

	T Pop()
	{
		Node* t = nullptr;
		Node* nextTop = nullptr;
		Node* maskedPopNode = nullptr;


		// 디버깅을 위한 코드
		//wchar_t maskedStr[65], popStr[65], nextStr[65];
		do
		{
			t = top;
			maskedPopNode = (Node*)((ULONGLONG)t & nodeMask);
			nextTop = maskedPopNode->next;

			// 디버깅을 위한 코드
			//to_bin64(reinterpret_cast<uint64_t>(maskedPopNode), maskedStr);
			//to_bin64(reinterpret_cast<uint64_t>(t), popStr);
			//to_bin64(reinterpret_cast<uint64_t>(nextTop), nextStr);
		}while(InterlockedCompareExchangePointer((void* volatile *)&top, nextTop, t) != t);
		T retData = maskedPopNode->data;
		//_LOG(dfLOG_LEVEL_DEBUG, L" [Pop] [PopNodeAddress = %016llx / nextTop = %016llx / data = %d \n", maskedPopNode, nextTop, retData);
		//_LOG(dfLOG_LEVEL_DEBUG, L" [Pop] MaskedPopNode = %ls \n / PopNodeAddress = %ls \n / nextTop = %ls \n / data = %d \n", maskedStr, popStr, nextStr, retData);

		delete maskedPopNode;
		return retData;
	}

public:
	Node* top;
	ULONGLONG stackSize;

	//--------------------------------------------
	// 노드 생성 시, 노드 포인터 상위17비트에 저장하는 노드의 고유 인덱스
	//--------------------------------------------
	ULONGLONG nodeSequence;

	//--------------------------------------------
	// Node*의 하위 47비트 추출할 마스크
	//--------------------------------------------
	static const ULONGLONG nodeMask = (1ULL << 47) - 1;

};


