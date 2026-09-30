#include "muse_avatar.h"
#include "avatar_path.h"
#include <stdio.h>
#include <stdlib.h>

const uint8_t *muse_avatar_data(void) {
    static uint8_t *bytes;
    if(bytes)return bytes;
    FILE *file=fopen(MUSE_AVATAR_PATH,"rb");
    if(!file)return NULL;
    bytes=malloc(MUSE_AVATAR_BYTES);
    if(!bytes) {fclose(file);return NULL;}
    size_t got=fread(bytes,1,MUSE_AVATAR_BYTES,file);
    int extra=fgetc(file);
    fclose(file);
    if(got!=MUSE_AVATAR_BYTES||extra!=EOF) {free(bytes);bytes=NULL;}
    return bytes;
}
