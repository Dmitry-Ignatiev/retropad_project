#pragma once

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <wchar.h>
#include <wctype.h>

#include "resource.h"

#define APP_NAME L"retropad"

typedef struct AppState {
    HINSTANCE hInst;
    HWND hwndMain;
    HWND hwndEdit;
    HWND hwndStatus;
    HFONT hFont;
    LOGFONTW logFont;
    WCHAR filePath[MAX_PATH];
    BOOL dirty;
    BOOL wordWrap;
    BOOL showStatus;
    BOOL suppressChange;
    UINT findMsgId;
    FINDREPLACEW find;
    WCHAR findText[128];
    WCHAR replaceText[128];
} AppState;

extern AppState g_app;
extern HWND g_hTab;
BOOL InitApplication(HINSTANCE hInstance, int nCmdShow);
void UpdateWindowTitle(void);
void MarkDocumentDirty(BOOL dirty);
void UpdateMenuStates(HMENU hMenu);
void UpdateStatusBarCaret(void);
void ToggleWordWrap(void);
void ToggleStatusBar(void);
void ResizeClientArea(RECT clientRect);
void ApplyCurrentFont(void);
void EnsureStatusBar(void);
void DestroyStatusBar(void);
void RecreateEditControl(void);
void SetCurrentFilePath(LPCWSTR path);
BOOL PromptSaveIfDirty(void);
void NewDocument(void);
void OpenDocument(void);
BOOL SaveDocument(BOOL saveAs);
BOOL LoadDocument(LPCWSTR path);
void InsertTimeDate(void);
void ShowFindDialog(BOOL replaceMode);
void HandleFindReplace(LPARAM lParam);
void FindNext(void);
void ShowGotoDialog(HWND owner);
void ShowFontDialog(HWND owner);
void ShowAboutDialog(HWND owner);
void ShowPageSetupDialog(HWND owner);
void PrintDocument(HWND owner);
void UpdateCurrentTabTitle(const wchar_t *filePath);
HWND CreateMainTabControl(HWND hwndParent, HINSTANCE hInst);