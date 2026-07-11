// wifi_manager.h - WiFi接続管理(AP設定モード対応)
// umbrella_alert (https://github.com/TsubokuLab/umbrella_alert) の仕組みを移植。

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include <ESPmDNS.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <Preferences.h>
#include <esp_mac.h>
#include "config.h"

// ==== 外部変数・関数の宣言 ====
extern String ssidList;
extern String wifi_ssid;
extern String wifi_password;
extern int networkCount;
extern DNSServer dnsServer;
extern WebServer webServer;
extern Preferences preferences;

extern void startWebServer();
extern void rebootDevice();

// ==== デバイス個体識別(複数台運用のためMAC由来で一意化) ====
// AP SSID = "PIR-Monitor-A1B2" / mDNS = "pir-monitor-a1b2.local"
String g_apSsid;    // 実行時に確定(initDeviceIdentity)
String g_mdnsHost;  // 実行時に確定(小文字)

// MAC末尾2バイトから4桁HEX
String deviceSuffix() {
    uint8_t mac[6] = {0};
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    char buf[5];
    snprintf(buf, sizeof(buf), "%02X%02X", mac[4], mac[5]);
    return String(buf);
}

// 個体固有の SSID / mDNSホスト名を確定(setupで一度呼ぶ)
void initDeviceIdentity() {
    String suf = deviceSuffix();
    g_apSsid   = String(AP_SSID_BASE) + "-" + suf;
    g_mdnsHost = String(DNS_DOMAIN_BASE) + "-" + suf;
    g_mdnsHost.toLowerCase();
    Serial.println("デバイス識別: SSID=" + g_apSsid + " / http://" + g_mdnsHost + ".local");
}

// 保存された設定の復元
boolean restoreConfig() {
    preferences.begin("wifi-config", false);
    wifi_ssid = preferences.getString("WIFI_SSID", "");
    wifi_password = preferences.getString("WIFI_PASSWD", "");
    preferences.end();
    if (wifi_ssid.length() > 0) {
        WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());
        return true;
    }
    return false;
}

// WiFi接続確認
boolean checkConnection() {
    int count = 0;
    Serial.print("WiFi接続確認中");
    while (count < WIFI_CONNECTION_TIMEOUT) {
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\n接続成功!");
            return true;
        }
        delay(500);
        Serial.print(".");
        count++;
    }
    Serial.println("接続タイムアウト");
    return false;
}

// ネットワークリスト更新(WiFiスキャン)
void updateNetworkList() {
    networkCount = WiFi.scanNetworks();
    delay(100);
    Serial.printf("ネットワークリスト更新: %d個のネットワークを検出\n", networkCount);
    ssidList = "";
    for (int i = 0; i < networkCount; ++i) {
        ssidList += "<option value='" + WiFi.SSID(i) + "'>";
        ssidList += WiFi.SSID(i);
        ssidList += " (" + String(WiFi.RSSI(i)) + "dBm)";
        ssidList += (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " 🔓" : " 🔒";
        ssidList += "</option>";
    }
}

// セットアップモード(アクセスポイントモード)の開始
void setupMode() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    Serial.println("ネットワークスキャンを開始");
    updateNetworkList();

    delay(100);
    WiFi.softAPConfig(AP_IP_ADDR, AP_IP_ADDR, IPAddress(255, 255, 255, 0));
    WiFi.softAP(g_apSsid.c_str(), AP_PASS);
    WiFi.mode(WIFI_AP);

    startWebServer();

    if (MDNS.begin(g_mdnsHost.c_str())) {
        Serial.println("MDNS responder started : http://" + g_mdnsHost + ".local");
    }

    Serial.printf("\nアクセスポイント開始: \"%s\"\n", g_apSsid.c_str());
    Serial.printf("設定URL: http://%s\n", AP_IP_ADDR.toString().c_str());
}

// WiFi設定をクリアして再起動(次回起動は設定モードになる)
void resetSettings() {
    Serial.println("WiFi設定をクリア");
    preferences.begin("wifi-config", false);
    preferences.remove("WIFI_SSID");
    preferences.remove("WIFI_PASSWD");
    preferences.end();
    rebootDevice();
}

#endif // WIFI_MANAGER_H
