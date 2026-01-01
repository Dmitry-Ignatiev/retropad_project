#define WIN32_LEAN_AND_MEAN
#define WINVER 0x0500
#define _WIN32_WINNT 0x0500
#define _WIN32_IE 0x0500

#include <windows.h>
#include <tchar.h>
#include <strsafe.h>
#include <commctrl.h>

#include "resource.h"
#include "app.h"

// Global Application State
AppState g_app = {0};

HWND g_hTab = NULL;
static HBRUSH g_hEditBgBrush = NULL;
// Forward Declarations
static LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL RegisterMainWindowClass(HINSTANCE hInstance);
static void HandleCommandLine(LPWSTR lpCmdLine);
static void EnsureDefaultFont(void);
static void OnCommand(HWND hwnd, WPARAM wParam, LPARAM lParam);

// Initialize the application
BOOL InitApplication(HINSTANCE hInstance, int nCmdShow) {
    InitCommonControls();
    
    INITCOMMONCONTROLSEX icex = {0};
    icex.dwSize = sizeof(icex);
    icex.dwICC = ICC_BAR_CLASSES|ICC_TAB_CLASSES;
    InitCommonControlsEx(&icex);

    g_app.hInst = hInstance;
    g_app.wordWrap = FALSE;
    g_app.showStatus = TRUE; 
    g_app.dirty = FALSE;
    g_app.suppressChange = FALSE;
    g_app.filePath[0] = L'\0';
    
    ZeroMemory(&g_app.find, sizeof(g_app.find));
    ZeroMemory(&g_app.logFont, sizeof(g_app.logFont));

    if (!RegisterMainWindowClass(hInstance)) {
        return FALSE;
    }

    HWND hwnd = CreateWindowExW(
        WS_EX_ACCEPTFILES, 
        APP_NAME,
        L"Untitled - " APP_NAME, 
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 900, 600,
        NULL, NULL, hInstance, NULL);
    
    if (!hwnd) {
        MessageBoxW(NULL, L"Unable to create main window.", APP_NAME, MB_ICONERROR);
        return FALSE;
    }

    g_app.hwndMain = hwnd;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
    return TRUE;
}

// Entry Point
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    
    if (!InitApplication(hInstance, nCmdShow)) {
        return 0;
    }

    if (lpCmdLine && lpCmdLine[0]) {
        HandleCommandLine(lpCmdLine);
    }

    HACCEL hAccel = LoadAcceleratorsW(hInstance, MAKEINTRESOURCEW(IDR_ACCEL));
    MSG msg;

    while (GetMessageW(&msg, NULL, 0, 0)) {
        if (!TranslateAcceleratorW(g_app.hwndMain, hAccel, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    return (int)msg.wParam;
}

static BOOL RegisterMainWindowClass(HINSTANCE hInstance) {
    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = MainWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_APPICON));
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = MAKEINTRESOURCEW(IDR_MAINMENU);
    wc.lpszClassName = APP_NAME;
    wc.hIconSm = wc.hIcon;
    return RegisterClassExW(&wc) != 0;
}

static void HandleCommandLine(LPWSTR lpCmdLine) {
    WCHAR *path = lpCmdLine;
    BOOL isQuoted = (*path == L'\"');
    
    if (isQuoted) path++;

    size_t len = lstrlenW(path);
    if (isQuoted && len > 0 && path[len - 1] == L'\"') {
        path[len - 1] = L'\0';
    }

    LoadDocument(path);
}

void MarkDocumentDirty(BOOL dirty) {
    if (g_app.dirty != dirty) {
        g_app.dirty = dirty;
        UpdateWindowTitle();
    }
}

static void EnsureDefaultFont(void) {
    if (g_app.hFont) return;

    NONCLIENTMETRICSW ncm = {0};
    ncm.cbSize = sizeof(ncm);
    if (SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0)) {
        g_app.logFont = ncm.lfMessageFont;
        g_app.hFont = CreateFontIndirectW(&g_app.logFont);
    }
    if (!g_app.hFont) {
        g_app.hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        if (g_app.hFont) {
            GetObjectW(g_app.hFont, sizeof(g_app.logFont), &g_app.logFont);
        }
    }
}

