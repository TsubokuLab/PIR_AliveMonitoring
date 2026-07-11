// config.h - アプリ定数(機密情報は含まない)
// WiFi・IFTTT・監視設定はすべて本体の設定ページ(ブラウザ)から行い、
// 本体内のNVS(Preferences)に保存されます。このファイルの編集は不要です。

#ifndef CONFIG_H
#define CONFIG_H

#include <IPAddress.h>

// ===== 基本情報 =====
#define APP_TITLE "PIR死活監視"
#define APP_VERSION "v2.0.0"
#define AUTHOR_NAME "Teruaki Tsubokura"
#define AUTHOR_URL "https://teruaki-tsubokura.com/"

// デバイスの動作モード
enum DeviceMode {
    SETUP_MODE,  // 初期設定モード(アクセスポイント+設定ページ)
    APP_MODE     // 通常動作(見守り+通知)
};

// ===== ネットワーク設定 =====
#define AP_SSID_BASE "PIR-Monitor"            // APのSSID(末尾にMAC由来の4桁が付く)
#define AP_PASS "12345678"                    // 設定モードAPのパスワード
#define DNS_DOMAIN_BASE "pir-monitor"         // mDNS: http://pir-monitor-xxxx.local
#define AP_IP_ADDR IPAddress(192, 168, 4, 1)  // 設定モードの固定IP
#define WEB_SERVER_PORT 80
#define DNS_SERVER_PORT 53
#define WIFI_CONNECTION_TIMEOUT 20            // 接続タイムアウト回数(500ms/回)
#define SERIAL_BAUD_RATE 115200

// ===== 監視のデフォルト値(設定ページで変更可能) =====
#define DEFAULT_PLACE_NAME "トイレ"           // 場所の名前
#define DEFAULT_LIMIT_HOURS 24                // 何時間動きが無ければ通知するか
#define DEFAULT_EVENT_ALIVE "AliveMonitoring" // IFTTT: 未検出通知のイベント名
#define DEFAULT_EVENT_DETECT "DetectEvent"    // IFTTT: 生存確認通知のイベント名

// ===== 動作設定 =====
#define DETECT_HOLD_SEC 60          // 「検出」表示を保持する秒数(トイレの電気方式)
#define LONG_PRESS_MS 800           // ボタンA長押し判定(見守りON/OFF)
#define INFO_SCREEN_TIMEOUT_MS 60000 // 設定QR画面から自動で戻るまでの時間
#define RESET_HOLD_MS 5000          // ボタンB長押しでWiFi設定リセット
#define IFTTT_TIMEOUT_MS 10000      // IFTTT送信のタイムアウト
#define WIFI_RETRY_INTERVAL_MS 60000 // WiFi切断時の再接続試行間隔

// ===== ハードウェア =====
#define PIR_PIN 36  // PIR Hat の信号ピン(G36)
#define LED_PIN 10  // 本体内蔵の赤色LED(LOWで点灯)

// ===== Web UI(設定ページ)スタイル =====
#define CONTAINER_MAX_WIDTH "480px"
#define BORDER_RADIUS "20px"
#define THEME_PRIMARY_START "#10b981"
#define THEME_PRIMARY_END "#059669"
#define THEME_DANGER_START "#ef4444"
#define THEME_DANGER_END "#dc2626"

#endif // CONFIG_H
