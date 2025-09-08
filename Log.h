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
// 로그 출력 모드
// 1. 콘솔
//  - _LOG(dfLOG_LEVEL_~, L"~") 매크로를 이용합니다.
// 2. 즉시 파일 쓰기
//  - _LOG(dfLOG_LEVEL_~, L"~") 매크로를 이용합니다.
// 3. 스레드에서 작업 메시지 형식으로 파일 쓰기 수행
//  - st_LogMessage를 작성한다음에 _LOG(dfLOG_LEVEL_~, &st_LogMessage) 매크로를 이용합니다.
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
// 로그 출력 모드에 따른 리소스 초기화
//----------------------------------------------
bool InitLog(int logLevel, ELogMode logMode);


//----------------------------------------------
// 로그 출력 모드에 따른 로그 출력
//----------------------------------------------
void Log(int level, const wchar_t* fmt, ...);

void Log(int level, CLogMessage* m);

