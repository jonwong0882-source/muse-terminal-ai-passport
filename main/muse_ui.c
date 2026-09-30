#include "muse_ui.h"
#include "muse_avatar.h"
#include "bsp_display.h"
#include "lvgl.h"
#include <stdio.h>
#include <string.h>

LV_FONT_DECLARE(muse_font_16);

static lv_obj_t *s_screen,*s_title,*s_full_avatar,*s_compact_avatar,*s_status,*s_body,*s_footer;
static lv_obj_t *s_battery,*s_dot,*s_meter,*s_panel,*s_record_avatar,*s_record_pill,*s_record_clock;
static lv_image_dsc_t s_frames[MUSE_AVATAR_FRAMES],s_compact,s_record;
static muse_state_t s_state=S_OFFLINE;
static unsigned s_tick;
static int s_frame=-1;

static lv_obj_t *label(lv_obj_t *parent,int x,int y,int w,const char *value,uint32_t color) {
    lv_obj_t *o=lv_label_create(parent);
    lv_obj_set_pos(o,x,y);lv_obj_set_width(o,w);
    lv_obj_set_style_text_font(o,&muse_font_16,0);
    lv_obj_set_style_text_color(o,lv_color_hex(color),0);
    lv_label_set_text(o,value);
    return o;
}

static void image_desc(lv_image_dsc_t *d,int w,int h,const uint8_t *pixels) {
    memset(d,0,sizeof(*d));
    d->header.magic=LV_IMAGE_HEADER_MAGIC;
    d->header.cf=LV_COLOR_FORMAT_RGB565;
    d->header.w=w;d->header.h=h;d->header.stride=w*2;
    d->data_size=w*h*2;d->data=pixels;
}

static bool compact_state(muse_state_t state) {
    return state==S_REVIEW||state==S_REPLY||state==S_ERROR;
}

static void avatar_tick(lv_timer_t *timer) {
    (void)timer;
    if(compact_state(s_state)||s_state==S_RECORDING)return;
    s_tick++;
    int frame=0;
    if(s_state==S_IDLE)frame=s_tick%MUSE_AVATAR_FRAMES;
    else if(s_state==S_TRANSCRIBING||s_state==S_WAITING)frame=(s_tick/2)%MUSE_AVATAR_FRAMES;
    else frame=(s_tick/4)%MUSE_AVATAR_FRAMES;
    if(frame!=s_frame) {lv_image_set_src(s_full_avatar,&s_frames[frame]);s_frame=frame;}
}

