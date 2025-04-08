#ifndef UI_CONSTANTS_H
#define UI_CONSTANTS_H

#include <citro2d.h>

// fonts
#define FONTPATH_2197BLOCK "romfs:/2197Block.bcfnt"
#define FONTPATH_2197HEAVY "romfs:/2197Heavy.bcfnt"
#define FONTPATH_F500ANGULAR "romfs:/F500Angular.bcfnt"

// colors
#define CLR_BLACK C2D_Color32(0, 0, 0, 0xFF)
#define CLR_WHITE C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF)
#define CLR_RED C2D_Color32(0xFF, 0, 0, 0xFF)
#define CLR_GREEN C2D_Color32(0, 0xFF, 0, 0xFF)
#define CLR_BLUE C2D_Color32(0, 0, 0xFF, 0xFF)
#define CLR_LIGHT_GRAY C2D_Color32(0x80, 0x80, 0x80, 0xFF)
#define CLR_DARK_GRAY C2D_Color32(0x40, 0x40, 0x40, 0xFF)

// screens dimensions
#define TOP_SCREEN_WIDTH 400
#define BOTTOM_SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240 // same for both screens

// main view components dimensions
#define HOME_TOOLBAR_WIDTH 100
#define HOME_TRACKS_WIDTH 300
#define HOME_TRACKS_HEIGHT 15
#define HOME_TRACKS_SPACER_W 6
#define HOME_TRACKS_HEADER_W 18

#endif // UI_CONSTANTS_H