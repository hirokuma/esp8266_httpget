//https://github.com/esp8266/source-code-examples/blob/master/dweet/user/user_main.c
#include "ets_sys.h"
#include "osapi.h"
#include "driver/uart.h"

#include "user_interface.h"
#include "user_config.h"

#define DBG_PRINTF(...)     os_printf(__VA_ARGS__)
#define DBG_FUNCNAME()      {\
    uart0_sendStr("[[ ");       \
    uart0_sendStr(__func__);    \
    uart0_sendStr("() ]]\n");   \
}

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
static void ICACHE_FLASH_ATTR data_sent(void *pArg);
static void ICACHE_FLASH_ATTR data_received(void *pArg, char *pData, unsigned short Len);


////////////////////////////////////////////////////////////////
// public
////////////////////////////////////////////////////////////////

void user_rf_pre_init(void)
{
    uart_init(BIT_RATE_115200, BIT_RATE_115200);
    os_printf("\nRF pre init.\n");

    struct rst_info *pInfo = system_get_rst_info();
    switch (pInfo->reason) {
    case REASON_DEFAULT_RST:
        os_printf("  REASON_DEFAULT_RST\n");
        break;
    case REASON_WDT_RST:
        os_printf("  REASON_WDT_RST\n");
        break;
    case REASON_EXCEPTION_RST:
        os_printf("  REASON_EXCEPTION_RST\n");
        break;
    case REASON_SOFT_WDT_RST:
        os_printf("  REASON_SOFT_WDT_RST\n");
        break;
    case REASON_SOFT_RESTART:
        os_printf("  REASON_SOFT_RESTART\n");
        break;
    case REASON_DEEP_SLEEP_AWAKE:
        os_printf("  REASON_DEEP_SLEEP_AWAKE\n");
        break;
    case REASON_EXT_SYS_RST:
        os_printf("  REASON_EXT_SYS_RST\n");
        break;
    default:
        os_printf("  unknown reason : %x\n", pInfo->reason);
        break;
    }
    os_delay_us(1 * 1000);     //1msec待って、UARTを全部吐かせる

    //system log output : OFF
    //これをすると、自分のos_printfも止まってしまう。
    //即座に止めるのか、上のログも"REASON_"くらいで終わってしまっていた。
    system_set_os_print(0);
    
    //os_printf()をUART1に割り振る(os_install_putc1)、というのもあるし、
    //UART0のピンを変更し、RX/TX→MTCK/MTDOにする(system_uart_swap)、というのもある。
    //しかし、システムが出力するログだけを停止する、というのはないようだ。

    //system_set_os_print()でログを停止させて、
    //自分で文字列を加工して、uart0_tx_buffer()でUART0に直接送信する方が確実か。
    //しかし、ここでuart0_tx_buffer()を使うと、REASONのログも正しく出力されなくなった。
    //出力されなくなったと言うよりは、ゴミになったというべきか。
    //system_set_os_print()をコメントアウトしても正しく出力されないので、os_printf()との相性が悪いのか。
    uart0_sendStr("Hello.\n");
}


void user_init(void)
{
    //user_rf_pre_init()でuart_init()を呼び出していても、ここでも呼ばないと化けた
    //しかし、system_set_os_print(0)は有効なままだった。
    uart_init(BIT_RATE_115200, BIT_RATE_115200);

    DBG_PRINTF("\nReady.\n");

    DBG_FUNCNAME();

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


////////////////////////////////////////////////////////////////
// private
////////////////////////////////////////////////////////////////


//static void ICACHE_FLASH_ATTR event_handler(os_event_t *pEvent)
//{
//    DBG_PRINTF("event!\n");
//}


static void ICACHE_FLASH_ATTR wifi_eventcb(System_Event_t *evt)
{
    err_t err;

    DBG_FUNCNAME();

    DBG_PRINTF("evt->event : %x\n", evt->event);

    switch (evt->event) {
    case EVENT_STAMODE_CONNECTED:
        DBG_PRINTF("[CONN] SSID[%s] CH[%d]\n", evt->event_info.connected.ssid, evt->event_info.connected.channel);
        break;
    case EVENT_STAMODE_DISCONNECTED:
        DBG_PRINTF("[DISC] SSID[%s] REASON[%d]\n", evt->event_info.disconnected.ssid, evt->event_info.disconnected.reason);
        break;
    case EVENT_STAMODE_AUTHMODE_CHANGE:
        DBG_PRINTF("[CHG_AUTH]\n");
        break;
    case EVENT_STAMODE_GOT_IP:
        DBG_PRINTF("[GOT_IP] IP[" IPSTR "] MASK[" IPSTR "] GW[" IPSTR "]\n",
                        IP2STR(&evt->event_info.got_ip.ip),
                        IP2STR(&evt->event_info.got_ip.mask),
                        IP2STR(&evt->event_info.got_ip.gw));
        
        //接続
        err = espconn_gethostbyname(&mConn, MY_HOST, &mHostIp, dns_done);
        if (err != ESPCONN_OK) {
            DBG_PRINTF("espconn_gethostbyname fail : %d\n", err);
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

    DBG_FUNCNAME();

    if (pConn == NULL) {
        DBG_PRINTF("DNS lookup fail.\n");
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

    DBG_FUNCNAME();

    espconn_regist_recvcb(pConn, data_received);
    espconn_regist_sentcb(pConn, data_sent);

    os_sprintf(mBuffer,
            "GET " MY_GET " HTTP/1.1\r\n"
            "Host: " MY_HOST "\r\n"
            "\r\n");
    DBG_PRINTF("[%s]\n", mBuffer);
    err = espconn_send(pConn, mBuffer, os_strlen(mBuffer));
    if (err != 0) {
        DBG_PRINTF("espconn_send fail: %d\n", err);
    }
}


static void ICACHE_FLASH_ATTR tcp_disconnected(void *pArg)
{
    struct espconn *pConn = (struct espconn *)pArg;

    DBG_FUNCNAME();

    wifi_station_disconnect();
}


static void ICACHE_FLASH_ATTR data_sent(void *pArg)
{
    DBG_FUNCNAME();
}


static void ICACHE_FLASH_ATTR data_received(void *pArg, char *pData, unsigned short Len)
{
    struct espconn *pConn = (struct espconn *)pArg;

    DBG_FUNCNAME();

    uart0_sendStr("----------\n");
    uart0_sendStr(pData);
    uart0_sendStr("\n----------\n");

    espconn_disconnect(pConn);
}
