#include "muse_network.h"
#include "muse_protocol.h"
#include "cJSON.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_tls.h"
#include "lwip/sockets.h"
#include "esp_sntp.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/stream_buffer.h"
#include "freertos/task.h"
#include <stdatomic.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
static char s_cfg[2049],s_host[128],s_token[96],s_ca[1500];
static int s_port;
static atomic_bool s_connected,s_wifi;
static QueueHandle_t s_tx;
static StreamBufferHandle_t s_rx;
typedef struct { size_t length; uint8_t bytes[MUSE_MAX_FRAME]; } packet_t;
static const char *string(cJSON *o,const char *key) {cJSON *v=cJSON_GetObjectItemCaseSensitive(o,key);return cJSON_IsString(v)?v->valuestring:NULL;}
static bool valid(cJSON *o) {
    const char *ssid=string(o,"ssid"),*pw=string(o,"password"),*host=string(o,"host"),*token=string(o,"token"),*ca=string(o,"ca");
    cJSON *port=cJSON_GetObjectItemCaseSensitive(o,"port"),*epoch=cJSON_GetObjectItemCaseSensitive(o,"epoch");
    return ssid&&strlen(ssid)>0&&strlen(ssid)<=32&&pw&&strlen(pw)<=63&&host&&strlen(host)>0&&strlen(host)<sizeof(s_host)&&token&&strlen(token)>=32&&strlen(token)<sizeof(s_token)&&ca&&strlen(ca)<sizeof(s_ca)&&strstr(ca,"-----BEGIN CERTIFICATE-----")&&cJSON_IsNumber(port)&&port->valueint>0&&port->valueint<65536&&cJSON_IsNumber(epoch)&&epoch->valuedouble>1700000000;
}
bool muse_network_configure(const uint8_t *json,size_t n) {
    if(n>=sizeof(s_cfg)||!n)return false;
    char temp[2049];memcpy(temp,json,n);temp[n]=0;
    cJSON *o=cJSON_Parse(temp);bool ok=o&&valid(o);cJSON_Delete(o);
    if(!ok)return false;
    nvs_handle_t h;if(nvs_open("muse",NVS_READWRITE,&h)!=ESP_OK)return false;
    ok=nvs_set_str(h,"network",temp)==ESP_OK&&nvs_commit(h)==ESP_OK;nvs_close(h);memset(temp,0,sizeof(temp));return ok;
}
const char *muse_network_token(void) {return s_token;}
static void event(void *arg,esp_event_base_t base,int32_t id,void *data) {
    (void)arg;(void)data;
    if(base==WIFI_EVENT&&id==WIFI_EVENT_STA_START)esp_wifi_connect();
    if(base==WIFI_EVENT&&id==WIFI_EVENT_STA_DISCONNECTED) {atomic_store(&s_wifi,false);esp_wifi_connect();}
    if(base==IP_EVENT&&id==IP_EVENT_STA_GOT_IP)atomic_store(&s_wifi,true);
}
static void worker(void *arg) {
    (void)arg;packet_t p;uint8_t buf[512];
    for(;;) {
        if(!atomic_load(&s_wifi)) {vTaskDelay(pdMS_TO_TICKS(500));continue;}
        esp_tls_t *tls=esp_tls_init();if(!tls) {vTaskDelay(pdMS_TO_TICKS(2000));continue;}
        esp_tls_cfg_t cfg={.cacert_buf=(const unsigned char *)s_ca,.cacert_bytes=strlen(s_ca)+1,.common_name="muse-bridge",.timeout_ms=3000};
        if(esp_tls_conn_new_sync(s_host,strlen(s_host),s_port,&cfg,tls)!=1) {esp_tls_conn_destroy(tls);vTaskDelay(pdMS_TO_TICKS(3000));continue;}
        int fd=-1;esp_tls_get_conn_sockfd(tls,&fd);struct timeval wait={.tv_sec=0,.tv_usec=10000};
        setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&wait,sizeof(wait));
        while(xQueueReceive(s_tx,&p,0)==pdTRUE) {}
        atomic_store(&s_connected,true);
        bool ok=true;
        while(ok&&atomic_load(&s_wifi)) {
            if(xQueueReceive(s_tx,&p,0)==pdTRUE) {
                size_t off=0;int retries=0;
                while(off<p.length) {int r=esp_tls_conn_write(tls,p.bytes+off,p.length-off);
                    if(r>0) {off+=(size_t)r;retries=0;}
                    else if((r==ESP_TLS_ERR_SSL_WANT_READ||r==ESP_TLS_ERR_SSL_WANT_WRITE)&&retries++<20)vTaskDelay(pdMS_TO_TICKS(5));
                    else {ok=false;break;}
                }
            }
            int n=esp_tls_conn_read(tls,buf,sizeof(buf));
            if(n>0) {if(xStreamBufferSend(s_rx,buf,n,pdMS_TO_TICKS(100))!=(size_t)n)ok=false;}
            else if(n!=ESP_TLS_ERR_SSL_WANT_READ&&n!=ESP_TLS_ERR_SSL_WANT_WRITE&&n!=ESP_TLS_ERR_SSL_TIMEOUT)ok=false;
            vTaskDelay(pdMS_TO_TICKS(1));
        }
        atomic_store(&s_connected,false);esp_tls_conn_destroy(tls);vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
bool muse_network_init(void) {
    if(nvs_flash_init()!=ESP_OK)return false;
    nvs_handle_t h;if(nvs_open("muse",NVS_READONLY,&h)!=ESP_OK)return false;
    size_t size=sizeof(s_cfg);esp_err_t e=nvs_get_str(h,"network",s_cfg,&size);nvs_close(h);if(e!=ESP_OK)return false;
    cJSON *o=cJSON_Parse(s_cfg);if(!o||!valid(o)){cJSON_Delete(o);return false;}
    wifi_config_t wifi={0};memcpy(wifi.sta.ssid,string(o,"ssid"),strlen(string(o,"ssid")));memcpy(wifi.sta.password,string(o,"password"),strlen(string(o,"password")));
    strcpy(s_host,string(o,"host"));strcpy(s_token,string(o,"token"));strcpy(s_ca,string(o,"ca"));s_port=cJSON_GetObjectItemCaseSensitive(o,"port")->valueint;
    /* Bootstrap a trusted lower bound supplied over physical USB; SNTP refreshes
       wall time after association. Certificate chain and name checks stay on. */
    struct timeval tv={.tv_sec=(time_t)cJSON_GetObjectItemCaseSensitive(o,"epoch")->valuedouble};settimeofday(&tv,NULL);
    cJSON_Delete(o);memset(s_cfg,0,sizeof(s_cfg));
    if(esp_netif_init()!=ESP_OK||esp_event_loop_create_default()!=ESP_OK)return false;
    if(!esp_netif_create_default_wifi_sta())return false;
    wifi_init_config_t init=WIFI_INIT_CONFIG_DEFAULT();
    if(esp_wifi_init(&init)!=ESP_OK)return false;
    esp_event_handler_register(WIFI_EVENT,ESP_EVENT_ANY_ID,event,NULL);esp_event_handler_register(IP_EVENT,IP_EVENT_STA_GOT_IP,event,NULL);
    if(esp_wifi_set_storage(WIFI_STORAGE_RAM)!=ESP_OK||esp_wifi_set_mode(WIFI_MODE_STA)!=ESP_OK||esp_wifi_set_config(WIFI_IF_STA,&wifi)!=ESP_OK)return false;
    memset(&wifi,0,sizeof(wifi));
    s_tx=xQueueCreate(4,sizeof(packet_t));s_rx=xStreamBufferCreate(4096,1);if(!s_tx||!s_rx)return false;
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);esp_sntp_setservername(0,"time.cloudflare.com");esp_sntp_init();
    if(esp_wifi_start()!=ESP_OK)return false;
    return xTaskCreate(worker,"muse_net",6144,NULL,4,NULL)==pdPASS;
}
muse_send_result_t muse_network_send_result(const uint8_t *data,size_t n) {
    if(!data||!n||n>MUSE_MAX_FRAME)return MUSE_SEND_INVALID;
    if(!atomic_load(&s_connected))return MUSE_SEND_DISCONNECTED;
    packet_t p={.length=n};memcpy(p.bytes,data,n);
    if(xQueueSend(s_tx,&p,pdMS_TO_TICKS(100))==pdTRUE)return MUSE_SEND_OK;
    return atomic_load(&s_connected)?MUSE_SEND_QUEUE_FULL:MUSE_SEND_DISCONNECTED;
}
bool muse_network_send(const uint8_t *data,size_t n) {
    return muse_network_send_result(data,n)==MUSE_SEND_OK;
}
size_t muse_network_read(uint8_t *data,size_t n) {return s_rx?xStreamBufferReceive(s_rx,data,n,0):0;}