// NOTE: UpdateMenuStates and UpdateStatusBarCaret removed from here
// because they are now in ui.c

static void OnCommand(HWND hwnd, WPARAM wParam, LPARAM lParam) {
    UINT cmdId = LOWORD(wParam);

    switch (cmdId) {
    case IDM_FILE_NEW:      NewDocument(); break;
    case IDM_FILE_OPEN:     OpenDocument(); break;
    case IDM_FILE_SAVE:     SaveDocument(FALSE); break;
    case IDM_FILE_SAVEAS:   SaveDocument(TRUE); break;

    // --- CORRECTION STARTS HERE ---
    // Do not use "void FunctionName..."; use "case ID:"
    case IDM_FILE_PAGESETUP: ShowPageSetupDialog(hwnd); break;
    case IDM_FILE_PRINT:     PrintDocument(hwnd); break;
    // --- CORRECTION ENDS HERE ---

    case IDM_FILE_EXIT:     SendMessageW(hwnd, WM_CLOSE, 0, 0); break;

    case IDM_EDIT_UNDO:     SendMessageW(g_app.hwndEdit, EM_UNDO, 0, 0); break;
    case IDM_EDIT_CUT:      SendMessageW(g_app.hwndEdit, WM_CUT, 0, 0); break;
    case IDM_EDIT_COPY:     SendMessageW(g_app.hwndEdit, WM_COPY, 0, 0); break;
    case IDM_EDIT_PASTE:    SendMessageW(g_app.hwndEdit, WM_PASTE, 0, 0); break;
    case IDM_EDIT_DELETE:   SendMessageW(g_app.hwndEdit, WM_CLEAR, 0, 0); break;
    case IDM_EDIT_FIND:     ShowFindDialog(FALSE); break;
    case IDM_EDIT_FINDNEXT: FindNext(); break;
    case IDM_EDIT_REPLACE:  ShowFindDialog(TRUE); break;
    case IDM_EDIT_GOTO:     ShowGotoDialog(hwnd); break;
    case IDM_EDIT_SELECTALL:SendMessageW(g_app.hwndEdit, EM_SETSEL, 0, -1); break;
    case IDM_EDIT_TIME:     InsertTimeDate(); break;

    case IDM_FORMAT_WORDWRAP: ToggleWordWrap(); break;
    case IDM_FORMAT_FONT:     ShowFontDialog(hwnd); break;
    case IDM_VIEW_STATUSBAR:  ToggleStatusBar(); break;
    case IDM_HELP_ABOUT:      ShowAboutDialog(hwnd); break;
    }

    if (lParam && (HWND)lParam == g_app.hwndEdit) {
        UINT code = HIWORD(wParam);
        if (code == EN_CHANGE && !g_app.suppressChange) {
            MarkDocumentDirty(TRUE);
            UpdateStatusBarCaret(); 
        }
    }
}
//helper function to resize window and layout controls
 void LayoutChildren(HWND hwnd)
{
    RECT rcClient;
    GetClientRect(hwnd, &rcClient);

    // Status bar
    int statusHeight = 0;
    if (g_app.hwndStatus) {
        SendMessageW(g_app.hwndStatus, WM_SIZE, 0, 0);

        RECT sr;
        GetClientRect(g_app.hwndStatus, &sr);
        statusHeight = sr.bottom;
    }

    int y = 0;

    // Tabs
    int tabHeight = 0;
    if (g_hTab) {
        RECT tr = rcClient;
        TabCtrl_AdjustRect(g_hTab, FALSE, &tr);
        tabHeight = tr.top;

        SetWindowPos(
            g_hTab,
            NULL,
            0, 0,
            rcClient.right,
            tabHeight,
            SWP_NOZORDER | SWP_NOACTIVATE
        );
    }

    y = tabHeight;

    // Edit control
    if (g_app.hwndEdit) {
        SetWindowPos(
            g_app.hwndEdit,
            NULL,
            0, y,
            rcClient.right,
            rcClient.bottom - y - statusHeight,
            SWP_NOZORDER | SWP_NOACTIVATE
        );
    }

    UpdateStatusBarCaret();
}