bool muse_ui_init(void) {
    const uint8_t *pixels=muse_avatar_data();
    if(!pixels||!bsp_lvgl_lock(1000))return false;
    for(int i=0;i<MUSE_AVATAR_FRAMES;i++)
        image_desc(&s_frames[i],MUSE_AVATAR_WIDTH,MUSE_AVATAR_HEIGHT,pixels+i*MUSE_AVATAR_FRAME_BYTES);
    image_desc(&s_compact,MUSE_AVATAR_COMPACT_WIDTH,MUSE_AVATAR_COMPACT_HEIGHT,
               pixels+MUSE_AVATAR_FRAMES*MUSE_AVATAR_FRAME_BYTES);
    image_desc(&s_record,MUSE_AVATAR_RECORD_WIDTH,MUSE_AVATAR_RECORD_HEIGHT,
               pixels+MUSE_AVATAR_FRAMES*MUSE_AVATAR_FRAME_BYTES+MUSE_AVATAR_COMPACT_BYTES);

    s_screen=lv_obj_create(NULL);
    lv_obj_set_style_bg_color(s_screen,lv_color_hex(0xFFFFFF),0);
    lv_obj_set_style_pad_all(s_screen,0,0);
    lv_obj_remove_flag(s_screen,LV_OBJ_FLAG_SCROLLABLE);
    s_full_avatar=lv_image_create(s_screen);
    lv_image_set_src(s_full_avatar,&s_frames[0]);lv_obj_set_pos(s_full_avatar,0,0);
    s_frame=0;
    s_compact_avatar=lv_image_create(s_screen);
    lv_image_set_src(s_compact_avatar,&s_compact);
    lv_obj_set_pos(s_compact_avatar,70,0);
    lv_obj_add_flag(s_compact_avatar,LV_OBJ_FLAG_HIDDEN);

    s_record_avatar=lv_image_create(s_screen);
    lv_image_set_src(s_record_avatar,&s_record);lv_obj_set_pos(s_record_avatar,38,42);
    lv_obj_add_flag(s_record_avatar,LV_OBJ_FLAG_HIDDEN);
    s_record_pill=lv_obj_create(s_screen);
    lv_obj_set_pos(s_record_pill,82,194);lv_obj_set_size(s_record_pill,76,29);
    lv_obj_set_style_bg_color(s_record_pill,lv_color_hex(0x35383B),0);
    lv_obj_set_style_border_width(s_record_pill,0,0);
    lv_obj_set_style_radius(s_record_pill,LV_RADIUS_CIRCLE,0);
    lv_obj_set_style_pad_all(s_record_pill,0,0);
    lv_obj_remove_flag(s_record_pill,LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *pill_text=label(s_record_pill,0,0,76,"Muse",0xFFFFFF);
    lv_obj_set_style_text_align(pill_text,LV_TEXT_ALIGN_CENTER,0);
    lv_obj_center(pill_text);
    lv_obj_add_flag(s_record_pill,LV_OBJ_FLAG_HIDDEN);
    s_record_clock=label(s_screen,20,270,200,"00 / 30 秒",0xE3DAD4);
    lv_obj_add_flag(s_record_clock,LV_OBJ_FLAG_HIDDEN);

    s_title=label(s_screen,15,9,100,"Muse",0x4A403A);
    s_battery=label(s_screen,181,9,55,"--%",0x746A64);
    s_dot=lv_obj_create(s_screen);lv_obj_set_pos(s_dot,19,241);lv_obj_set_size(s_dot,9,9);
    lv_obj_set_style_radius(s_dot,LV_RADIUS_CIRCLE,0);
    lv_obj_set_style_border_width(s_dot,0,0);
    s_status=label(s_screen,36,235,190,"等待 Mac",0x413A35);
    s_meter=lv_bar_create(s_screen);lv_obj_set_pos(s_meter,20,225);lv_obj_set_size(s_meter,200,3);
    lv_obj_set_style_bg_color(s_meter,lv_color_hex(0xE8DED5),0);
    lv_obj_set_style_bg_color(s_meter,lv_color_hex(0xC99176),LV_PART_INDICATOR);
    s_panel=lv_obj_create(s_screen);lv_obj_set_pos(s_panel,12,260);lv_obj_set_size(s_panel,216,40);
    lv_obj_set_style_bg_color(s_panel,lv_color_hex(0xF8F4EF),0);
    lv_obj_set_style_border_width(s_panel,0,0);
    lv_obj_set_style_radius(s_panel,12,0);
    lv_obj_set_style_pad_all(s_panel,0,0);
    lv_obj_set_scrollbar_mode(s_panel,LV_SCROLLBAR_MODE_OFF);
    s_body=label(s_panel,8,3,200,"连接 Mac 后，按确定键开始说话。",0x413A35);
    s_footer=label(s_screen,14,302,216,"确定开始说话",0x776D67);
    lv_obj_set_style_text_line_space(s_footer,-6,0);
    lv_screen_load(s_screen);
    lv_timer_create(avatar_tick,250,NULL);
    bsp_lvgl_unlock();
    return true;
}

/* Network text uses the verified CJK font. Unsupported glyphs are explicit. */
static void supported_text(char *out,size_t cap,const char *value) {
    size_t at=0;bool missing=false;uint32_t pos=0;
    while(value[pos]&&at+5<cap) {
        uint32_t before=pos,cp=(unsigned char)value[pos++];
        unsigned extra=cp<0x80?0:cp<0xe0?1:cp<0xf0?2:3;
        if(extra)cp&=(1u<<(6-extra))-1;
        while(extra--&&value[pos])cp=(cp<<6)|((unsigned char)value[pos++]&0x3f);
        lv_font_glyph_dsc_t g={0};
        if(cp=='\n'||(lv_font_get_glyph_dsc(&muse_font_16,&g,cp,0)&&!g.is_placeholder)) {
            size_t n=pos-before;if(at+n>=cap)break;
            memcpy(out+at,value+before,n);at+=n;
        } else {out[at++]='?';missing=true;}
    }
    out[at]=0;
    const char *note="\n部分字符请在 Mac 查看";
    if(missing&&at+strlen(note)<cap)strcpy(out+at,note);
}

void muse_ui_update(muse_state_t state,const char *value,int battery,int seconds) {
    static const char *titles[]={"等待 Mac","准备就绪","正在聆听","识别语音","确认发送","Muse 正在处理","Muse 回复","连接或处理失败"};
    char body[2304],footer[120];
    if(!bsp_lvgl_lock(500))return;
    supported_text(body,sizeof(body),value);
    bool compact=compact_state(state);
    bool recording=state==S_RECORDING;
    bool idle=state==S_IDLE;
    if(state!=s_state) {s_state=state;s_tick=0;s_frame=-1;}
    lv_obj_set_style_bg_color(s_screen,lv_color_hex(recording?0x181C21:0xFFFFFF),0);
    lv_obj_set_style_bg_color(s_panel,lv_color_hex(compact?0xFFFFFF:0xF8F4EF),0);
    lv_obj_set_style_border_color(s_panel,lv_color_hex(0xEEE8E2),0);
    lv_obj_set_style_border_width(s_panel,compact?1:0,0);
    lv_obj_set_style_text_color(s_title,lv_color_hex(recording?0xF9F3EE:0x4A403A),0);
    lv_obj_set_style_text_color(s_battery,lv_color_hex(recording?0xC9C2BC:0x746A64),0);
    lv_obj_set_style_text_color(s_status,lv_color_hex(recording?0xF9F3EE:0x413A35),0);
    lv_obj_set_style_text_color(s_footer,lv_color_hex(recording?0xC9C2BC:0x776D67),0);
    if(recording) {
        lv_obj_add_flag(s_full_avatar,LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_compact_avatar,LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(s_record_avatar,LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(s_record_pill,LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(s_record_clock,LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_panel,LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(s_meter,LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_pos(s_dot,19,238);lv_obj_set_pos(s_status,36,232);
        lv_obj_set_pos(s_meter,20,259);lv_obj_set_size(s_meter,200,4);
        lv_obj_set_pos(s_footer,14,294);
        lv_label_set_text_fmt(s_record_clock,"%02d / 30 秒",seconds<0?0:seconds>30?30:seconds);
    } else {
        lv_obj_add_flag(s_record_avatar,LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_record_pill,LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_record_clock,LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_pos(s_meter,20,225);lv_obj_set_size(s_meter,200,3);
    }
    if(compact) {
        lv_obj_add_flag(s_full_avatar,LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(s_compact_avatar,LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_meter,LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_pos(s_dot,19,116);lv_obj_set_pos(s_status,36,109);
        lv_obj_set_pos(s_panel,10,139);lv_obj_set_size(s_panel,220,128);
        lv_obj_set_pos(s_body,10,9);lv_obj_set_width(s_body,200);
        lv_obj_set_pos(s_footer,14,276);
        lv_obj_remove_flag(s_panel,LV_OBJ_FLAG_HIDDEN);
    } else if(!recording) {
        lv_obj_remove_flag(s_full_avatar,LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_compact_avatar,LV_OBJ_FLAG_HIDDEN);
        if(idle)lv_obj_add_flag(s_meter,LV_OBJ_FLAG_HIDDEN);
        else lv_obj_remove_flag(s_meter,LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_pos(s_dot,19,idle?258:241);lv_obj_set_pos(s_status,36,idle?252:235);
        lv_obj_set_pos(s_panel,12,260);lv_obj_set_size(s_panel,216,40);
        lv_obj_set_pos(s_body,8,3);lv_obj_set_width(s_body,200);
        lv_obj_set_pos(s_footer,14,idle?291:302);
        if(idle)lv_obj_add_flag(s_panel,LV_OBJ_FLAG_HIDDEN);
        else lv_obj_remove_flag(s_panel,LV_OBJ_FLAG_HIDDEN);
    }
    lv_label_set_text(s_status,titles[state]);
    lv_label_set_text(s_body,body);
    if(battery<0)lv_label_set_text(s_battery,"--%");
    else lv_label_set_text_fmt(s_battery,"%d%%",battery);
    uint32_t color=state==S_RECORDING?0xD47B56:(state==S_ERROR?0xC45759:0x83A476);
    lv_obj_set_style_bg_color(s_dot,lv_color_hex(color),0);
    lv_bar_set_value(s_meter,state==S_RECORDING?seconds*100/30:0,LV_ANIM_OFF);
    if(state==S_OFFLINE)snprintf(footer,sizeof(footer),"检查配网和 Mac 桥接");
    else if(state==S_RECORDING)snprintf(footer,sizeof(footer),"确定结束 · 长按取消");
    else if(state==S_REVIEW)snprintf(footer,sizeof(footer),"确定发送 · 上下滚动\n长按确定取消");
    else if(state==S_WAITING||state==S_TRANSCRIBING)snprintf(footer,sizeof(footer),"长按确定取消等待");
    else if(state==S_REPLY)snprintf(footer,sizeof(footer),"确定开始说话 · 上下滚动");
    else if(state==S_ERROR)snprintf(footer,sizeof(footer),"确定重试");
    else snprintf(footer,sizeof(footer),"确定开始说话");
    lv_label_set_text(s_footer,footer);
    bsp_lvgl_unlock();
}

void muse_ui_scroll(int direction) {
    if(!bsp_lvgl_lock(200))return;
    if(compact_state(s_state))lv_obj_scroll_by(s_panel,0,direction*55,LV_ANIM_OFF);
    bsp_lvgl_unlock();
}
