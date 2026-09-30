#include "muse_protocol.h"
#include <string.h>
static uint32_t le32(const uint8_t *p) { return (uint32_t)p[0] | (uint32_t)p[1]<<8 | (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24; }
static void put32(uint8_t *p,uint32_t v) { for(int i=0;i<4;i++)p[i]=(uint8_t)(v>>(8*i)); }
uint32_t muse_crc32(const uint8_t *data,size_t n,uint32_t crc) {
    for(size_t i=0;i<n;i++) { crc^=data[i]; for(int j=0;j<8;j++)crc=(crc>>1)^((crc&1)?0xedb88320u:0); }
    return crc;
}
size_t muse_encode(uint8_t *out,uint8_t type,uint32_t session,const void *data,size_t n) {
    if(n>MUSE_MAX_PAYLOAD || (!data&&n))return 0;
    memcpy(out,"MUS1",4); out[4]=type; out[5]=0; out[6]=n&255; out[7]=n>>8; put32(out+8,session);
    if(n)memcpy(out+16,data,n);
    uint32_t c=muse_crc32(out+4,8,0xffffffffu); c=muse_crc32(out+16,n,c)^0xffffffffu; put32(out+12,c);
    return 16+n;
}
bool muse_feed(muse_parser_t *p,uint8_t b,muse_frame_t *out) {
    if(p->used>=sizeof(p->bytes))p->used=0;
    p->bytes[p->used++]=b;
    while(p->used && memcmp(p->bytes,"MUS1",p->used<4?p->used:4)) { memmove(p->bytes,p->bytes+1,--p->used); }
    if(p->used<16)return false;
    size_t n=p->bytes[6]|(size_t)p->bytes[7]<<8;
    if(n>MUSE_MAX_PAYLOAD || p->bytes[5]!=0) { p->used=0; return false; }
    if(p->used<16+n)return false;
    uint32_t c=muse_crc32(p->bytes+4,8,0xffffffffu); c=muse_crc32(p->bytes+16,n,c)^0xffffffffu;
    p->used=0;
    if(c!=le32(p->bytes+12))return false;
    out->type=p->bytes[4]; out->session=le32(p->bytes+8); out->length=(uint16_t)n;
    if(n)memcpy(out->data,p->bytes+16,n);
    return true;
}
size_t muse_utf8_copy(char *out,size_t capacity,const uint8_t *src,size_t n) {
    size_t i=0,w=0;
    if(!capacity)return 0;
    while(i<n && w+1<capacity) {
        uint8_t a=src[i]; size_t k=a<0x80?1:(a>=0xc2&&a<=0xdf?2:(a>=0xe0&&a<=0xef?3:(a>=0xf0&&a<=0xf4?4:0)));
        bool ok=k && i+k<=n; uint32_t cp=k==1?a:(a&((1u<<(7-k))-1));
        for(size_t j=1;ok&&j<k;j++) { if((src[i+j]&0xc0)!=0x80)ok=false; else cp=(cp<<6)|(src[i+j]&63); }
        if(ok && ((k==2&&cp<128)||(k==3&&cp<2048)||(k==4&&cp<65536)||(cp>=0xd800&&cp<=0xdfff)||cp>0x10ffff||cp==0))ok=false;
        if(!ok) { out[w++]='?'; i++; continue; }
        if(w+k>=capacity)break;
        memcpy(out+w,src+i,k); w+=k;i+=k;
    }
    out[w]=0;return w;
}
