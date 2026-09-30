#include "lvgl.h"
#include "muse_ui.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
LV_FONT_DECLARE(muse_font_16);
static uint8_t framebuffer[240*320*4];
static uint8_t idle_before_animation[sizeof(framebuffer)];
static uint8_t draw_buffer[240*40*4];
bool bsp_lvgl_lock(int timeout) {(void)timeout;return true;}
void bsp_lvgl_unlock(void) {}
static void flush(lv_display_t *display,const lv_area_t *area,uint8_t *px) {
    for(int y=area->y1;y<=area->y2;y++) {
        size_t size=(area->x2-area->x1+1)*4;
        memcpy(framebuffer+(y*240+area->x1)*4,px,size);px+=size;
    }
    lv_display_flush_ready(display);
}
int main(int argc,char **argv) {
    const char *out=argc>1?argv[1]:".";
    lv_init();lv_display_t *display=lv_display_create(240,320);
    lv_display_set_color_format(display,LV_COLOR_FORMAT_XRGB8888);
    lv_display_set_buffers(display,draw_buffer,NULL,sizeof(draw_buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display,flush);
    if(!muse_ui_init()) {fputs("Muse UI initialization failed\n",stderr);return 1;}
    FILE *glyphs=argc>2?fopen(argv[2],"r"):NULL;unsigned cp;size_t checked=0;
    if(argc>2&&!glyphs) {perror(argv[2]);return 1;}
    if(glyphs) {while(fscanf(glyphs,"%x",&cp)==1) {lv_font_glyph_dsc_t d={0};if(!lv_font_get_glyph_dsc(&muse_font_16,&d,cp,0)||d.is_placeholder) {fprintf(stderr,"Missing glyph U+%04X\n",cp);return 1;}checked++;}fclose(glyphs);}
    lv_font_glyph_dsc_t missing={0};if(lv_font_get_glyph_dsc(&muse_font_16,&missing,0x1f680,0)&&!missing.is_placeholder) {fputs("Unexpected rocket glyph\n",stderr);return 1;}
    const char *texts[]={"首次使用请通过 USB 配网，然后无线连接 Mac。","按确定键开始说话。识别后可确认发送。","请说话。再次按确定键结束录音，最长三十秒。","Mac 正在将语音转换成文字。","请告诉我今天需要完成的任务。","指令已交给 Mac。请等待 Muse 回复。","你好！简体中文 / 繁體中文，ABC 123。\n表情测试 🚀","等待超时。请在 Muse 查看状态，不会自动重复发送。"};
    for(int s=0;s<8;s++) {
        muse_ui_update((muse_state_t)s,texts[s],s==0?-1:82,12);lv_tick_inc(100);lv_timer_handler();lv_refr_now(display);
        char path[512];snprintf(path,sizeof(path),"%s/state-%d.ppm",out,s);FILE *f=fopen(path,"wb");if(!f) {perror(path);return 1;}fprintf(f,"P6\n240 320\n255\n");
        for(int i=0;i<240*320;i++){fputc(framebuffer[i*4+2],f);fputc(framebuffer[i*4+1],f);fputc(framebuffer[i*4],f);}fclose(f);
    }
    muse_ui_update(S_IDLE,"按确定键开始说话。",82,0);
    lv_refr_now(display);
    memcpy(idle_before_animation,framebuffer,sizeof(framebuffer));
    lv_tick_inc(250);lv_timer_handler();lv_refr_now(display);
    if(memcmp(idle_before_animation,framebuffer,sizeof(framebuffer))==0) {
        fputs("Idle avatar did not animate\n",stderr);return 1;
    }
    printf("Actual LVGL font descriptors: %zu glyphs verified; 8 application states and idle animation rendered\n",checked);
}
