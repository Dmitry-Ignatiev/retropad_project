#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include "app.h"


static BOOL SearchBuffer(const WCHAR *text, int textLen, const WCHAR *needle, int needleLen,
                         BOOL matchCase, BOOL wholeWord, BOOL searchDown, int startPos, int *matchPos);
static BOOL IsWholeWordMatch(const WCHAR *text, int textLen, int pos, int len);
static BOOL ExecuteFindReplace(BOOL doReplace, BOOL replaceAll);
static INT_PTR CALLBACK GotoDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK AboutDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

void InsertTimeDate(void) {
    SYSTEMTIME st;
    GetLocalTime(&st);

    WCHAR dateBuf[64] = {0};
    WCHAR timeBuf[64] = {0};
    GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &st, NULL, dateBuf, ARRAYSIZE(dateBuf));
    GetTimeFormatW(LOCALE_USER_DEFAULT, 0, &st, NULL, timeBuf, ARRAYSIZE(timeBuf));

    WCHAR stamp[128];
    wsprintfW(stamp, L"%s %s", timeBuf, dateBuf);
    SendMessageW(g_app.hwndEdit, EM_REPLACESEL, TRUE, (LPARAM)stamp);
}

void ShowFindDialog(BOOL replaceMode) {
    g_app.find.lStructSize = sizeof(g_app.find);
    g_app.find.hwndOwner = g_app.hwndMain;
    g_app.find.hInstance = g_app.hInst;
    g_app.find.lpstrFindWhat = g_app.findText;
    g_app.find.wFindWhatLen = ARRAYSIZE(g_app.findText);
    g_app.find.lpstrReplaceWith = g_app.replaceText;
    g_app.find.wReplaceWithLen = ARRAYSIZE(g_app.replaceText);
    if (!(g_app.find.Flags & FR_DOWN)) {
        g_app.find.Flags |= FR_DOWN;
    }

    if (replaceMode) {
        ReplaceTextW(&g_app.find);
    } else {
        FindTextW(&g_app.find);
    }
}

void HandleFindReplace(LPARAM lParam) {
    FINDREPLACEW *fr = (FINDREPLACEW *)lParam;
    if (fr->Flags & FR_DIALOGTERM) {
        return;
    }

    lstrcpynW(g_app.findText, fr->lpstrFindWhat, ARRAYSIZE(g_app.findText));
    lstrcpynW(g_app.replaceText, fr->lpstrReplaceWith, ARRAYSIZE(g_app.replaceText));
    g_app.find.Flags = fr->Flags;

    if (fr->Flags & FR_REPLACEALL) {
        ExecuteFindReplace(TRUE, TRUE);
    } else if (fr->Flags & FR_REPLACE) {
        ExecuteFindReplace(TRUE, FALSE);
    } else if (fr->Flags & FR_FINDNEXT) {
        ExecuteFindReplace(FALSE, FALSE);
    }
}

void FindNext(void) {
    if (!g_app.findText[0]) {
        ShowFindDialog(FALSE);
        return;
    }
    ExecuteFindReplace(FALSE, FALSE);
}

static BOOL ExecuteFindReplace(BOOL doReplace, BOOL replaceAll) {
    if (!g_app.findText[0]) {
        return FALSE;
    }

    DWORD selStart = 0, selEnd = 0;
    SendMessageW(g_app.hwndEdit, EM_GETSEL, (WPARAM)&selStart, (LPARAM)&selEnd);
    BOOL searchDown = (g_app.find.Flags & FR_DOWN) != 0;
    int startPos = searchDown ? (int)selEnd : (int)selStart;

    BOOL foundAny = FALSE;
    for (;;) {
        int textLen = GetWindowTextLengthW(g_app.hwndEdit);
        WCHAR *buffer = (WCHAR *)LocalAlloc(LPTR, sizeof(WCHAR) * (textLen + 1));
        if (!buffer) {
            return FALSE;
        }
        GetWindowTextW(g_app.hwndEdit, buffer, textLen + 1);

        int needleLen = lstrlenW(g_app.findText);
        int matchPos = -1;
        BOOL found = SearchBuffer(buffer, textLen, g_app.findText, needleLen,
                                  (g_app.find.Flags & FR_MATCHCASE) != 0,
                                  (g_app.find.Flags & FR_WHOLEWORD) != 0,
                                  searchDown, startPos, &matchPos);
        if (!found) {
            LocalFree(buffer);
            break;
        }

        foundAny = TRUE;
        SendMessageW(g_app.hwndEdit, EM_SETSEL, matchPos, matchPos + needleLen);
        SendMessageW(g_app.hwndEdit, EM_SCROLLCARET, 0, 0);

        if (doReplace) {
            SendMessageW(g_app.hwndEdit, EM_REPLACESEL, TRUE, (LPARAM)g_app.replaceText);
            MarkDocumentDirty(TRUE);
        }

        LocalFree(buffer);

        if (!replaceAll || !doReplace) {
            break;
        }

        DWORD newStart = 0, newEnd = 0;
        SendMessageW(g_app.hwndEdit, EM_GETSEL, (WPARAM)&newStart, (LPARAM)&newEnd);
        startPos = searchDown ? (int)newEnd : (int)newStart;
    }

    if (!foundAny) {
        MessageBoxW(g_app.hwndMain, L"Cannot find the specified text.", APP_NAME, MB_ICONINFORMATION);
    }
    return foundAny;
}

