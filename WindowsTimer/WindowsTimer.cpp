// WindowsTimer.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <math.h>
#include <Windows.h>
#include <MMSystem.h>
#include <corecrt_wstdio.h>
#include <tchar.h>

#pragma comment(lib, "winmm")

LARGE_INTEGER freq;
double elapsed1[10000], elapsed2[10000], elapsed3[10000], elapsed4[10000];
int idx1, idx2, idx3, idx4;

// timer queue
void CALLBACK timer1_callback(void* param, BOOLEAN unused)
{
    static LARGE_INTEGER old = { 0 };
    LARGE_INTEGER current;

    if (old.QuadPart == 0)
        QueryPerformanceCounter(&old);

    QueryPerformanceCounter(&current);
    double elapsed = (current.QuadPart - old.QuadPart) / (double)freq.QuadPart;
    elapsed1[idx1] = elapsed;
    old.QuadPart = current.QuadPart;
    idx1++;
}

// MM timer
void CALLBACK timer2_callback(UINT uTimerID, UINT uMsg, DWORD_PTR param, DWORD_PTR dw1, DWORD_PTR dw2)
{
    static LARGE_INTEGER old = { 0 };
    LARGE_INTEGER current;

    if (old.QuadPart == 0)
        QueryPerformanceCounter(&old);

    QueryPerformanceCounter(&current);
    double elapsed = (current.QuadPart - old.QuadPart) / (double)freq.QuadPart;
    elapsed2[idx2] = elapsed;
    old.QuadPart = current.QuadPart;
    idx2++;
}

// busy wait
void timer3_callback()
{
    static LARGE_INTEGER old = { 0 };
    LARGE_INTEGER current;

    if (old.QuadPart == 0)
        QueryPerformanceCounter(&old);

    QueryPerformanceCounter(&current);
    double elapsed = (current.QuadPart - old.QuadPart) / (double)freq.QuadPart;
    elapsed3[idx3] = elapsed;
    old.QuadPart = current.QuadPart;
    idx3++;
}

// SetTimer
DWORD WINAPI UiThread(void* param)
{
    static LARGE_INTEGER old = { 0 };
    LARGE_INTEGER current;
    MSG msg;
    DWORD period = (DWORD)param;

    SetTimer(0, 1, period, 0); // this creates message queue

    while (1) // message loop
    {
        if (GetMessage(&msg, 0, 0, 0))
        {
            switch (msg.message)
            {
            case WM_TIMER:
                if (old.QuadPart == 0)
                    QueryPerformanceCounter(&old);

                QueryPerformanceCounter(&current);
                double elapsed = (current.QuadPart - old.QuadPart) / (double)freq.QuadPart;
                elapsed4[idx4] = elapsed;
                old.QuadPart = current.QuadPart;
                idx4++;
                break;
            }
        }
    }

    return 0;
}

void display_stats(double* array, int count, DWORD period, WCHAR* name)
{
    double off, min, total = 0, avg = 0, max = 0, stddev = 0;
    min = period * 10;

    count--;
    for (int i = 1; i < count + 1; i++)
    {
        off = fabs(array[i] - period / 1000.0) / (period / 1000.0) * 100.0;
        total += off;
        if (off < min)
            min = off;
        if (off > max)
            max = off;
    }
    avg = total / count;

    for (int i = 1; i < count + 1; i++)
    {
        off = fabs(array[i] - period / 1000.0) / (period / 1000.0) * 100.0;
        stddev += (off - avg) * (off - avg);
    }
    stddev /= (count - 1);
    stddev = sqrt(stddev);

    wprintf(L"%-25s: count=%4d, avg error=%6.3f%%, std dev=%6.3f\n", name, count, avg, stddev);
}

int _tmain(int argc, _TCHAR* argv[])
{
    LARGE_INTEGER pc1, pc2;
    QueryPerformanceFrequency(&freq);
    DWORD periods[] = { 100, 50, 20, 10, 5, 2, 1 }; // timer periods in ms

    timeBeginPeriod(1); // set best multimedia timer resolution, in case it affectso ther timers

    for (int p = 0; p < 7; p++)
    {
        DWORD period = periods[p];
        wprintf(L"\nPeriod: %dms\n", period);

        // timer queue
        idx1 = 0;
        HANDLE timer1;
        CreateTimerQueueTimer(&timer1, NULL, timer1_callback, 0, 0, period, 0);
        QueryPerformanceCounter(&pc1);
        Sleep(1000);
        QueryPerformanceCounter(&pc2);
        DeleteTimerQueueTimer(NULL, timer1, 0);
        display_stats(elapsed1, idx1, period, L"CreateTimerQueueTimer");

        // MM timer
        idx2 = 0;
        MMRESULT timer2 = timeSetEvent(period, 0, timer2_callback, 0, TIME_PERIODIC);
        QueryPerformanceCounter(&pc1);
        Sleep(1000);
        QueryPerformanceCounter(&pc2);
        timeKillEvent(timer2);
        display_stats(elapsed2, idx2, period, L"timeSetEvent");

        // busy waiting
        idx3 = 0;
        double slice = period / 1000.0;
        QueryPerformanceCounter(&pc1);
        while ((pc2.QuadPart - pc1.QuadPart) / (double)freq.QuadPart < 1)
        {
            // "timer" function
            timer3_callback();

            // delay till next loop
            do
            {
                QueryPerformanceCounter(&pc2);
            } while ((pc2.QuadPart - pc1.QuadPart) / (double)freq.QuadPart < slice * idx3);
        }
        QueryPerformanceCounter(&pc2);
        display_stats(elapsed3, idx3, period, L"Busy wait");

        // user32 SetTimer
        idx4 = 0;
        HANDLE thread = CreateThread(0, 0, UiThread, (void*)period, 0, 0);
        QueryPerformanceCounter(&pc1);
        Sleep(1000);
        QueryPerformanceCounter(&pc2);
        TerminateThread(thread, 0);
        display_stats(elapsed4, idx4, period, L"SetTimer");
    }
    timeEndPeriod(1);

    return 0;
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
