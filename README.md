# esp8266_gethttp

* ESP8266 + esp-open-sdk(ESP8266_NONOS_SDK_V2.0.0_16_08_10)

## メモ
ビルドする場合は、フォルダごとSDKの直下に置くかすること。
そこで ./gen_misc.sh としないとビルドができない(どこかでエラーになる)。

## メモ2
esp-open-sdkを使うための環境変数を設定しておくこと。
- XTENSA_INCLUDE : /path/to/&lt;esp-open-sdk&gt;/sdk/include
- XTENSA_LIB : /path/to/&lt;esp-open-sdk&gt;/sdk/lib

### behavior

UART : 115200bps  
http://www7b.biglobe.ne.jp/~hiro99ma/

```
<!DOCTYPE html>
<html lang="ja">
	<head>
		<meta charset="UTF-8" />
		<title>hiro99ma test site</title>
	</head>
	
	<body>
		<header>
			<a href="ikaku01/ikaku.html">ikaku01</a>
			<a href="ikaku02/ikaku.html">ikaku02</a>
			<a href="ikaku03/ikaku.html">ikaku03</a>
			<a href="ikaku04/ikaku.html">ikaku04</a>
			<a href="ikaku05/ikaku.html">ikaku05</a>
		</header>
	</body>

</html>
```

### 出力例1 - system_set_os_print(1)

```
Ready.
mode : sta(自分のMACアドレス)
add if0
f r0, scandone
state: 0 -> 2 (b0)
state: 2 -> 3 (0)
state: 3 -> 5 (10)
add 0
aid 1
cnt

connected with 自分のSSID, channel 自分のチャネル
dhcp client start...
evt->event : 0
[CONN] SSID[自分のSSID] CH[自分のチャネル]
ip:自分のIP,mask:自分のMASK,gw:自分のGW
evt->event : 3
[GOT_IP] IP[自分のIP] MASK[自分のMASK] GW[自分のGW]
espconn_gethostbyname fail : -5
[[dns_done]]
[[tcp_connected]]
[GET /~hiro99ma/index.html HTTP/1.1
Host: www7b.biglobe.ne.jp

]
RECV[HTTP/1.0 200 OK
Date: Wed, 30 Dec 2015 00:44:04 GMT
Server: httpd
Last-Modified: Fri, 08 Mar 2013 15:39:00 GMT
Accept-Ranges: bytes
Content-Length: 384
Content-Type: text/html
X-Cache: MISS from bvka33274
X-Cache-Lookup: MISS from bvka33274:80
Via: 1.1 bvka33274:80 (squid)
Connection: close

<!DOCTYPE html>
<html lang="ja">
        <head>
                <meta charset="UTF-8" />
                <title>hiro99ma test site</title>
        </head>

        <body>
                <header>
                        <a href="ikaku01/ikaku.html">ikaku01</a>
                        <a href="ikaku02/ikaku.html">ikaku02</a>
                        <a href="ikaku03/ikaku.html">ikaku03</a>
                        <a href="ikaku04/ikaku.html">ikaku04</a>
                        <a href="ikaku05/ikaku.html">ikaku05</a>
                </header>
        </body>

</html>
]
[[tcp_disconnected]]
state: 5 -> 0 (0)
rm 0
evt->event : 1
[DISC] SSID[自分のSSID] REASON[8]
```

### 出力例2 - system_set_os_print(0)

```
[[ user_init() ]]
[[ wifi_eventcb() ]]
[[ wifi_eventcb() ]]
[[ dns_done() ]]
[[ tcp_connected() ]]
[[ data_sent() ]]
[[ data_received() ]]
----------
HTTP/1.0 200 OK
Last-Modified: Fri, 08 Mar 2013 15:39:00 GMT
Accept-Ranges: bytes
Content-Length: 384
Content-Type: text/html
Date: Tue, 05 Jan 2016 04:48:27 GMT
Server: httpd
X-Cache: HIT from bvka33276
X-Cache-Lookup: HIT from bvka33276:80
Via: 1.1 bvka33276:80 (squid)
Connection: close

<!DOCTYPE html>
<html lang="ja">
        <head>
                <meta charset="UTF-8" />
                <title>hiro99ma test site</title>
        </head>

        <body>
                <header>
                        <a href="ikaku01/ikaku.html">ikaku01</a>
                        <a href="ikaku02/ikaku.html">ikaku02</a>
                        <a href="ikaku03/ikaku.html">ikaku03</a>
                        <a href="ikaku04/ikaku.html">ikaku04</a>
                        <a href="ikaku05/ikaku.html">ikaku05</a>
                </header>
        </body>

</html>

----------
[[ tcp_disconnected() ]]
[[ wifi_eventcb() ]]
```