static BOOL SearchBuffer(const WCHAR *text, int textLen, const WCHAR *needle, int needleLen,
                         BOOL matchCase, BOOL wholeWord, BOOL searchDown, int startPos, int *matchPos) {
    if (!text || !needle || needleLen <= 0) {
        return FALSE;
    }

    int maxStart = textLen - needleLen;
    if (startPos < 0) startPos = 0;
    if (startPos > textLen) startPos = textLen;

    WCHAR *haystack = NULL;
    WCHAR *pattern = NULL;
    if (!matchCase) {
        haystack = (WCHAR *)LocalAlloc(LPTR, sizeof(WCHAR) * (textLen + 1));
        pattern = (WCHAR *)LocalAlloc(LPTR, sizeof(WCHAR) * (needleLen + 1));
        if (!haystack || !pattern) {
            if (haystack) LocalFree(haystack);
            if (pattern) LocalFree(pattern);
            return FALSE;
        }
        lstrcpynW(haystack, text, textLen + 1);
        lstrcpynW(pattern, needle, needleLen + 1);
        CharUpperBuffW(haystack, textLen);
        CharUpperBuffW(pattern, needleLen);
    }

    const WCHAR *hay = matchCase ? text : haystack;
    const WCHAR *pat = matchCase ? needle : pattern;

    int pos = searchDown ? startPos : startPos - needleLen;
    if (!searchDown && pos > maxStart) {
        pos = maxStart;
    }

    for (; searchDown ? (pos <= maxStart) : (pos >= 0); searchDown ? (++pos) : (--pos)) {
        if (wcsncmp(hay + pos, pat, needleLen) == 0) {
            if (!wholeWord || IsWholeWordMatch(text, textLen, pos, needleLen)) {
                if (haystack) LocalFree(haystack);
                if (pattern) LocalFree(pattern);
                *matchPos = pos;
                return TRUE;
            }
        }
    }

    if (haystack) LocalFree(haystack);
    if (pattern) LocalFree(pattern);
    return FALSE;
}

static BOOL IsWholeWordMatch(const WCHAR *text, int textLen, int pos, int len) {
    int left = pos - 1;
    int right = pos + len;
    BOOL leftOk = (left < 0) || !iswalnum(text[left]);
    BOOL rightOk = (right >= textLen) || !iswalnum(text[right]);
    return leftOk && rightOk;
}

void ShowGotoDialog(HWND owner) {
    if (g_app.wordWrap) {
        MessageBoxW(owner, L"Go To is unavailable when Word Wrap is on.", APP_NAME, MB_ICONINFORMATION);
        return;
    }
    DialogBoxParamW(g_app.hInst, MAKEINTRESOURCEW(IDD_GOTO), owner, GotoDlgProc, 0);
}

static INT_PTR CALLBACK GotoDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_INITDIALOG:
        SendDlgItemMessageW(hDlg, IDC_GOTO_EDIT, EM_SETLIMITTEXT, 10, 0);
        return TRUE;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK: {
            WCHAR input[32];
            GetDlgItemTextW(hDlg, IDC_GOTO_EDIT, input, ARRAYSIZE(input));
            int line = _wtoi(input);
            if (line <= 0) {
                MessageBoxW(hDlg, L"Please enter a valid line number.", APP_NAME, MB_ICONINFORMATION);
                return TRUE;
            }
            int lineCount = (int)SendMessageW(g_app.hwndEdit, EM_GETLINECOUNT, 0, 0);
            if (line > lineCount) {
                MessageBoxW(hDlg, L"Line number is too large.", APP_NAME, MB_ICONINFORMATION);
                return TRUE;
            }
            int idx = (int)SendMessageW(g_app.hwndEdit, EM_LINEINDEX, line - 1, 0);
            if (idx >= 0) {
                SendMessageW(g_app.hwndEdit, EM_SETSEL, idx, idx);
                SendMessageW(g_app.hwndEdit, EM_SCROLLCARET, 0, 0);
            }
            EndDialog(hDlg, IDOK);
            return TRUE;
        }
        case IDCANCEL:
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
        default:
            break;
        }
        break;
    default:
        break;
    }
    return FALSE;
}

void ShowFontDialog(HWND owner) {
    CHOOSEFONTW cf = {0};
    cf.lStructSize = sizeof(cf);
    cf.hwndOwner = owner;
    cf.hInstance = g_app.hInst;
    cf.lpLogFont = &g_app.logFont;
    cf.Flags = CF_SCREENFONTS | CF_EFFECTS | CF_INITTOLOGFONTSTRUCT;
    if (ChooseFontW(&cf)) {
        HFONT newFont = CreateFontIndirectW(&g_app.logFont);
        if (newFont) {
            if (g_app.hFont && g_app.hFont != GetStockObject(DEFAULT_GUI_FONT)) {
                DeleteObject(g_app.hFont);
            }
            g_app.hFont = newFont;
            ApplyCurrentFont();
        }
    }
}

void ShowAboutDialog(HWND owner) {
    DialogBoxParamW(g_app.hInst, MAKEINTRESOURCEW(IDD_ABOUTBOX), owner, AboutDlgProc, 0);
}

static INT_PTR CALLBACK AboutDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    UNREFERENCED_PARAMETER(lParam);
    switch (msg) {
    case WM_INITDIALOG:
        return TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, (int)LOWORD(wParam));
            return TRUE;
        }
        break;
    default:
        break;
    }
    return FALSE;
}
