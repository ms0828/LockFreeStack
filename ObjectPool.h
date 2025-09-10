#pragma once
#include <iostream>
#include <Windows.h>
#include "Log.h"

template<typename T>
class CObjectPool
{
public:
	struct Node
	{
	public:
		T instance;
		USHORT seed;
		Node* next;
	};

public:

	//------------------------------------------------------------
	// ������Ʈ ��������Ʈ
	//------------------------------------------------------------
	CObjectPool(bool preConstructor)
	{
		poolSeed = rand();
		bPreConstructor = preConstructor;
		top = nullptr;
		poolCnt = 0;
	}

	//------------------------------------------------------------
	// ������Ʈ Ǯ
	// - ��Ƽ ������ ȯ�濡���� �� ������ ȣ���� ������ ����� ��
	//------------------------------------------------------------
	CObjectPool(bool preConstructor, int poolNum)
	{
		poolSeed = rand();
		bPreConstructor = preConstructor;
		top = nullptr;
		for (int i = 0; i < poolNum; i++)
		{
			Node* newNode = (Node*)malloc(sizeof(Node));
			newNode->seed = poolSeed;
			newNode->next = top;
			top = newNode;

			// bPreConstructor�� true�� ��쿡�� ������ ȣ��
			if (bPreConstructor)
			{
				T* instance = (T*)newNode;
				new (instance) T();
			}
		}
		poolCnt = poolNum;
	}

	~CObjectPool()
	{
		Node* curNode = top;
		while (curNode != nullptr)
		{
			Node* deleteNode = curNode;
			curNode = curNode->next;
			if (bPreConstructor)
				delete deleteNode;
			else
				free(deleteNode);
		}
	}

	T* allocObject()
	{
		Node* t = nullptr;
		Node* nextTop = nullptr;
		Node* maskedAllocNode = nullptr;
		do
		{
			t = top;

			//----------------------------------------
			// Ǯ�� ������� �� ������Ʈ�� ���� �����Ͽ� �Ҵ�
			//----------------------------------------
			if (t == nullptr)
			{
				Node* newNode = (Node*)malloc(sizeof(Node));
				newNode->seed = poolSeed;
				new (newNode) T();
				return &(newNode->instance);
			}

			maskedAllocNode = (Node*)((ULONGLONG)t & nodeMask);
			nextTop = maskedAllocNode->next;
		} while (InterlockedCompareExchangePointer((void* volatile*)&top, nextTop, t) != t);
		InterlockedDecrement(&poolCnt);

		//----------------------------------------
		// bPreConstructor�� ���� �ִ� ��� �Ҵ縶�� �����ڰ� ȣ��
		//----------------------------------------
		if (!bPreConstructor)
			new (maskedAllocNode) T();
		
		return &(maskedAllocNode->instance);
	}

	bool freeObject(T* objectPtr)
	{
		Node* freeNode = (Node*)objectPtr;
		if (freeNode->seed != poolSeed)
		{
			_LOG(dfLOG_LEVEL_ERROR, L"Miss match poolSeed / freeObject Node : %016llx / Seed(%hu) != poolSeed(%hu)\n", freeNode, freeNode->seed, poolSeed);
			return false;
		}

		//--------------------------------------------------
		// freeNode(���� ������ �޸� �ּ�)�� ���� 17��Ʈ�� ����Ͽ� ��� ID �ο�
		//--------------------------------------------------
		ULONGLONG nodeID = InterlockedIncrement(&nodeSequence) % (1 << 17);
		freeNode = (Node*)((nodeID << 47) | (ULONGLONG)freeNode);

		Node* t;
		do
		{
			t = top;
			Node* maskedFreeNode = (Node*)((ULONGLONG)freeNode & nodeMask);
			maskedFreeNode->next = t;
		} while (InterlockedCompareExchangePointer((void* volatile*)&top, freeNode, t) != t);
		InterlockedIncrement(&poolCnt);

		if (!bPreConstructor)
			objectPtr->~T();

		return true;
	}

	ULONG GetPoolCnt()
	{
		return poolCnt;
	}

private:
	Node* top;
	bool bPreConstructor;
	USHORT poolSeed;
	ULONG poolCnt;

	//--------------------------------------------
	// ��� ���� ��, ��� ������ ����17��Ʈ�� �����ϴ� ����� ���� �ε���
	//--------------------------------------------
	ULONGLONG nodeSequence;

	//--------------------------------------------
	// Node*�� ���� 47��Ʈ ������ ����ũ
	//--------------------------------------------
	static const ULONGLONG nodeMask = (1ULL << 47) - 1;
};