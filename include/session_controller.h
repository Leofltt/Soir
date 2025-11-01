#ifndef SESSION_CONTROLLER_H
#define SESSION_CONTROLLER_H

typedef enum {
    VIEW_MAIN               = 0,
    VIEW_SETTINGS           = 1,
    VIEW_ABOUT              = 2,
    VIEW_QUIT               = 3,
    VIEW_STEP_SETTINGS_EDIT = 4,
} TopScreenView;

typedef enum {
    VIEW_TOUCH_SETTINGS       = 0,
    VIEW_TOUCH_CLOCK_SETTINGS = 1,
    VIEW_SAMPLE_MANAGER       = 2,
    VIEW_STEP_SETTINGS        = 3
} BottomScreenView;

typedef enum { FOCUS_TOP, FOCUS_BOTTOM } ScreenFocus;

typedef struct {
    TopScreenView    main_screen_view;
    BottomScreenView touch_screen_view;

} Session;

#endif