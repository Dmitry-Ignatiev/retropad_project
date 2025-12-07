#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned int DWORD;
typedef int LONG;

#pragma pack(push, 1)
typedef struct {
    WORD idReserved; WORD idType; WORD idCount;
} ICONDIR;

typedef struct {
    BYTE bWidth; BYTE bHeight; BYTE bColorCount; BYTE bReserved;
    WORD wPlanes; WORD wBitCount; DWORD dwBytesInRes; DWORD dwImageOffset;
} ICONDIRENTRY;

typedef struct {
    DWORD biSize; LONG biWidth; LONG biHeight; WORD biPlanes; WORD biBitCount;
    DWORD biCompression; DWORD biSizeImage; LONG biXPelsPerMeter; LONG biYPelsPerMeter;
    DWORD biClrUsed; DWORD biClrImportant;
} BITMAPINFOHEADER;
#pragma pack(pop)

// BGR Color structure (Windows uses Blue-Green-Red order)
typedef struct { BYTE b; BYTE g; BYTE r; } Pixel;

Pixel canvas[32][32]; // [Row][Col] (0,0 is Bottom-Left in BMPs usually, but we'll manage)
BYTE alphaMask[32][4]; // 1 bit per pixel, 32 pixels wide = 4 bytes. 0=Opaque, 1=Transp

void setPixel(int x, int y, int r, int g, int b) {
    // BMPs store rows bottom-to-top. Let's flip Y so 0,0 is Top-Left for ease of drawing.
    int flipY = 31 - y; 
    if (x < 0 || x >= 32 || y < 0 || y >= 32) return;
    
    canvas[flipY][x].r = (BYTE)r;
    canvas[flipY][x].g = (BYTE)g;
    canvas[flipY][x].b = (BYTE)b;
    
    // Mark as Opaque (0) in the AND mask
    // x / 8 gives the byte index. 
    // 7 - (x % 8) gives the bit index (MSB first).
    alphaMask[flipY][x / 8] &= ~(1 << (7 - (x % 8)));
}

int main() {
    FILE *f = fopen("res/retropad.ico", "wb");
    if (!f) { printf("Error: Create 'res' folder first.\n"); return 1; }

    // 1. Initialize Canvas
    // Default to Black (0,0,0) and Fully Transparent (Mask = 0xFF)
    memset(canvas, 0, sizeof(canvas));
    memset(alphaMask, 0xFF, sizeof(alphaMask));

    // 2. Draw the Icon (Pixel Art Logic)
    // Canvas size is 0-31.
    
    // Draw Shadow (Offset right/down)
    for(int y=4; y<29; y++) 
        for(int x=4; x<29; x++) setPixel(x, y, 80, 80, 80); // Dark Grey Shadow

    // Draw Main Page Background (White)
    for(int y=2; y<28; y++) 
        for(int x=3; x<27; x++) setPixel(x, y, 255, 255, 255);

    // Draw Header (Navy Blue)
    for(int y=2; y<8; y++) 
        for(int x=3; x<27; x++) setPixel(x, y, 0, 51, 153); // Deep Blue

    // Draw "Notepad" spiral binding holes (Left side)
    for(int y=4; y<26; y+=4) {
        setPixel(3, y, 200, 200, 200);
        setPixel(3, y+1, 200, 200, 200);
    }

    // Draw Text Lines (Grey)
    for(int y=11; y<26; y+=3) {
        for(int x=6; x<24; x++) {
            setPixel(x, y, 180, 180, 180);
        }
    }

    // Draw Border (Black Outline)
    // Left/Right
    for(int y=2; y<28; y++) { setPixel(3, y, 0,0,0); setPixel(27, y, 0,0,0); }
    // Top/Bottom
    for(int x=3; x<28; x++) { setPixel(x, 2, 0,0,0); setPixel(x, 28, 0,0,0); }
    // Header divider
    for(int x=3; x<27; x++) setPixel(x, 8, 0,0,0);


    // 3. Write ICO Headers
    ICONDIR dir = {0, 1, 1};
    fwrite(&dir, 1, sizeof(dir), f);

    int colorSize = sizeof(canvas); // 32*32*3 = 3072 bytes
    int maskSize = sizeof(alphaMask); // 32*4 = 128 bytes
    int imageTotalSize = 40 + colorSize + maskSize; // Header + Pixels + Mask
    
    ICONDIRENTRY entry;
    entry.bWidth = 32; entry.bHeight = 32; entry.bColorCount = 0; entry.bReserved = 0;
    entry.wPlanes = 1; entry.wBitCount = 24; // 24-bit TrueColor
    entry.dwBytesInRes = imageTotalSize;
    entry.dwImageOffset = sizeof(ICONDIR) + sizeof(ICONDIRENTRY);
    fwrite(&entry, 1, sizeof(entry), f);

    // 4. Write Bitmap Info Header
    BITMAPINFOHEADER bmi = {0};
    bmi.biSize = 40; bmi.biWidth = 32; bmi.biHeight = 64; // Height doubled for mask
    bmi.biPlanes = 1; bmi.biBitCount = 24; bmi.biSizeImage = colorSize + maskSize;
    fwrite(&bmi, 1, sizeof(bmi), f);

    // 5. Write Pixel Data (BGR)
    fwrite(canvas, 1, sizeof(canvas), f);

    // 6. Write Mask Data (1-bit transparency)
    fwrite(alphaMask, 1, sizeof(alphaMask), f);

    fclose(f);
    printf("Generated high-quality TrueColor icon.\n");
    return 0;
}