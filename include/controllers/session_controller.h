#ifndef SESSION_CONTROLLER_H
#define SESSION_CONTROLLER_H

#include "session.h" // Include the data structures
#include <3ds.h>
#include <stdbool.h>

bool handle_continuous_press(u32 kDown, u32 kHeld, u64 now, u32 key, u64 *timer,
                             const u64 delay_initial, const u64 delay_repeat);

void sessionControllerHandleInput(SessionContext *ctx, u32 kDown, u32 kHeld, u64 now,
                                  bool *should_break_loop);

#endif // SESSION_CONTROLLER_H