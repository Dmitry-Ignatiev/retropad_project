#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include "app.h"

static BOOL ChooseFilePath(HWND owner, LPWSTR buffer, DWORD length, BOOL saveDialog);
static BOOL WriteBufferToFile(LPCWSTR path, const BYTE *data, DWORD size);
static BOOL ReadFileToBuffer(LPCWSTR path, BYTE **data, DWORD *size);
static BOOL DecodeBufferToWide(const BYTE *data, DWORD size, WCHAR **outText);
static int g_untitledCounter = 1;
void UpdateWindowTitle(void) {
    WCHAR name[MAX_PATH];
    if (g_app.filePath[0]) {
        lstrcpynW(name, g_app.filePath, MAX_PATH);
    } else {
        lstrcpyW(name, L"Untitled");
    }

    WCHAR title[MAX_PATH + 32];
    wsprintfW(title, L"%s%s - %s", g_app.dirty ? L"*" : L"", name, APP_NAME);
    if (g_app.hwndMain) {
        SetWindowTextW(g_app.hwndMain, title);
    }
}



BOOL PromptSaveIfDirty(void) {
    if (!g_app.dirty) {
        return TRUE;
    }

    int res = MessageBoxW(g_app.hwndMain,
                          L"Do you want to save changes to this document?",
                          APP_NAME,
                          MB_ICONQUESTION | MB_YESNOCANCEL | MB_DEFBUTTON1);
    if (res == IDCANCEL) {
        return FALSE;
    }
    if (res == IDYES) {
        return SaveDocument(FALSE);
    }
    return TRUE;
}

void NewDocument(void) {
    if (!PromptSaveIfDirty()) {
        return;
    }
    g_app.suppressChange = TRUE;
    SetWindowTextW(g_app.hwndEdit, L"");
    g_app.suppressChange = FALSE;
    
    g_app.filePath[0] = L'\0';
    //clear dirty flag and update title
    MarkDocumentDirty(FALSE);
    UpdateStatusBarCaret();
    UpdateWindowTitle();
    LayoutChildren(g_app.hwndMain);

    //reset tab title
     if (g_hTab) {
        TCITEM tie = {0};
        tie.mask = TCIF_TEXT;
        wchar_t title[32];
        wsprintfW(title, L"Untitled %d", g_untitledCounter++);
        tie.pszText = title;
        TabCtrl_SetItem(g_hTab, TabCtrl_GetCurSel(g_hTab), &tie);
    }
}

void OpenDocument(void) {
    if (!PromptSaveIfDirty()) {
        return;
    }
    WCHAR path[MAX_PATH] = {0};
    if (ChooseFilePath(g_app.hwndMain, path, MAX_PATH, FALSE)) {
        LoadDocument(path);
    }
}

BOOL SaveDocument(BOOL saveAs) {
    WCHAR path[MAX_PATH];
    if (!g_app.filePath[0] || saveAs) {
        path[0] = L'\0';
        if (!ChooseFilePath(g_app.hwndMain, path, MAX_PATH, TRUE)) {
            return FALSE;
        }
        SetCurrentFilePath(path);
        UpdateCurrentTabTitle(path);

    }

    int len = GetWindowTextLengthW(g_app.hwndEdit);
    WCHAR *text = (WCHAR *)LocalAlloc(LPTR, sizeof(WCHAR) * (len + 1));
    if (!text) {
        MessageBoxW(g_app.hwndMain, L"Out of memory while saving.", APP_NAME, MB_ICONERROR);
        return FALSE;
    }
    GetWindowTextW(g_app.hwndEdit, text, len + 1);

    int utf8Len = WideCharToMultiByte(CP_UTF8, 0, text, len, NULL, 0, NULL, NULL);
    BYTE *buffer = (BYTE *)LocalAlloc(LPTR, utf8Len + 3);
    if (!buffer) {
        LocalFree(text);
        MessageBoxW(g_app.hwndMain, L"Out of memory while saving.", APP_NAME, MB_ICONERROR);
        return FALSE;
    }

    buffer[0] = 0xEF;
    buffer[1] = 0xBB;
    buffer[2] = 0xBF;
    WideCharToMultiByte(CP_UTF8, 0, text, len, (LPSTR)(buffer + 3), utf8Len, NULL, NULL);

    BOOL ok = WriteBufferToFile(g_app.filePath, buffer, utf8Len + 3);
    LocalFree(buffer);
    LocalFree(text);

    if (ok) {
        MarkDocumentDirty(FALSE);
        UpdateCurrentTabTitle(g_app.filePath);
        LayoutChildren(g_app.hwndMain);
    }
    return ok;
    

}

BOOL LoadDocument(LPCWSTR path) {
    BYTE *data = NULL;
    DWORD size = 0;
    if (!ReadFileToBuffer(path, &data, &size)) {
        return FALSE;
    }

    WCHAR *text = NULL;
    if (!DecodeBufferToWide(data, size, &text)) {
        LocalFree(data);
        MessageBoxW(g_app.hwndMain, L"Unable to decode file text.", APP_NAME, MB_ICONERROR);
        return FALSE;
    }
    LocalFree(data);

    g_app.suppressChange = TRUE;
    SetWindowTextW(g_app.hwndEdit, text);
    SendMessageW(g_app.hwndEdit, EM_SETSEL, 0, 0);
    g_app.suppressChange = FALSE;
    LocalFree(text);

    SetCurrentFilePath(path);
    UpdateCurrentTabTitle(path);
    MarkDocumentDirty(FALSE);
    UpdateStatusBarCaret();
    LayoutChildren(g_app.hwndMain);
    return TRUE;
}

