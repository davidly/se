#ifndef UNICODE
#define UNICODE
#endif 

#include <windows.h>
#include <stdio.h>
#include <math.h>

#include "djlres.hxx"
#include "djl_wait.hxx"

#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

#define REGISTRY_APP_NAME L"SOFTWARE\\davidlyseconds"
#define REGISTRY_WINDOW_POSITION L"WindowPosition"

LRESULT CALLBACK WindowProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

HFONT g_fontText;
COLORREF g_crRainbow[ 360 ];
SYSTEMTIME g_lt = {};

static void HSVToRGB( int h, int s, int v, int &r, int &g, int &b )
{
    // Assume h 0..359, s 0..255, v 0..255.
    const int sixtyDegrees = 60; // 60 out of 360, and 43 out of 256 (43, 85, 171 are color changes)
    const int divby = 255; // use 256 when performance is critical and accuracy is not.

    int hi = ( h / sixtyDegrees ); 
    int f = ( 255 * ( h % sixtyDegrees ) ) / sixtyDegrees; 
    int p = ( v * ( 255 - s ) ) / divby;

    int t = 0, q = 0;
    if ( hi & 0x1 )
        q = ( v * ( 255 - ( ( f * s ) / divby ) ) ) / divby;
    else
        t = ( v * ( 255 - ( ( ( 255 - f ) * s ) / divby ) ) ) / divby;

    if ( 0 == hi )
    {
        r = v; g = t; b = p;
    }
    else if ( 1 == hi )
    {
        r = q; g = v; b = p;
    }
    else if ( 2 == hi )
    {
        r = p; g = v; b = t;
    }
    else if ( 3 == hi )
    {
        r = p; g = q; b = v;
    }
    else if ( 4 == hi )
    {
        r = t; g = p; b = v;
    }
    else if ( 5 == hi )
    {
        r = v; g = p; b = q;
    }
    else
    {
        r = 0; g = 0; b = 0;
    }
} //HSVToRGB

void CALLBACK TimerApcRoutine( LPVOID arg, DWORD low, DWORD high )
{
    HWND hwnd = (HWND) arg;

    SYSTEMTIME lt;
    GetLocalTime( &lt );

    if ( lt.wSecond != g_lt.wSecond )
    {
        g_lt = lt;
        InvalidateRect( hwnd, NULL, TRUE );
    }
} //TimerApcRoutine

int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE, PWSTR pCmdLine, int nCmdShow )
{
    typedef BOOL ( WINAPI *LPFN_SPDAC )( DPI_AWARENESS_CONTEXT );
    LPFN_SPDAC spdac = (LPFN_SPDAC) GetProcAddress( GetModuleHandleA( "user32" ), "SetProcessDpiAwarenessContext" );
    if ( spdac )
        spdac( DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 );

    GetLocalTime( &g_lt );

    for ( int c = 0; c < _countof( g_crRainbow ); c++ )
    {
        int r, g, b;
        HSVToRGB( c, 0x70, 0xc0, r, g, b );
        g_crRainbow[ c ] = r | ( g << 8 ) | ( b << 16 );
    }

    RECT rectDesk;
    GetWindowRect( GetDesktopWindow(), &rectDesk );

    int fontHeight = (int) round( (double) rectDesk.bottom * 0.0208333 );
    int windowWidth = (int) round( (double) fontHeight * 3.9 );   // 2.8 for mm::ss

    g_fontText = CreateFont( fontHeight, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_OUTLINE_PRECIS,
                             CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, VARIABLE_PITCH, L"Tahoma" ); //TEXT("Arial") );

    int posLeft = rectDesk.right - windowWidth;
    int posTop = fontHeight;

    // Any command-line argument will override the registry setting with the default window position

    if ( 0 == pCmdLine[ 0 ] )
    {
        WCHAR awcPos[ 100 ];
        BOOL fFound = CDJLRegistry::readStringFromRegistry( HKEY_CURRENT_USER, REGISTRY_APP_NAME, REGISTRY_WINDOW_POSITION, awcPos, sizeof( awcPos ) );
        if ( fFound )
            swscanf( awcPos, L"%d %d", &posLeft, &posTop );
    }

    const WCHAR CLASS_NAME[] = L"Seconds-davidly-Class";
    WNDCLASS wc = { };
    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursor( NULL, IDC_ARROW );
    wc.hIcon         = LoadIcon( hInstance, MAKEINTRESOURCE( 100 ) ) ;
    wc.lpszClassName = CLASS_NAME;
    RegisterClass( &wc );

    HWND hwnd = CreateWindowEx( WS_EX_TOOLWINDOW, CLASS_NAME, L"Seconds", WS_POPUP, posLeft, posTop, windowWidth, fontHeight, NULL, NULL, hInstance, NULL );
    if ( NULL == hwnd )
        return 0;

    ShowWindow( hwnd, nCmdShow );

    CWaitPrecise wait;
    bool ok = wait.SetTimer( TimerApcRoutine, (LPVOID) hwnd, 20 );
    if ( !ok )
        return 0;

    SetProcessWorkingSetSize( GetCurrentProcess(), ~0, ~0 );

    MSG msg = {0};
    bool done = false;

    do
    {
        DWORD dw = MsgWaitForMultipleObjectsEx( 0, 0, INFINITE, QS_ALLINPUT, MWMO_ALERTABLE | MWMO_INPUTAVAILABLE );
        if ( WAIT_FAILED == dw )
            break;

        while ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );

            if ( WM_QUIT == msg.message )
            {
                done = true;
                break;
            }
        }
    } while( !done );

    DeleteObject( g_fontText );

    return 0;
} //wWinMain