// -----------------------------------------------------------------------------
// Main Window Procedure
// -----------------------------------------------------------------------------
static LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // Handle custom Find/Replace message
    if (msg == g_app.findMsgId && msg != 0) {
        HandleFindReplace(lParam);
        return 0;
    }

    switch (msg)
    {
    case WM_CTLCOLOREDIT:
    {
        HDC hdc = (HDC)wParam;
        HWND hCtrl = (HWND)lParam;

        if (hCtrl == g_app.hwndEdit) {
            SetBkColor(hdc, RGB(255, 255, 255));
            SetTextColor(hdc, RGB(0, 0, 0));

            if (!g_hEditBgBrush) {
                g_hEditBgBrush = CreateSolidBrush(RGB(255, 255, 255));
            }
            return (LRESULT)g_hEditBgBrush;
        }
        break;
    }

    case WM_CREATE:
    {
        g_app.hwndMain = hwnd;
        EnsureDefaultFont();
        g_app.findMsgId = RegisterWindowMessageW(FINDMSGSTRINGW);

        RECT rcClient;
        GetClientRect(hwnd, &rcClient);

        g_hTab = CreateWindowExW(
            0, WC_TABCONTROL, NULL,
            WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE,
            0, 0,
            rcClient.right,
            rcClient.bottom - 20,
            hwnd, (HMENU)IDC_MAIN_TAB, g_app.hInst, NULL
        );

        if (g_hTab) {
            TCITEM tie = {0};
            tie.mask = TCIF_TEXT;
            tie.pszText = L"Untitled 1";
            TabCtrl_InsertItem(g_hTab, 0, &tie);
            TabCtrl_SetCurSel(g_hTab, 0);
        }

        RecreateEditControl();
        EnsureStatusBar();
        UpdateStatusBarCaret();
        UpdateWindowTitle();
        DragAcceptFiles(hwnd, TRUE);
        return 0;
    }


   


    case WM_SETFOCUS:
        if (g_app.hwndEdit) SetFocus(g_app.hwndEdit);
        return 0;

    case WM_COMMAND:
        OnCommand(hwnd, wParam, lParam);
        UpdateStatusBarCaret();
        return 0;

    case WM_INITMENUPOPUP:
        UpdateMenuStates((HMENU)wParam);
        return 0;

    case WM_DROPFILES:
    {
        HDROP hDrop = (HDROP)wParam;
        WCHAR path[MAX_PATH];

        if (DragQueryFileW(hDrop, 0, path, MAX_PATH)) {
            if (PromptSaveIfDirty()) {
                LoadDocument(path);
            }
        }
        DragFinish(hDrop);
        return 0;
    }
    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED)
            LayoutChildren(hwnd);
        return 0;
    case WM_QUERYENDSESSION:
        return PromptSaveIfDirty();

    case WM_CLOSE:
        if (PromptSaveIfDirty()) DestroyWindow(hwnd);
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_DESTROY:
        if (g_app.hFont && g_app.hFont != GetStockObject(DEFAULT_GUI_FONT)) {
            DeleteObject(g_app.hFont);
            g_app.hFont = NULL;
        }

        if (g_hEditBgBrush) {
            DeleteObject(g_hEditBgBrush);
            g_hEditBgBrush = NULL;
        }

        PostQuitMessage(0);
        return 0;
    }

    // ✅ switch ends here
    
   return DefWindowProcW(hwnd, msg, wParam, lParam);
}
