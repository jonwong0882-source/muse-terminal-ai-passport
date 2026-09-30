#include "muse_avatar.h"
#include <stddef.h>

extern const uint8_t s_avatar_start[] asm("_binary_muse_avatar_start");
extern const uint8_t s_avatar_end[] asm("_binary_muse_avatar_end");

const uint8_t *muse_avatar_data(void) {
    return (size_t)(s_avatar_end-s_avatar_start)==MUSE_AVATAR_BYTES?s_avatar_start:NULL;
}