__forceinline void WordToWC( WCHAR * pwc, WORD x )
{
    if ( x > 99 )
        x = 0;

    pwc[0] = ( x / 10 ) + L'0';
    pwc[1] = ( x % 10 ) + L'0';
} //WordToWC

LRESULT CALLBACK WindowProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    static RECT rectClient = {};
    static WCHAR awcSeconds[ 9 ] = { L'h', L'h', L':', L'm', L'm', L':', L's', L's', 0 };
    const int secondsLen = _countof( awcSeconds ) - 1;

    switch ( uMsg )
    {
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint( hwnd, &ps );

            double load = (double) g_lt.wSecond / 60.0;
            COLORREF cr = g_crRainbow[ (int) round( load * ( _countof( g_crRainbow ) - 1 ) ) ];

            WordToWC( awcSeconds,     g_lt.wHour );
            WordToWC( awcSeconds + 3, g_lt.wMinute );
            WordToWC( awcSeconds + 6, g_lt.wSecond );

            HFONT fontOld = (HFONT) SelectObject( hdc, g_fontText );
            COLORREF crOld = SetBkColor( hdc, cr );
            UINT taOld = SetTextAlign( hdc, TA_CENTER );
        
            ExtTextOut( hdc, rectClient.right / 2, 0, ETO_OPAQUE, &rectClient, awcSeconds, secondsLen, NULL );
    
            SetTextAlign( hdc, taOld );
            SetBkColor( hdc, crOld );
            SelectObject( hdc, fontOld );
            
            EndPaint( hwnd, &ps );
            return 0;
        }

        case WM_CHAR:
        {
            if ( 'q' == wParam || 0x1b == wParam ) // q or ESC
                DestroyWindow( hwnd );

            return 0;
        }

        case WM_NCHITTEST:
        {
            // Turn the whole window into what Windows thinks is the title bar so the user can drag the window around

            LRESULT hittest = DefWindowProc( hwnd, uMsg, wParam, lParam );
            if ( HTCLIENT == hittest )
                return HTCAPTION;

            return lParam;
        }

        case WM_SIZE:
        {
            GetClientRect( hwnd, &rectClient );
            break;
        }

        case WM_DESTROY:
        {
            RECT rectPos;
            GetWindowRect( hwnd, &rectPos );

            WCHAR awcPos[ 100 ];
            int len = swprintf_s( awcPos, _countof( awcPos ), L"%d %d", rectPos.left, rectPos.top );
            CDJLRegistry::writeStringToRegistry( HKEY_CURRENT_USER, REGISTRY_APP_NAME, REGISTRY_WINDOW_POSITION, awcPos );

            PostQuitMessage( 0 );
            return 0;
        }
    }

    return DefWindowProc( hwnd, uMsg, wParam, lParam );
} //WindowProc
