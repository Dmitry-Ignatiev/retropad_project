#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include "app.h"

// -----------------------------------------------------------------------------
// Forward Declarations
// -----------------------------------------------------------------------------
void ApplyCurrentFont(void);

// -----------------------------------------------------------------------------
// UpdateMenuStates
// Enables/Disables menu items based on current context (Selection, clipboard, etc.)
// -----------------------------------------------------------------------------
void UpdateMenuStates(HMENU hMenu) {
    if (!hMenu) return;

    EnableMenuItem(hMenu, IDM_EDIT_UNDO, SendMessageW(g_app.hwndEdit, EM_CANUNDO, 0, 0) ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(hMenu, IDM_EDIT_PASTE, IsClipboardFormatAvailable(CF_TEXT) || IsClipboardFormatAvailable(CF_UNICODETEXT) ? MF_ENABLED : MF_GRAYED);

    DWORD selStart = 0, selEnd = 0;
    SendMessageW(g_app.hwndEdit, EM_GETSEL, (WPARAM)&selStart, (LPARAM)&selEnd);
    BOOL hasSel = (selEnd > selStart);
    UINT selState = hasSel ? MF_ENABLED : MF_GRAYED;
    
    EnableMenuItem(hMenu, IDM_EDIT_CUT, selState);
    EnableMenuItem(hMenu, IDM_EDIT_COPY, selState);
    EnableMenuItem(hMenu, IDM_EDIT_DELETE, selState);

    int textLen = GetWindowTextLengthW(g_app.hwndEdit);
    EnableMenuItem(hMenu, IDM_EDIT_FIND, textLen > 0 ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(hMenu, IDM_EDIT_FINDNEXT, g_app.findText[0] ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(hMenu, IDM_EDIT_REPLACE, MF_ENABLED);

    BOOL canGoto = !g_app.wordWrap && (textLen > 0);
    EnableMenuItem(hMenu, IDM_EDIT_GOTO, canGoto ? MF_ENABLED : MF_GRAYED);

    CheckMenuItem(hMenu, IDM_FORMAT_WORDWRAP, g_app.wordWrap ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, IDM_VIEW_STATUSBAR, g_app.showStatus ? MF_CHECKED : MF_UNCHECKED);
    
    // In Classic Notepad, Status Bar is disabled if Word Wrap is ON
    EnableMenuItem(hMenu, IDM_VIEW_STATUSBAR, g_app.wordWrap ? MF_GRAYED : MF_ENABLED);
}

// -----------------------------------------------------------------------------
// UpdateStatusBarCaret
// Updates the "Ln X, Col Y" text in the status bar
// -----------------------------------------------------------------------------
void UpdateStatusBarCaret(void) {
    if (!g_app.showStatus || !g_app.hwndStatus || !g_app.hwndEdit) {
        return;
    }

    DWORD start = 0, end = 0;
    SendMessageW(g_app.hwndEdit, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);
    int line = (int)SendMessageW(g_app.hwndEdit, EM_LINEFROMCHAR, (WPARAM)start, 0) + 1;
    int lineIndex = (int)SendMessageW(g_app.hwndEdit, EM_LINEINDEX, line - 1, 0);
    int col = (int)(start - lineIndex) + 1;

    WCHAR buf[64];
    wsprintfW(buf, L"Ln %d, Col %d", line, col);
    SendMessageW(g_app.hwndStatus, SB_SETTEXTW, 0, (LPARAM)buf);
}

// -----------------------------------------------------------------------------
// ApplyCurrentFont
// -----------------------------------------------------------------------------
void ApplyCurrentFont(void) {
    if (!g_app.hFont) {
        return;
    }

    if (g_app.hwndEdit) {
        SendMessageW(g_app.hwndEdit, WM_SETFONT, (WPARAM)g_app.hFont, TRUE);
    }
    if (g_app.hwndStatus) {
        SendMessageW(g_app.hwndStatus, WM_SETFONT, (WPARAM)g_app.hFont, FALSE);
    }
}

// -----------------------------------------------------------------------------
// EnsureStatusBar
// -----------------------------------------------------------------------------
void EnsureStatusBar(void) {
    // If Word Wrap is on, we do NOT show status bar in classic mode
    if (!g_app.showStatus || g_app.wordWrap) {
        return;
    }
    if (g_app.hwndStatus) {
        return;
    }

    g_app.hwndStatus = CreateWindowExW(
        0,
        STATUSCLASSNAMEW,
        NULL,
        WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
        0, 0, 0, 0,
        g_app.hwndMain,
        (HMENU)IDC_STATUSBAR,
        g_app.hInst,
        NULL
    );

    ApplyCurrentFont();
}

// -----------------------------------------------------------------------------
// DestroyStatusBar
// -----------------------------------------------------------------------------
void DestroyStatusBar(void) {
    if (g_app.hwndStatus) {
        DestroyWindow(g_app.hwndStatus);
        g_app.hwndStatus = NULL;
    }
}

// -----------------------------------------------------------------------------
// ResizeClientArea
// -----------------------------------------------------------------------------
void ResizeClientArea(RECT clientRect) {
    int width = clientRect.right - clientRect.left;
    int height = clientRect.bottom - clientRect.top;
    int statusHeight = 0;

    if (g_app.showStatus && g_app.hwndStatus) {
        SendMessageW(g_app.hwndStatus, WM_SIZE, 0, 0);

        RECT rcStatus;
        GetClientRect(g_app.hwndStatus, &rcStatus);
        statusHeight = rcStatus.bottom;

        MoveWindow(g_app.hwndStatus,
                   0, height - statusHeight,
                   width, statusHeight, TRUE);
    }

    if (g_app.hwndEdit) {
        MoveWindow(g_app.hwndEdit,
                   0, 0,
                   width, height - statusHeight,
                   TRUE);
    }
}

// -----------------------------------------------------------------------------
// RecreateEditControl
// -----------------------------------------------------------------------------
void RecreateEditControl(void) {
    RECT rc;
    GetClientRect(g_app.hwndMain, &rc);

    HWND oldEdit = g_app.hwndEdit;
    DWORD selStart = 0, selEnd = 0;
    int len = 0;
    WCHAR *buffer = NULL;

    // Preserve old text and selection
    if (oldEdit && IsWindow(oldEdit)) {
        len = GetWindowTextLengthW(oldEdit);
        if (len > 0) {
            buffer = (WCHAR *)LocalAlloc(LPTR, sizeof(WCHAR) * (len + 1));
            if (buffer) {
                GetWindowTextW(oldEdit, buffer, len + 1);
            }
        }
        LONG start = 0, end = 0;
        SendMessageW(oldEdit, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);
        selStart = (DWORD)start;
        selEnd = (DWORD)end;
    }

    // Define styles
    DWORD style = WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP |
                  ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN;

    DWORD exStyle = WS_EX_CLIENTEDGE;
    
    // Logic: If WordWrap is OFF, we need a Horizontal Scrollbar.
    if (!g_app.wordWrap) {
        style |= WS_HSCROLL | ES_AUTOHSCROLL;
    }

    // Create the new EDIT control
    g_app.hwndEdit = CreateWindowExW(
        exStyle,
        L"EDIT",
        L"",
        style,
        0, 0, rc.right, rc.bottom,
        g_app.hwndMain,
        (HMENU)IDC_MAIN_EDIT,
        g_app.hInst,
        NULL
    );

    if (!g_app.hwndEdit) {
        DWORD err = GetLastError();
        WCHAR msg[128];
        wsprintfW(msg, L"EDIT create failed. GetLastError=%lu", err);
        MessageBoxW(g_app.hwndMain, msg, APP_NAME, MB_ICONERROR);
        return;
    }

    // Apply font and settings
    SendMessageW(g_app.hwndEdit, EM_SETLIMITTEXT, 0, 0);
    ApplyCurrentFont();

    // Restore text and selection
    if (buffer) {
        g_app.suppressChange = TRUE;
        SetWindowTextW(g_app.hwndEdit, buffer);
        SendMessageW(g_app.hwndEdit, EM_SETSEL, selStart, selEnd);
        g_app.suppressChange = FALSE;
        LocalFree(buffer);
    }

    // Destroy old control only after new one is valid
    if (oldEdit && IsWindow(oldEdit)) {
        DestroyWindow(oldEdit);
    }

    // Resize and update UI
    ResizeClientArea(rc);
    UpdateStatusBarCaret();

    // Show and focus the control
    ShowWindow(g_app.hwndEdit, SW_SHOW);
    SetFocus(g_app.hwndEdit);
}

// -----------------------------------------------------------------------------
// ToggleWordWrap
// -----------------------------------------------------------------------------
void ToggleWordWrap(void) {
    if (!IsWindow(g_app.hwndMain)) {
        return;
    }

    g_app.wordWrap = !g_app.wordWrap;
    
    // Classic Notepad Logic: Word Wrap forces Status Bar OFF
    if (g_app.wordWrap) {
        // Force status bar off visually but remember the preference? 
        // Classic notepad actually disables the menu option.
        DestroyStatusBar();
    } else {
        // If we unwrapped, and the user wanted the status bar, bring it back
        if (g_app.showStatus) {
            EnsureStatusBar();
        }
    }

    RecreateEditControl();
    UpdateWindowTitle();
}

// -----------------------------------------------------------------------------
// ToggleStatusBar
// -----------------------------------------------------------------------------
void ToggleStatusBar(void) {
    g_app.showStatus = !g_app.showStatus;

    if (g_app.showStatus) {
        EnsureStatusBar();
    } else {
        DestroyStatusBar();
    }

    RECT rc;
    GetClientRect(g_app.hwndMain, &rc);
    ResizeClientArea(rc);
    UpdateStatusBarCaret();
}