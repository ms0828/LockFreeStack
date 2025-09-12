#pragma once
#include <Windows.h>
#include "ObjectPool.h"
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
		Node* newNode = nodePool.allocObject();
		
		//-----------------------------------
		// ABA ���� �׽�Ʈ
		// - ���ÿ� Push�ϸ� ��� Ǯ Pop�Ͽ� 1���� �ʱ�ȭ ��, ���ÿ� ����
		// - ���ÿ��� Pop�� �� 0�� �ʱ�ȭ �� ��ȯ(��� Ǯ�� Push)
		// - ��� Ǯ���� Pop�ߴµ� 0�� �ƴϸ� ��� Ǯ�� �ִ� ���� ������ �����Ͽ� ����� ��
		//-----------------------------------
		if (newNode->data == 1)
		{
			_LOG(dfLOG_LEVEL_DEBUG, L"[Error] newNode->data == 1 \n");
			exit(1);
		}
		newNode->data = data;

		//--------------------------------------------------
		// newNode(���� ������ �޸� �ּ�)�� ���� 17��Ʈ�� ����Ͽ� ��� ID �ο�
		//--------------------------------------------------
		ULONGLONG nodeID = InterlockedIncrement(&nodeSequence) % (1 << 17);
		newNode = (Node*)((nodeID << 47) | (ULONGLONG)newNode);


		//wchar_t maskedStr[65], insertStr[65], nextStr[65];
		do
		{
			t = top;
			Node* maskedInsertNode = (Node*)((ULONGLONG)newNode & nodeMask);
			maskedInsertNode->next = t;

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

		//wchar_t maskedStr[65], popStr[65], nextStr[65];
		do
		{
			t = top;
			maskedPopNode = (Node*)((ULONGLONG)t & nodeMask);
			nextTop = maskedPopNode->next;

			//to_bin64(reinterpret_cast<uint64_t>(maskedPopNode), maskedStr);
			//to_bin64(reinterpret_cast<uint64_t>(t), popStr);
			//to_bin64(reinterpret_cast<uint64_t>(nextTop), nextStr);
		}while(InterlockedCompareExchangePointer((void* volatile *)&top, nextTop, t) != t);
		T retData = maskedPopNode->data;
		//_LOG(dfLOG_LEVEL_DEBUG, L" [Pop] [PopNodeAddress = %016llx / nextTop = %016llx / data = %d \n", maskedPopNode, nextTop, retData);
		//_LOG(dfLOG_LEVEL_DEBUG, L" [Pop] MaskedPopNode = %ls \n / PopNodeAddress = %ls \n / nextTop = %ls \n / data = %d \n", maskedStr, popStr, nextStr, retData);


		//--------------------------------------------------------
		// ABA ���� �׽�Ʈ
		// - ��� Ǯ�� ��� ��ȯ ��, ������ 0���� �ʱ�ȭ
		// - �ٵ� pop�� ��� �����Ͱ� 0�̶�� top�� ��� Ǯ�� ��ȯ�� ��带 ����Ű�� �ִ� ��
		//--------------------------------------------------------
		if (retData != 1)
		{
			_LOG(dfLOG_LEVEL_DEBUG, L"[Error] retData == %d\n", retData);
			exit(1);
		}
		else
			_LOG(dfLOG_LEVEL_DEBUG, L"[Check] retData == %d\n", retData);
		maskedPopNode->data = 0; // ������

		nodePool.freeObject(maskedPopNode);
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


	CObjectPool<CLockFreeStack::Node> nodePool;
};


