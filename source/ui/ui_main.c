#include "ui/ui.h"

#include "audio_utils.h"
#include <string.h>
#include "engine_constants.h"
#include "filters.h"
#include "polybleposc.h"
#include "samplers.h"
#include "sample.h"
#include "sample_bank.h"
#include "sample_browser.h"
#include "session.h"
#include "ui_constants.h"
#include <stdio.h>

C2D_Font    font_angular;
C2D_Font    font_heavy;
C2D_TextBuf text_buf;
C2D_Text    text_obj;

bool initViews() {
    font_angular = C2D_FontLoad(FONTPATH_F500ANGULAR);
    if (!font_angular)
        return false;

    font_heavy = C2D_FontLoad(FONTPATH_2197HEAVY);
    if (!font_heavy) {
        C2D_FontFree(font_angular);
        return false;
    }

    text_buf = C2D_TextBufNew(128);
    if (!text_buf) {
        C2D_FontFree(font_angular);
        C2D_FontFree(font_heavy);
        return false;
    }

    return true;
}

void deinitViews() {
    C2D_FontFree(font_angular);
    C2D_FontFree(font_heavy);
    C2D_TextBufDelete(text_buf);
}
