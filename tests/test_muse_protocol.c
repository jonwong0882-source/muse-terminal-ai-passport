#include "muse_protocol.h"
#include "muse_state.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
int main(void) {
    muse_parser_t p={0};muse_frame_t f;uint8_t wire[MUSE_MAX_FRAME];
    size_t n=muse_encode(wire,M_TRANSCRIPT,0x12345678,"\xe4\xb8\xad\xe6\x96\x87",6);
    assert(n==22);assert((muse_crc32((const uint8_t *)"123456789",9,0xffffffffu)^0xffffffffu)==0xcbf43926u);
    const char *noise="ESP-ROM boot log\r\nMUMUS";
    for(size_t i=0;i<strlen(noise);i++)assert(!muse_feed(&p,noise[i],&f));
    for(size_t i=0;i<n;i++)assert(muse_feed(&p,wire[i],&f)==(i==n-1));
    assert(f.type==M_TRANSCRIPT&&f.session==0x12345678&&f.length==6);
    wire[20]^=1;for(size_t i=0;i<n;i++)assert(!muse_feed(&p,wire[i],&f));
    wire[20]^=1;for(size_t i=0;i<n;i++)muse_feed(&p,wire[i],&f);
    assert(f.session==0x12345678);
    assert(muse_encode(wire,1,1,wire,2049)==0);
    char text[5];assert(muse_utf8_copy(text,sizeof(text),(uint8_t *)"\xe4\xb8\xad\xe6\x96\x87",6)==3);assert(!strcmp(text,"\xe4\xb8\xad"));
    assert(muse_utf8_copy(text,sizeof(text),(uint8_t *)"\xff\xc0x",3)==3);assert(!strcmp(text,"??x"));
    assert(!muse_accepts_reply(S_RECORDING));assert(!muse_can_record(S_WAITING));assert(muse_can_record(S_REPLY));
    puts("Muse framing, corruption recovery, UTF-8 and state guard tests: PASS");
}
