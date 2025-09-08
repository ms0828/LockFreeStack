#pragma once
#include <stdarg.h>
#include <strsafe.h> 
#include <Windows.h>
#include "RingBuffer.h"

#define dfLOG_LEVEL_DEBUG 0
#define dfLOG_LEVEL_ERROR 1
#define dfLOG_LEVEL_SYSTEM 2

#define LOG_BUFFER_LEN 1024


//----------------------------------------------
// �α� ��� ���
// 1. �ܼ�
//  - _LOG(dfLOG_LEVEL_~, L"~") ��ũ�θ� �̿��մϴ�.
// 2. ��� ���� ����
//  - _LOG(dfLOG_LEVEL_~, L"~") ��ũ�θ� �̿��մϴ�.
// 3. �����忡�� �۾� �޽��� �������� ���� ���� ����
//  - st_LogMessage�� �ۼ��Ѵ����� _LOG(dfLOG_LEVEL_~, &st_LogMessage) ��ũ�θ� �̿��մϴ�.
//----------------------------------------------
#define _LOG(Level, ...)               \
do{                                    \
    Log((Level), __VA_ARGS__);         \
} while (0)                            \


enum ELogMode
{
    CONSOLE = 0,
    FILE_DIRECT,
    FILE_THREAD
};

enum EArgType : char
{
    I32, U32, I64, U64, DBL, PTR,
    STR, B, C
};

struct st_ArgEntry
{
    const char* argName;
    EArgType type;
    union
    {
        int i32;
        unsigned int u32;
        long long i64;
        unsigned long long u64;
        double dbl;
        void* ptr;
        char* str;
        bool b;
        char ch;
    };
};


class CLogMessage
{
public:
    CLogMessage()
    {

    };

    CLogMessage(const char* name)
    {
        msgName = name;
        argNum = 0;
        memset(argEntries, 0, sizeof(st_ArgEntry) * 5);
        tid = GetCurrentThreadId();
        seq = 0;
    };

    inline unsigned int GetTID()
    {
        return tid;
    };


public:
    const char* msgName;
    st_ArgEntry argEntries[5];
    char argNum;
    unsigned int seq;
private:
    unsigned int tid;
};


//----------------------------------------------
// �α� ��� ��忡 ���� ���ҽ� �ʱ�ȭ
//----------------------------------------------
bool InitLog(int logLevel, ELogMode logMode);


//----------------------------------------------
// �α� ��� ��忡 ���� �α� ���
//----------------------------------------------
void Log(int level, const wchar_t* fmt, ...);

void Log(int level, CLogMessage* m);

