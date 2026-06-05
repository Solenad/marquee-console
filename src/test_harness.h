#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// Helper to allocate a console for testing
// Returns 0 on success, -1 on failure.
inline int test_alloc_console()
{
    if (GetStdHandle(STD_OUTPUT_HANDLE) != INVALID_HANDLE_VALUE &&
        GetStdHandle(STD_OUTPUT_HANDLE) != NULL)
    {
        // Already has a console
        return 0;
    }
    return AllocConsole() ? 0 : -1;
}

// Helper to wait briefly (ms) and let console render pipe flush
inline void test_wait(DWORD ms)
{
    Sleep(ms);
}