static BOOL ChooseFilePath(HWND owner, LPWSTR buffer, DWORD length, BOOL saveDialog) {
    OPENFILENAMEW ofn = {0};
    WCHAR filter[] = L"Text Documents (*.txt)\0*.txt\0All Files (*.*)\0*.*\0\0";
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = owner;
    ofn.hInstance = g_app.hInst;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = buffer;
    ofn.nMaxFile = length;
    ofn.Flags = OFN_HIDEREADONLY | OFN_EXPLORER | OFN_NOCHANGEDIR;

    if (saveDialog) {
        ofn.lpstrDefExt = L"txt";
        if (!GetSaveFileNameW(&ofn)) {
            return FALSE;
        }
    } else {
        if (!GetOpenFileNameW(&ofn)) {
            return FALSE;
        }
    }
    return TRUE;
}

static BOOL WriteBufferToFile(LPCWSTR path, const BYTE *data, DWORD size) {
   HANDLE hFile = CreateFileW(
    path,
    GENERIC_WRITE,
    FILE_SHARE_READ | FILE_SHARE_WRITE,
    NULL,
    CREATE_ALWAYS,
    FILE_ATTRIBUTE_NORMAL,
    NULL
);
    if (hFile == INVALID_HANDLE_VALUE) {
        MessageBoxW(g_app.hwndMain, L"Unable to write file.", APP_NAME, MB_ICONERROR);
        return FALSE;
    }

    DWORD written = 0;
    BOOL ok = WriteFile(hFile, data, size, &written, NULL);
    CloseHandle(hFile);

    if (!ok || written != size) {
        MessageBoxW(g_app.hwndMain, L"Failed to save file completely.", APP_NAME, MB_ICONERROR);
        return FALSE;
    }
    return TRUE;
}

static BOOL ReadFileToBuffer(LPCWSTR path, BYTE **data, DWORD *size) {
    HANDLE hFile = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        MessageBoxW(g_app.hwndMain, L"Unable to open file.", APP_NAME, MB_ICONERROR);
        return FALSE;
    }

    LARGE_INTEGER li = {0};
    if (!GetFileSizeEx(hFile, &li) || li.QuadPart > 50 * 1024 * 1024) {
        MessageBoxW(g_app.hwndMain, L"File is too large.", APP_NAME, MB_ICONERROR);
        CloseHandle(hFile);
        return FALSE;
    }

    DWORD length = (DWORD)li.QuadPart;
    BYTE *buffer = (BYTE *)LocalAlloc(LPTR, length + 2);
    if (!buffer) {
        CloseHandle(hFile);
        MessageBoxW(g_app.hwndMain, L"Out of memory loading file.", APP_NAME, MB_ICONERROR);
        return FALSE;
    }

    DWORD read = 0;
    BOOL ok = ReadFile(hFile, buffer, length, &read, NULL);
    CloseHandle(hFile);

    if (!ok || read != length) {
        LocalFree(buffer);
        MessageBoxW(g_app.hwndMain, L"Failed to read file.", APP_NAME, MB_ICONERROR);
        return FALSE;
    }

    *data = buffer;
    *size = length;
    return TRUE;
}

static BOOL DecodeBufferToWide(const BYTE *data, DWORD size, WCHAR **outText) {
    if (size == 0) {
        *outText = (WCHAR *)LocalAlloc(LPTR, sizeof(WCHAR));
        return *outText != NULL;
    }

    UINT codepage = CP_UTF8;
    DWORD skip = 0;

    if (size >= 2 && data[0] == 0xFF && data[1] == 0xFE) {
        int wcharCount = (size - 2) / sizeof(WCHAR);
        WCHAR *text = (WCHAR *)LocalAlloc(LPTR, (wcharCount + 1) * sizeof(WCHAR));
        if (!text) {
            return FALSE;
        }
        CopyMemory(text, data + 2, wcharCount * sizeof(WCHAR));
        text[wcharCount] = L'\0';
        *outText = text;
        return TRUE;
    }

    if (size >= 3 && data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF) {
        codepage = CP_UTF8;
        skip = 3;
    }

    int needed = MultiByteToWideChar(codepage, 0, (LPCSTR)(data + skip), size - skip, NULL, 0);
    if (needed == 0) {
        codepage = CP_ACP;
        needed = MultiByteToWideChar(codepage, 0, (LPCSTR)data, size, NULL, 0);
    }

    if (needed == 0) {
        return FALSE;
    }

    WCHAR *text = (WCHAR *)LocalAlloc(LPTR, sizeof(WCHAR) * (needed + 1));
    if (!text) {
        return FALSE;
    }

    MultiByteToWideChar(codepage, 0, (LPCSTR)(data + skip), size - skip, text, needed);
    text[needed] = L'\0';
    *outText = text;
    return TRUE;
}
