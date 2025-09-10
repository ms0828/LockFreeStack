#pragma once
#include <Windows.h>
#include "Log.h"

// ������� ���� �Լ�
// 64��Ʈ ���� 64�ڸ� ���� wide ���ڿ��� ��ȯ (���� 0 ����)
inline void to_bin64(uint64_t v, wchar_t out[65]) 
{
	for (int i = 63; i >= 0; --i)
		out[63 - i] = (v & (1ULL << i)) ? L'1' : L'0';
	out[64] = L'\0';
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
		//nodeSequence = (1 << 17) - 2; // �׽�Ʈ��
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
		// newNode(���� ������ �޸� �ּ�)�� ���� 17��Ʈ�� ����Ͽ� ��� ID �ο�
		//--------------------------------------------------
		ULONGLONG nodeID = InterlockedIncrement(&nodeSequence) % (1 << 17);
		newNode = (Node*)((nodeID << 47) | (ULONGLONG)newNode);


		// ������� ���� �ڵ�
		//wchar_t maskedStr[65], insertStr[65], nextStr[65];
		do
		{
			t = top;
			maskedInsertNode = (Node*)((ULONGLONG)newNode & nodeMask);
			maskedInsertNode->next = t;

			// ������� ���� �ڵ�
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


		// ������� ���� �ڵ�
		//wchar_t maskedStr[65], popStr[65], nextStr[65];
		do
		{
			t = top;
			maskedPopNode = (Node*)((ULONGLONG)t & nodeMask);
			nextTop = maskedPopNode->next;

			// ������� ���� �ڵ�
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
	// ��� ���� ��, ��� ������ ����17��Ʈ�� �����ϴ� ����� ���� �ε���
	//--------------------------------------------
	ULONGLONG nodeSequence;

	//--------------------------------------------
	// Node*�� ���� 47��Ʈ ������ ����ũ
	//--------------------------------------------
	static const ULONGLONG nodeMask = (1ULL << 47) - 1;

};


