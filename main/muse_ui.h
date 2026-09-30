#pragma once
#include "muse_state.h"
#include <stdbool.h>
bool muse_ui_init(void);
void muse_ui_update(muse_state_t state,const char *text,int battery,int seconds);
void muse_ui_scroll(int direction);
