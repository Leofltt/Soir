#pragma once
#include "session.h"
void handleInputQuitView(SessionContext *ctx, u32 kDown, bool *should_break_loop);
void handleInputSaveSubMenu(SessionContext *ctx, u32 kDown);
void handleInputLoadSubMenu(SessionContext *ctx, u32 kDown);