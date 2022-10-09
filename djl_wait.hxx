#pragma once

#include <windows.h>

class  CWaitPrecise
{
    private:

        HANDLE waitableTimer;

    public:

        CWaitPrecise() : waitableTimer( 0 )
        {
            waitableTimer = CreateWaitableTimer( 0, TRUE, 0 );
        }

        ~CWaitPrecise()
        {
            if ( 0 != waitableTimer )
                CloseHandle( waitableTimer );
        }

        bool SetTimer( PTIMERAPCROUTINE proutine, LPVOID arg, LONGLONG ms )
        {
            if ( 0 == waitableTimer )
                return false;

            LARGE_INTEGER li;
            li.QuadPart = -1;

            if ( !SetWaitableTimer( waitableTimer, &li, ms, proutine, arg, FALSE ) )
                return false;

            return true;
        } //SetTimer

        bool Wait( LONGLONG hundredNS )
        {
            if ( 0 == waitableTimer )
                return false;

            LARGE_INTEGER li;
            li.QuadPart = -hundredNS;
            if ( !SetWaitableTimer( waitableTimer, &li, 0, 0, 0, FALSE ) )
                return false;

            WaitForSingleObject( waitableTimer, INFINITE );
            return true;
        }

        bool WaitInMS( LONGLONG ms )
        {
            return Wait( ms * 10000 );
        }
}; //CWaitPrecise
