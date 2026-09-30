#include "muse_protocol.h"
#include "muse_state.h"
#include "muse_ui.h"
#include "muse_network.h"
#include "esp_system.h"
#include "bsp_audio.h"
#include "bsp_battery.h"
#include "bsp_button.h"
#include "bsp_display.h"
#include "bsp_i2c.h"
#include "driver/usb_serial_jtag.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <stdio.h>
#include <string.h>
typedef struct { bsp_btn_t button; bsp_btn_ev_t event; } muse_key_t;
static QueueHandle_t s_keys;
static muse_parser_t s_parser,s_usb_parser;
static muse_frame_t s_frame;
static uint8_t s_wire[MUSE_MAX_FRAME];
static muse_state_t s_state=S_OFFLINE;
static uint32_t s_session;
static bool s_audio_ok,s_ready;
static int s_battery=-1;
static int64_t s_started,s_activity,s_ping,s_state_at;
static char s_text[2200]="首次使用请通过 USB 配网，然后无线连接 Mac。";
static int64_t now_ms(void) {return esp_timer_get_time()/1000;}
static void show(muse_state_t state,const char *text) {
    s_state=state;s_state_at=now_ms();
    if(text)snprintf(s_text,sizeof(s_text),"%s",text);
    muse_ui_update(s_state,s_text,s_battery,(int)((now_ms()-s_started)/1000));
}
static bool send_frame(uint8_t type,const void *data,size_t n) {
    size_t len=muse_encode(s_wire,type,s_session,data,n);
    return muse_network_send(s_wire,len);
}
static void on_key(bsp_btn_t b,bsp_btn_ev_t e,void *ctx) {
    (void)ctx;muse_key_t k={b,e};if(s_keys)xQueueSend(s_keys,&k,0);
}
static void cancel(void) {
    send_frame(M_CANCEL,NULL,0);s_session++;if(!s_session)s_session=1;
    show(s_ready?S_IDLE:S_OFFLINE,s_ready?"按确定键开始说话。":"请检查 Mac 桥接与 Muse 网页。");
}
static void key(muse_key_t k) {
    if(k.event!=BSP_BTN_CLICK&&k.event!=BSP_BTN_LONG)return;
    s_activity=now_ms();bsp_display_backlight(75);
    if(k.button==BSP_BTN_OK&&k.event==BSP_BTN_LONG) {cancel();return;}
    if(k.event!=BSP_BTN_CLICK)return;
    if(k.button!=BSP_BTN_OK) {
        if(s_state==S_REPLY||s_state==S_REVIEW)muse_ui_scroll(k.button==BSP_BTN_UP?1:-1);
        return;
    }
    if(s_state==S_RECORDING) {
        if(!send_frame(M_END,NULL,0))show(S_ERROR,"无线传输失败，请重新连接。");
        else show(S_TRANSCRIBING,"Mac 正在将语音转换成文字。");
    } else if(s_state==S_REVIEW) {
        if(send_frame(M_CONFIRM,NULL,0))show(S_WAITING,"指令已交给 Mac。请等待 Muse 回复。");
        else show(S_ERROR,"发送失败。请在 Muse 查看是否已收到，避免重复操作。");
    } else if(muse_can_record(s_state)&&s_ready) {
        if(!s_audio_ok) {show(S_ERROR,"音频硬件初始化失败，请重启后重试。");return;}
        s_session++;if(!s_session)s_session=1;s_started=now_ms();
        if(send_frame(M_START,NULL,0))show(S_RECORDING,"请说话。再次按确定键结束录音，最长三十秒。");
        else show(S_ERROR,"Mac 未收到录音请求，请检查连接。");
    }
}
static void receive(muse_frame_t *f) {
    if(f->type==M_PING) {
        s_ping=now_ms();s_ready=f->length==1&&f->data[0]==1;
        if(s_state==S_OFFLINE&&s_ready)show(S_IDLE,"按确定键开始说话。识别后可确认发送。");
        if(!s_ready&&s_state==S_IDLE)show(S_OFFLINE,"请在 Mac 完成语音模型加载，并打开已登录的 Muse 网页。");
        return;
    }
    if(f->session!=s_session)return;
    if(f->type==M_TRANSCRIPT&&s_state==S_TRANSCRIBING) {
        muse_utf8_copy(s_text,sizeof(s_text),f->data,f->length);
        if(!s_text[0])show(S_ERROR,"未识别到语音，请重新录音。");else show(S_REVIEW,NULL);
    } else if(f->type==M_STATUS&&s_state==S_WAITING) {
        muse_utf8_copy(s_text,sizeof(s_text),f->data,f->length);show(S_WAITING,NULL);
    } else if(f->type==M_REPLY&&muse_accepts_reply(s_state)) {
        muse_utf8_copy(s_text,sizeof(s_text),f->data,f->length);show(S_REPLY,NULL);
    }     else if(f->type==M_ERROR&&s_state!=S_IDLE&&s_state!=S_OFFLINE) {
        muse_utf8_copy(s_text,sizeof(s_text),f->data,f->length);show(S_ERROR,NULL);
    }
}
void app_main(void) {
    esp_log_level_set("*",ESP_LOG_NONE);
    s_session=esp_random();if(!s_session)s_session=1;
    if(bsp_i2c_init()!=ESP_OK)return;
    if(bsp_display_init()!=ESP_OK||!bsp_lvgl_init())return;
    bsp_display_backlight(75);
    if(!muse_ui_init())return;
    s_keys=xQueueCreate(8,sizeof(muse_key_t));
    if(!s_keys||bsp_button_init(on_key,NULL)!=ESP_OK) {show(S_ERROR,"按键初始化失败，请重启。");return;}
    s_audio_ok=bsp_audio_init()==ESP_OK;
    if(s_audio_ok)s_audio_ok=bsp_audio_set_format(16000,16,1)==ESP_OK;
    if(bsp_battery_init()==ESP_OK)s_battery=bsp_battery_soc();
    usb_serial_jtag_driver_config_t cfg={.tx_buffer_size=4096,.rx_buffer_size=4096};
    if(usb_serial_jtag_driver_install(&cfg)!=ESP_OK) {show(S_ERROR,"USB 初始化失败，请重启。");return;}
    muse_network_init();
    s_activity=now_ms();int64_t hello=0,battery_at=0,refresh=0;bool dimmed=false;
    show(S_OFFLINE,NULL);
    uint8_t in[256];int16_t pcm[256];
    for(;;) {
        int64_t now=now_ms();muse_key_t k;
        while(xQueueReceive(s_keys,&k,0)==pdTRUE) {key(k);dimmed=false;}
        int n=usb_serial_jtag_read_bytes(in,sizeof(in),pdMS_TO_TICKS(1));
        for(int i=0;i<n;i++)if(muse_feed(&s_usb_parser,in[i],&s_frame)&&s_frame.type==15) {
            bool ok=muse_network_configure(s_frame.data,s_frame.length);
            const char *result=ok?"OK":"INVALID_CONFIG";
            size_t length=muse_encode(s_wire,16,s_frame.session,result,strlen(result));
            usb_serial_jtag_write_bytes(s_wire,length,pdMS_TO_TICKS(1000));
            if(ok) {show(S_OFFLINE,"配网已保存，正在重启连接。");vTaskDelay(pdMS_TO_TICKS(1000));esp_restart();}
        }
        n=(int)muse_network_read(in,sizeof(in));
        for(int i=0;i<n;i++)if(muse_feed(&s_parser,in[i],&s_frame))receive(&s_frame);
        if(now-hello>=2000) {const char *token=muse_network_token();send_frame(M_HELLO,token,strlen(token));hello=now;}
        if(s_ping&&now-s_ping>7000&&s_state!=S_OFFLINE) {s_ready=false;cancel();show(S_OFFLINE,"Mac 连接已断开。已发送的任务请在 Muse 查看。");}
        if(s_state==S_RECORDING) {
            if(now-s_started>=30000) {send_frame(M_END,NULL,0);show(S_TRANSCRIBING,"录音已结束，正在识别。");}
            else {
                esp_err_t audio_error=bsp_audio_read(pcm,sizeof(pcm));
                muse_send_result_t send_error=MUSE_SEND_OK;
                if(audio_error==ESP_OK) {
                    size_t length=muse_encode(s_wire,M_AUDIO,s_session,pcm,sizeof(pcm));
                    send_error=muse_network_send_result(s_wire,length);
                }
                if(audio_error!=ESP_OK||send_error!=MUSE_SEND_OK) {
                    int second=(int)((now_ms()-s_started)/1000)+1;
                    char reason[120];
                    if(audio_error!=ESP_OK)snprintf(reason,sizeof(reason),"麦克风采集失败（第 %d 秒，%s）。",second,esp_err_to_name(audio_error));
                    else if(send_error==MUSE_SEND_DISCONNECTED)snprintf(reason,sizeof(reason),"Mac 连接断开（第 %d 秒）。",second);
                    else if(send_error==MUSE_SEND_QUEUE_FULL)snprintf(reason,sizeof(reason),"音频发送队列阻塞（第 %d 秒）。",second);
                    else snprintf(reason,sizeof(reason),"音频数据发送失败（第 %d 秒）。",second);
                    send_frame(M_CANCEL,NULL,0);show(S_ERROR,reason);
                }
            }
        }
        if((s_state==S_TRANSCRIBING&&now-s_state_at>90000)||(s_state==S_WAITING&&now-s_state_at>180000)) {
            send_frame(M_CANCEL,NULL,0);show(S_ERROR,"等待超时。请在 Muse 查看状态，不会自动重复发送。");
        }
        if(now-battery_at>30000&&s_state!=S_RECORDING) {s_battery=bsp_battery_soc();battery_at=now;}
        if(now-refresh>=1000) {muse_ui_update(s_state,s_text,s_battery,(int)((now-s_started)/1000));refresh=now;}
        if(now-s_activity>60000&&!dimmed&&s_state!=S_RECORDING) {bsp_display_backlight(15);dimmed=true;}
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}
