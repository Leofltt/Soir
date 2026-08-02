#ifndef CONTROLLER_PROJECT_LOAD_H
#define CONTROLLER_PROJECT_LOAD_H

#include "session.h"
#include "project.h"

extern char g_project_names[MAX_PROJECTS][MAX_PROJECT_NAME_LEN];
extern int g_project_count;

void enterProjectLoadView(SessionContext *ctx, int mode);
void handleInputProjectLoad(SessionContext *ctx, u32 kDown, int mode);

#endif // CONTROLLER_PROJECT_LOAD_H
