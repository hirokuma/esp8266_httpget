//https://github.com/esp8266/source-code-examples/blob/master/dweet/user/user_main.c
#include "ets_sys.h"
#include "osapi.h"
#include "driver/uart.h"

#include "user_interface.h"
#include "user_config.h"

#define MY_HOST     "www7b.biglobe.ne.jp"
#define MY_GET      "/~hiro99ma/index.html"

static const char M_SSID[] = MY_SSID;
static const char M_PASSWD[] = MY_PASSWD;

//#define M_PRIOR         (0)
//#define M_SZ_QUEUE      (1)
//static os_event_t queue[M_SZ_QUEUE];
//static void ICACHE_FLASH_ATTR event_handler(os_event_t *pEvent);

#include "espconn.h"

static uint8_t mBuffer[2048];
static struct espconn mConn;
static ip_addr_t mHostIp;
static esp_tcp mTcp;
static void ICACHE_FLASH_ATTR wifi_eventcb(System_Event_t *evt);
static void ICACHE_FLASH_ATTR dns_done(const char *pName, ip_addr_t *pIpAddr, void *pArg);
static void ICACHE_FLASH_ATTR tcp_connected(void *pArg);
static void ICACHE_FLASH_ATTR tcp_disconnected(void *pArg);
static void ICACHE_FLASH_ATTR data_received(void *pArg, char *pData, unsigned short Len);

void user_rf_pre_init(void)
{
}

void user_init(void)
{
    uart_init(BIT_RATE_115200, BIT_RATE_115200);
    os_printf("\nReady.\n");

    //////////////////////////////////////////////////////
    struct station_config stconf;
    
    os_memcpy(stconf.ssid, M_SSID, sizeof(M_SSID));
    os_memcpy(stconf.password, M_PASSWD, sizeof(M_PASSWD));
    stconf.bssid_set = 0;
    
    wifi_set_opmode(STATION_MODE);
    //wifi_set_opmode(STATIONAP_MODE);
    wifi_station_set_config(&stconf);
    
    //////////////////////////////////////////////////////

    //タスク
//    system_os_task(event_handler, M_PRIOR, queue, M_SZ_QUEUE);
//    system_os_post(M_PRIOR, 0, 0);

    //WiFiイベント
    wifi_set_event_handler_cb(wifi_eventcb);
}


//static void ICACHE_FLASH_ATTR event_handler(os_event_t *pEvent)
//{
//    os_printf("event!\n");
//}


static void ICACHE_FLASH_ATTR wifi_eventcb(System_Event_t *evt)
{
    err_t err;

    os_printf("evt->event : %x\n", evt->event);

    switch (evt->event) {
    case EVENT_STAMODE_CONNECTED:
        os_printf("[CONN] SSID[%s] CH[%d]\n", evt->event_info.connected.ssid, evt->event_info.connected.channel);
        break;
    case EVENT_STAMODE_DISCONNECTED:
        os_printf("[DISC] SSID[%s] REASON[%d]\n", evt->event_info.disconnected.ssid, evt->event_info.disconnected.reason);
        break;
    case EVENT_STAMODE_AUTHMODE_CHANGE:
        os_printf("[CHG_AUTH]\n");
        break;
    case EVENT_STAMODE_GOT_IP:
        os_printf("[GOT_IP] IP[" IPSTR "] MASK[" IPSTR "] GW[" IPSTR "]\n",
                        IP2STR(&evt->event_info.got_ip.ip),
                        IP2STR(&evt->event_info.got_ip.mask),
                        IP2STR(&evt->event_info.got_ip.gw));
        
        //接続
        err = espconn_gethostbyname(&mConn, MY_HOST, &mHostIp, dns_done);
        if (err != ESPCONN_OK) {
            os_printf("espconn_gethostbyname fail : %d\n", err);
        }
        break;
    case EVENT_STAMODE_DHCP_TIMEOUT:
        break;
    default:
        break;
    }
}

static void ICACHE_FLASH_ATTR dns_done(const char *pName, ip_addr_t *pIpAddr, void *pArg)
{
    struct espconn *pConn = (struct espconn *)pArg;

    os_printf("[[%s]]\n", __func__);

    if (pConn == NULL) {
        os_printf("DNS lookup fail.\n");
        return;
    }

    pConn->type = ESPCONN_TCP;
    pConn->state = ESPCONN_NONE;
    pConn->proto.tcp = &mTcp;
    pConn->proto.tcp->local_port = espconn_port();
    pConn->proto.tcp->remote_port = 80;         //http
    os_memcpy(pConn->proto.tcp->remote_ip, &pIpAddr->addr, 4);
    
    espconn_regist_connectcb(pConn, tcp_connected);
    espconn_regist_disconcb(pConn, tcp_disconnected);
    espconn_connect(pConn);
}

static void ICACHE_FLASH_ATTR tcp_connected(void *pArg)
{
    err_t err;
    struct espconn *pConn = (struct espconn *)pArg;

    os_printf("[[%s]]\n", __func__);

    espconn_regist_recvcb(pConn, data_received);
    espconn_sent_callback(data_sent);

    os_sprintf(mBuffer,
            "GET " MY_GET " HTTP/1.1\r\n"
            "Host: " MY_HOST "\r\n"
            "\r\n");
    os_printf("[%s]\n", mBuffer);
    err = espconn_send(pConn, mBuffer, os_strlen(mBuffer));
    if (err != 0) {
        os_printf("espconn_send fail: %d\n", err);
    }
}

static void ICACHE_FLASH_ATTR tcp_disconnected(void *pArg)
{
    struct espconn *pConn = (struct espconn *)pArg;

    os_printf("[[%s]]\n", __func__);

    wifi_station_disconnect();
}

static void ICACHE_FLASH_ATTR data_sent(void *pArg)
{
    os_printf("[[%s]]\n", __func__);
}


static void ICACHE_FLASH_ATTR data_received(void *pArg, char *pData, unsigned short Len)
{
    struct espconn *pConn = (struct espconn *)pArg;

    os_printf("RECV[%s]\n", pData);

    espconn_disconnect(pConn);
}
