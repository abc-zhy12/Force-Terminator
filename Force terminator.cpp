#include <windows.h>
#include <shellapi.h>

// Global variables
const UINT WM_TRAYICON = WM_USER + 1;
const UINT HOTKEY_ID = 1;
HINSTANCE hInstance;
NOTIFYICONDATAW nid = {};
HMENU hMenu = NULL;
bool isHotkeyActive = true;

// Function declarations
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void CreateTrayIcon(HWND);
void ShowContextMenu(HWND);
void ToggleHotkey(bool);
void ForceTerminateActiveProcess();

// Main entry point
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int)
{
    hInstance = hInst;
    
    // Register window class
    WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = L"HiddenTrayApp";
    
    if (!RegisterClassExW(&wc))
    {
        MessageBoxW(NULL, L"Window Registration Failed!", L"Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    // Create hidden window - message window
    HWND hWnd = CreateWindowExW(
        0, 
        wc.lpszClassName, 
        L"", 
        0, 
        0, 0, 0, 0,
        HWND_MESSAGE, 
        NULL, 
        hInst, 
        NULL
    );
    
    if (!hWnd)
    {
        MessageBoxW(NULL, L"Window Creation Failed!", L"Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    // Create tray icon
    CreateTrayIcon(hWnd);
    
    // Register global hotkey: Ctrl+Shift+Alt+F4
    if (!RegisterHotKey(hWnd, HOTKEY_ID, MOD_CONTROL | MOD_SHIFT | MOD_ALT, VK_F4))
    {
        // Get error information for debugging
        DWORD error = GetLastError();
        wchar_t errorMsg[256];
        wsprintfW(errorMsg, L"Hotkey Registration Failed! Error: %d", error);
        MessageBoxW(hWnd, errorMsg, L"Warning", MB_ICONEXCLAMATION | MB_OK);
    }

    // Main message loop
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return (int)msg.wParam;
}

// Window procedure
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_HOTKEY:
            if (wParam == HOTKEY_ID && isHotkeyActive)
            {
                ForceTerminateActiveProcess();
            }
            break;

        case WM_TRAYICON:
            if (lParam == WM_RBUTTONUP)
            {
                ShowContextMenu(hWnd);
            }
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case 1:  // Enable
                    ToggleHotkey(true);
                    break;
                case 2:  // Disable
                    ToggleHotkey(false);
                    break;
                case 3:  // Exit
                    Shell_NotifyIconW(NIM_DELETE, &nid);
                    PostQuitMessage(0);
                    break;
            }
            break;

        case WM_DESTROY:
            Shell_NotifyIconW(NIM_DELETE, &nid);
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
    return 0;
}

// Create tray icon
void CreateTrayIcon(HWND hWnd)
{
    nid.cbSize = sizeof(NOTIFYICONDATAW);
    nid.hWnd = hWnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
    nid.uCallbackMessage = WM_TRAYICON;
    
    nid.hIcon = (HICON)LoadImageW(
        NULL, 
        MAKEINTRESOURCEW(32512), // IDI_APPLICATION resource ID
        IMAGE_ICON, 
        0, 
        0, 
        LR_SHARED | LR_DEFAULTSIZE
    );
    
    // Safely copy tooltip text
    wcsncpy(nid.szTip, L"Force Terminator", sizeof(nid.szTip)/sizeof(WCHAR) - 1);
    nid.szTip[sizeof(nid.szTip)/sizeof(WCHAR) - 1] = L'\0'; // Ensure null-terminated

    Shell_NotifyIconW(NIM_ADD, &nid);
}

// Show context menu
void ShowContextMenu(HWND hWnd)
{
    POINT pt;
    GetCursorPos(&pt);
    
    hMenu = CreatePopupMenu();
    if (hMenu)
    {
        InsertMenuW(hMenu, 0, MF_BYPOSITION | MF_STRING | (isHotkeyActive ? MF_CHECKED : MF_UNCHECKED), 1, L"Enable");
        InsertMenuW(hMenu, 1, MF_BYPOSITION | MF_STRING | (!isHotkeyActive ? MF_CHECKED : MF_UNCHECKED), 2, L"Disable");
        InsertMenuW(hMenu, 2, MF_BYPOSITION | MF_STRING, 3, L"Exit");

        SetForegroundWindow(hWnd);
        TrackPopupMenu(hMenu, TPM_BOTTOMALIGN, pt.x, pt.y, 0, hWnd, NULL);
        DestroyMenu(hMenu);
    }
}

// Toggle hotkey state
void ToggleHotkey(bool enable)
{
    isHotkeyActive = enable;
}

// Force terminate active process
void ForceTerminateActiveProcess()
{
    // Get foreground window (active window)
    HWND hwndForeground = GetForegroundWindow();
    if (hwndForeground)
    {
        DWORD processId;
        GetWindowThreadProcessId(hwndForeground, &processId);
        
        // Open the process with terminate permission
        HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, processId);
        if (hProcess)
        {
            // Force terminate
            TerminateProcess(hProcess, 0);
            CloseHandle(hProcess);
        }
    }
}