#include "views.h"

#include "audio_utils.h"
#include <string.h>
#include "engine_constants.h"
#include "filters.h"
#include "polybleposc.h"
#include "samplers.h"
#include "sample.h"
#include "sample_bank.h"
#include "sample_browser.h"
#include "session_controller.h"
#include "ui_constants.h"
#include <stdio.h>

C2D_Font    font_angular;
C2D_Font    font_heavy;
C2D_TextBuf text_buf;
C2D_Text    text_obj;

void initViews() {
    font_angular = C2D_FontLoad(FONTPATH_F500ANGULAR);
    font_heavy   = C2D_FontLoad(FONTPATH_2197HEAVY);
    text_buf     = C2D_TextBufNew(128);
}

void deinitViews() {
    C2D_FontFree(font_angular);
    C2D_FontFree(font_heavy);
    C2D_TextBufDelete(text_buf);
}
