#pragma once
#include <Windows.h>
#include "ObjectPool.h"
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
			next = nullptr;
		};

	public:
		T data;
		Node* next;
	};


public:
	CLockFreeStack() : nodePool(false)
	{
		stackSize = 0;
		top = nullptr;
	}

	~CLockFreeStack()
	{
		Node* curNode = UnpackingNode(top);
		while (curNode != nullptr)
		{
			Node* deleteNode = curNode;
			curNode = curNode->next;
			delete deleteNode;
		}
	}
	
	void Push(T& data)
	{
		
		Node* newNode = nodePool.allocObject();
		
		//-----------------------------------
		// ABA 검출 테스트
		// - 스택에 Push하면 노드 풀 Pop하여 1으로 초기화 후, 스택에 삽입
		// - 스택에서 Pop할 때 0로 초기화 후 반환(노드 풀에 Push)
		// - 노드 풀에서 AllocObject(노드 풀 POP)했는데 0이 아니면 노드 풀에 있는 것을 누군가 참조하여 사용한 것
		//-----------------------------------
		if (newNode->data == 1)
		{
			_LOG(dfLOG_LEVEL_DEBUG, L"[Error] newNode->data == 1 \n");
			exit(1);
		}
		newNode->data = data;


		Node* t = nullptr;
		Node* nextTop;

		wchar_t beforeTop[65], afterTop[65];
		wchar_t beforeMaskedTop[65], afterMaskedTop[65];
		do
		{
			t = top;
			Node* maskedT = UnpackingNode(t);
			newNode->next = maskedT;
			nextTop = PackingNode(newNode, GetNodeStamp(t) + 1);
			to_bin64(reinterpret_cast<uint64_t>(t), beforeTop);
			to_bin64(reinterpret_cast<uint64_t>(maskedT), beforeMaskedTop);
			to_bin64(reinterpret_cast<uint64_t>(nextTop), afterTop);
			to_bin64(reinterpret_cast<uint64_t>(newNode), afterMaskedTop);
		} while (InterlockedCompareExchangePointer((void* volatile*)&top, nextTop, t) != t);
		InterlockedIncrement(&stackSize);
		_LOG(dfLOG_LEVEL_DEBUG, L"[Push]\n - beforeTop : %ls\n -    masked : %ls \n - afterTop : %ls\n -    masked : %ls) \n", beforeTop, beforeMaskedTop, afterTop, afterMaskedTop);
		//_LOG(dfLOG_LEVEL_DEBUG, L" [Push] MaskedNode = %ls \n / InsertNodeAddress = %ls \n / nextNode = %ls \n / data = %d\n", maskedStr, insertStr, nextStr, data);
	}
	

	boolean Pop(T& value)
	{
		Node* t;
		Node* nextTop;
		Node* maskedT;

		wchar_t beforeTop[65], afterTop[65];
		wchar_t beforeMaskedTop[65], afterMaskedTop[65];
		do
		{
			t = top;
			if (t == nullptr)
				return false;

			maskedT = UnpackingNode(t);
			nextTop = PackingNode(maskedT->next, GetNodeStamp(t) + 1);

			to_bin64(reinterpret_cast<uint64_t>(t), beforeTop);
			to_bin64(reinterpret_cast<uint64_t>(maskedT), beforeMaskedTop);
			to_bin64(reinterpret_cast<uint64_t>(nextTop), afterTop);
			to_bin64(reinterpret_cast<uint64_t>(maskedT->next), afterMaskedTop);
		}while(InterlockedCompareExchangePointer((void* volatile *)&top, nextTop, t) != t);
		InterlockedDecrement(&stackSize);
		
		value = maskedT->data;
		_LOG(dfLOG_LEVEL_DEBUG, L"[POP]\n - beforeTop : %ls\n -    masked : %ls) \n - afterTop : %ls\n -    masked : %ls) \n", beforeTop, beforeMaskedTop, afterTop, afterMaskedTop);
		//_LOG(dfLOG_LEVEL_DEBUG, L" [Pop] MaskedPopNode = %ls \n / PopNodeAddress = %ls \n / nextTop = %ls \n / data = %d \n", maskedStr, popStr, nextStr, retData);

		maskedT->data = 0; // 디버깅용
		nodePool.freeObject(maskedT);
		return true;
	}


private:
	inline Node* PackingNode(Node* ptr, ULONGLONG stamp)
	{
		return (Node*)((ULONGLONG)ptr | (stamp << stampShift));
	}
	inline Node* UnpackingNode(Node* ptr)
	{
		return (Node*)((ULONGLONG)ptr & nodeMask);
	}
	inline ULONGLONG GetNodeStamp(Node* ptr)
	{
		return (ULONGLONG)ptr >> stampShift;
	}


public:
	Node* top;
	ULONGLONG stackSize;

	//--------------------------------------------
	// Node*의 하위 47비트 추출할 마스크
	//--------------------------------------------
	static const ULONGLONG nodeMask = (1ULL << 47) - 1;
	static const ULONG stampShift = 47;


	CObjectPool<CLockFreeStack::Node> nodePool;
};


