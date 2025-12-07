#include <stdio.h>

// A 16x16 Monochrome "Retro Terminal" Icon
// It depicts a small document with a "matrix" style look.
const unsigned char icon_data[] = {
    // ICO Header (6 bytes)
    0x00, 0x00,             // Reserved
    0x01, 0x00,             // Type (1 = Icon)
    0x01, 0x00,             // Count (1 image)

    // Directory Entry (16 bytes)
    0x10,                   // Width (16)
    0x10,                   // Height (16)
    0x02,                   // Colors (2 - Monochrome)
    0x00,                   // Reserved
    0x01, 0x00,             // Planes
    0x01, 0x00,             // BPP
    0xC0, 0x00, 0x00, 0x00, // Size (192 bytes)
    0x16, 0x00, 0x00, 0x00, // Offset (22 bytes from start)

    // BMP Info Header (40 bytes)
    0x28, 0x00, 0x00, 0x00, // Header Size
    0x10, 0x00, 0x00, 0x00, // Width (16)
    0x20, 0x00, 0x00, 0x00, // Height (32 - XOR + AND masks)
    0x01, 0x00,             // Planes
    0x01, 0x00,             // BPP
    0x00, 0x00, 0x00, 0x00, // Compression
    0x00, 0x00, 0x00, 0x00, // Image Size
    0x00, 0x00, 0x00, 0x00, // X Pixels/meter
    0x00, 0x00, 0x00, 0x00, // Y Pixels/meter
    0x00, 0x00, 0x00, 0x00, // Colors Used
    0x00, 0x00, 0x00, 0x00, // Important Colors

    // Color Table (8 bytes) - Black and Retro Green
    0x00, 0x00, 0x00, 0x00, // Color 0: Black
    0x00, 0xFF, 0x00, 0x00, // Color 1: Bright Green

    // XOR MASK (32 bytes) - The Shape (1 = Green, 0 = Black)
    0xFF, 0xFF, // [##############]
    0x80, 0x01, // [#            #]
    0x80, 0x01, // [#            #]
    0x8F, 0xF1, // [#   ####     #]
    0x88, 0x11, // [#   #  #     #]
    0x88, 0x11, // [#   #  #     #]
    0x8F, 0xF1, // [#   ####     #]
    0x88, 0x91, // [#   #  #     #]
    0x88, 0x91, // [#   #  #     #]
    0x80, 0x01, // [#            #]
    0x80, 0x01, // [#            #]
    0xBF, 0xFD, // [# ########## #]
    0x80, 0x01, // [#            #]
    0x80, 0x01, // [#            #]
    0xFF, 0xFF, // [##############]
    0x00, 0x00, // [              ]

    // AND MASK (32 bytes) - Transparency (0 = Opaque, 1 = Transparent)
    0x00, 0x00, // Full Opaque square
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
};

int main() {
    // Ensure the res folder exists (or user creates it manually)
    FILE *f = fopen("res/retropad.ico", "wb");
    if (!f) {
        printf("Error: Could not create res/retropad.ico.\n");
        printf("Make sure the 'res' folder exists!\n");
        return 1;
    }
    fwrite(icon_data, 1, sizeof(icon_data), f);
    fclose(f);
    printf("Success! Generated res/retropad.ico\n");
    return 0;
}