#ifndef SESSION_CONTROLLER_H
#define SESSION_CONTROLLER_H

typedef enum {
    VIEW_MAIN                 = 0,
    VIEW_SETTINGS             = 1,
    VIEW_ABOUT                = 2,
    VIEW_QUIT                 = 3,
    VIEW_TOUCH_SETTINGS       = 4,
    VIEW_TOUCH_CLOCK_SETTINGS = 5,
    VIEW_SAMPLE_MANAGER       = 6
} View;

typedef enum { FOCUS_TOP, FOCUS_BOTTOM } ScreenFocus;

typedef struct {
    View main_screen_view;
    View touch_screen_view;

} Session;

#endif