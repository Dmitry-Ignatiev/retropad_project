#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include "app.h"

// Store page settings (margins, paper size) globally so they persist
static PAGESETUPDLGW g_pagesetup = {0};
static BOOL g_pagesetupInit = FALSE;

void InitPageSetup(HWND owner) {
    if (!g_pagesetupInit) {
        g_pagesetup.lStructSize = sizeof(g_pagesetup);
        g_pagesetup.hwndOwner = owner;
        // Default margins (in 100ths of millimeters)
        g_pagesetup.rtMargin.left = 2000;
        g_pagesetup.rtMargin.top = 2000;
        g_pagesetup.rtMargin.right = 2000;
        g_pagesetup.rtMargin.bottom = 2000;
        g_pagesetup.Flags = PSD_MARGINS | PSD_RETURNDEFAULT; 
        PageSetupDlgW(&g_pagesetup); // Get system defaults
        g_pagesetup.Flags = PSD_MARGINS; // Ready for user
        g_pagesetupInit = TRUE;
    }
}

void ShowPageSetupDialog(HWND owner) {
    InitPageSetup(owner);
    g_pagesetup.hwndOwner = owner;
    PageSetupDlgW(&g_pagesetup);
}

// Helper to print a page of text
static int PrintPage(HDC hdc, const WCHAR *text, int len, int startChar, RECT *rcPage, int lineHeight) {
    int y = rcPage->top;
    int charsProcessed = 0;
    int maxLines = (rcPage->bottom - rcPage->top) / lineHeight;
    int linesPrinted = 0;
    
    // Simple logic: We use DrawText with DT_WORDBREAK to handle wrapping 
    // strictly for printing. This is a basic implementation.
    
    // Note: A true robust clone calculates exact line breaks matching the screen.
    // Here we trust the printer DC to wrap text.
    
    // We need to loop until we fill the page or run out of text
    // This is "Hard Mode" programming. For this simple clone, we will use a simplified approach:
    // We just dump text. (A robust solution requires implementing a full layout engine).
    
    // Simplified: Print line by line from the buffer
    // To do this properly requires parsing \r\n. 
    
    // Alternative: Use the Edit Control to format for us? 
    // Standard Edit controls don't support EM_FORMATRANGE (RichEdit does).
    
    // LET'S GO SIMPLE: TextOut line by line.
    
    // 1. Get the font
    HFONT hFont = (HFONT)GetCurrentObject(hdc, OBJ_FONT);
    SelectObject(hdc, hFont);

    // 2. Iterate lines in the text buffer
    const WCHAR *cursor = text + startChar;
    int remaining = len - startChar;
    
    while (remaining > 0 && linesPrinted < maxLines) {
        // Find next line break
        int lineLen = 0;
        while (lineLen < remaining && cursor[lineLen] != L'\n' && cursor[lineLen] != L'\r') {
            lineLen++;
        }
        
        TextOutW(hdc, rcPage->left, y, cursor, lineLen);
        
        y += lineHeight;
        linesPrinted++;
        
        // Skip the newline characters
        if (lineLen < remaining) {
            if (cursor[lineLen] == L'\r' && (lineLen + 1 < remaining) && cursor[lineLen+1] == L'\n') {
                lineLen += 2;
            } else {
                lineLen += 1;
            }
        }
        
        cursor += lineLen;
        startChar += lineLen;
        remaining = len - startChar;
    }
    
    return startChar;
}

void PrintDocument(HWND owner) {
    InitPageSetup(owner);
    
    PRINTDLGW pd = {0};
    pd.lStructSize = sizeof(pd);
    pd.hwndOwner = owner;
    pd.hDevMode = g_pagesetup.hDevMode;
    pd.hDevNames = g_pagesetup.hDevNames;
    pd.Flags = PD_RETURNDC | PD_NOSELECTION | PD_NOPAGENUMS;

    if (!PrintDlgW(&pd)) {
        return;
    }

    // Get text from editor
    int len = GetWindowTextLengthW(g_app.hwndEdit);
    WCHAR *buffer = (WCHAR *)LocalAlloc(LPTR, sizeof(WCHAR) * (len + 1));
    if (!buffer) return;
    GetWindowTextW(g_app.hwndEdit, buffer, len + 1);

    // Initialize Printer DC
    HDC hdc = pd.hDC;
    DOCINFOW di = {0};
    di.cbSize = sizeof(di);
    di.lpszDocName = L"Retropad Document";
    
    if (StartDocW(hdc, &di) > 0) {
        // Set Font (Scale height for printer DPI)
        int logY = GetDeviceCaps(hdc, LOGPIXELSY);
        int fontHeight = -MulDiv(10, logY, 72); // 10pt font standard
        HFONT hPrintFont = CreateFontW(fontHeight, 0,0,0, FW_NORMAL, 0,0,0, 
                                       DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                       DEFAULT_QUALITY, DEFAULT_PITCH, L"Courier New");
        HFONT hOldFont = (HFONT)SelectObject(hdc, hPrintFont);

        // Get Line Height metrics
        TEXTMETRICW tm;
        GetTextMetricsW(hdc, &tm);
        int lineHeight = tm.tmHeight + tm.tmExternalLeading;

        // Calculate printable area based on margins (converted from 100th mm to Device Pixels)
        // 1 inch = 25.4 mm = 2540 units
        int horzRes = GetDeviceCaps(hdc, HORZRES);
        int vertRes = GetDeviceCaps(hdc, VERTRES);
        int dpiX = GetDeviceCaps(hdc, LOGPIXELSX);
        int dpiY = GetDeviceCaps(hdc, LOGPIXELSY);

        // Convert margins to pixels
        RECT rcPage;
        rcPage.left = (g_pagesetup.rtMargin.left * dpiX) / 2540;
        rcPage.top = (g_pagesetup.rtMargin.top * dpiY) / 2540;
        rcPage.right = horzRes - ((g_pagesetup.rtMargin.right * dpiX) / 2540);
        rcPage.bottom = vertRes - ((g_pagesetup.rtMargin.bottom * dpiY) / 2540);

        int processed = 0;
        while (processed < len) {
            StartPage(hdc);
            processed = PrintPage(hdc, buffer, len, processed, &rcPage, lineHeight);
            EndPage(hdc);
        }

        EndDoc(hdc);
        SelectObject(hdc, hOldFont);
        DeleteObject(hPrintFont);
    }

    DeleteDC(hdc);
    LocalFree(buffer);
}