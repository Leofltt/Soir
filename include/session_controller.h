#ifndef SESSION_CONTROLLER_H
#define SESSION_CONTROLLER_H

typedef enum { VIEW_MAIN = 0, VIEW_SETTINGS = 1, VIEW_ABOUT = 2, VIEW_QUIT = 3 } View;

typedef struct {
    View main_screen_view;
    View touch_screen_view;

} Session;

#endif