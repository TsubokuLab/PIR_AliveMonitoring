// ============================================================
//  PIR_AliveMonitoring
//  M5StickC Plus + PIRセンサによる独り暮らし向け「死活監視システム」
//
//  一定時間(初期設定24時間)動きが検出されなかった場合に
//  IFTTT経由で通知を送ります。動きが再検出された際には
//  「生存確認」の通知を送ります。
//
//  初期設定はスマホから行います:
//   1. 本体画面のQRコードを読み取って本体のWiFi(AP)に接続
//   2. 自動で開く設定ページで自宅のWiFiを選択
//   3. 再起動後、http://pir-monitor-xxxx.local で通知設定
//
//  ボタン操作:
//   ボタンA 短押し = 設定ページのQRコード表示 / 戻る
//   ボタンA 長押し = 見守りの開始・停止
//   ボタンB 5秒長押し = WiFi設定をリセット(初期設定モードへ)
//
//  https://github.com/TsubokuLab/PIR_AliveMonitoring
//  https://protopedia.net/prototype/2432
// ============================================================
#include <WiFi.h>
#include <HTTPClient.h>
#include <ESPmDNS.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <M5StickCPlus.h>
#include "efont.h"
#include "efontM5StickCPlus.h"
#include "efontEnableJaMini.h"
#include "time.h"

#include "config.h"
#include "styles.h"
#include "settings.h"      // preferences・監視設定を定義
#include "wifi_manager.h"
#include "web_server.h"

// ===== 動作モード・画面 =====
DeviceMode deviceMode = SETUP_MODE;
enum ScreenMode { SCREEN_MAIN, SCREEN_INFO };
ScreenMode screenMode = SCREEN_MAIN;
unsigned long infoScreenSince = 0;  // QR画面を開いた時刻(自動で戻る用)

// ===== Web/WiFi 用グローバル(各ヘッダから extern 参照) =====
String ssidList;
String wifi_ssid;
String wifi_password;
int networkCount = 0;
DNSServer dnsServer;
WebServer webServer(WEB_SERVER_PORT);
unsigned long lastWifiRetryMs = 0;

// ===== 時刻 =====
const long gmtOffset_sec = 3600 * 9;  // JST (UTC+9)
time_t t = 0, sent_t = 0, last_t = 0;
struct tm timeinfo;

// ===== 監視状態 =====
bool AliveMonitoring = true;        // 見守り中かどうか
bool LastSent = false;              // 未検出通知を送信済みかどうか
bool rawDetect = false;             // PIRセンサの生の検出状態
unsigned long lastRawDetectMs = 0;  // 最後に動きを検出した時刻(millis)

// ===== 画面描画の状態(ちらつき防止のため変化した部分だけ再描画) =====
bool mainDirty = true;   // メイン画面の全再描画が必要か
bool shownAlive = true;
bool shownLatch = false;
int  shownSec = -1;

// ============================================================
//  ユーティリティ
// ============================================================

// NTP同期済みかどうか(2020年以降なら有効とみなす)
bool timeValid() { return t > 1600000000; }

// 現在時刻を更新(ブロックしない)
void updateClock() {
    time(&t);
    localtime_r(&t, &timeinfo);
}

// 「検出」表示を保持中か(トイレの電気方式: 最後の動きから DETECT_HOLD_SEC 秒間)
bool detectLatched() {
    return lastRawDetectMs != 0 && (millis() - lastRawDetectMs) < (unsigned long)DETECT_HOLD_SEC * 1000UL;
}

// URLエンコード(日本語をURLに含められるようにする)
String urlEncode(const String &s) {
    String encoded = "";
    char buf[4];
    for (size_t i = 0; i < s.length(); i++) {
        char c = s[i];
        if (isalnum((unsigned char)c) || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded += c;
        } else {
            sprintf(buf, "%%%02X", (unsigned char)c);
            encoded += buf;
        }
    }
    return encoded;
}

// 経過秒数を「3日と5時間」のような文字列に変換
String SecondsToTimeString(long _seconds) {
    String _txt = "";
    long _days = _seconds / 86400;
    if (_days > 0) {
        _txt = String(_days) + "日";
        long _hour = (_seconds % 86400) / 3600;
        if (_hour >= 1) _txt += "と" + String(_hour) + "時間";
    } else {
        _txt = String(_seconds / 3600) + "時間";
    }
    return _txt;
}

// 最終検出からの経過を表示用文字列に(設定ページで使用)
String lastDetectElapsedString() {
    if (!timeValid() || last_t == 0) return "-";
    long sec = (long)difftime(t, last_t);
    if (sec < 60) return "たった今";
    if (sec < 3600) return String(sec / 60) + "分前";
    if (sec < 86400) return String(sec / 3600) + "時間前";
    return String(sec / 86400) + "日前";
}

// ===== ブザー =====
// M5.BeepはESP32コア3.xのAPI変更で音が出ないため、内蔵ブザー(GPIO2)を直接制御する
unsigned long beepUntilMs = 0;

// ブザーを鳴らす(指定時間後にupdateBeep()が止める)
void ringBeep(int freq, int durationMs) {
    ledcWriteTone(BUZZER_PIN, freq);
    beepUntilMs = millis() + durationMs;
}

// 鳴動時間が過ぎたらブザーを止める(loopから毎回呼ぶ)
void updateBeep() {
    if (beepUntilMs != 0 && millis() >= beepUntilMs) {
        ledcWriteTone(BUZZER_PIN, 0);
        beepUntilMs = 0;
    }
}

// 再起動
void rebootDevice() {
    Serial.println("再起動します");
    delay(500);
    ESP.restart();
}

// ============================================================
//  IFTTT通知
// ============================================================

// IFTTT Webhooksへ通知を送信(タイムアウト付き・ブロックしすぎない)
bool sendIfttt(const String &event, const String &v1, const String &v2) {
    if (!hasIftttKey()) {
        Serial.println("IFTTTキー未設定のため通知をスキップ");
        return false;
    }
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi未接続のため通知をスキップ");
        return false;
    }
    String url = "http://maker.ifttt.com/trigger/" + event + "/with/key/" + cfgIftttKey +
                 "?value1=" + urlEncode(v1) + "&value2=" + urlEncode(v2);
    Serial.println("IFTTT送信: " + event + " (" + v1 + " / " + v2 + ")");

    HTTPClient http;
    http.setConnectTimeout(IFTTT_TIMEOUT_MS);
    http.setTimeout(IFTTT_TIMEOUT_MS);
    if (!http.begin(url)) return false;
    int code = http.GET();
    http.end();
    Serial.printf("IFTTT応答: %d\n", code);
    return code >= 200 && code < 300;
}

// テスト通知(設定ページの「テスト通知を送信」ボタンから)
bool sendIftttTest() {
    return sendIfttt(cfgEventAlive, cfgPlaceName, "(テスト送信)");
}

// 見守りのON/OFF切り替え(ボタン長押し・設定ページ共通)
void setMonitoring(bool on) {
    if (AliveMonitoring == on) return;
    AliveMonitoring = on;
    if (on) {
        last_t = sent_t = t;  // ゼロから計測を再開
    }
    LastSent = false;
    ringBeep(3000, 100);
    mainDirty = true;
    Serial.println(on ? "見守りを開始" : "見守りを停止");
}

// ============================================================
//  画面描画
// ============================================================

// メイン画面(見守り状態・時刻・検出状態)。変化した部分だけ再描画する。
void updateMainScreen() {
    bool latch = detectLatched();

    if (mainDirty) {
        M5.Lcd.fillScreen(BLACK);
        shownSec = -1;  // 時刻も強制再描画
    }
    // 上段: 見守り状態
    if (mainDirty || shownAlive != AliveMonitoring) {
        if (AliveMonitoring) {
            M5.Lcd.fillRect(0, 0, M5.Lcd.width(), 40, GREEN);
            M5.Lcd.setTextColor(BLACK, GREEN);
            printEfont("見守り中", 55, 4, 2);
        } else {
            M5.Lcd.fillRect(0, 0, M5.Lcd.width(), 40, RED);
            M5.Lcd.setTextColor(BLACK, RED);
            printEfont("停止中", 70, 4, 2);
        }
        shownAlive = AliveMonitoring;
    }
    // 下段: 検出状態(60秒間保持して表示)
    if (mainDirty || shownLatch != latch) {
        if (latch) {
            M5.Lcd.fillRect(0, 80, M5.Lcd.width(), 55, GREEN);
            M5.Lcd.setTextColor(BLACK, GREEN);
            printEfont("検出", 70, 85, 3);
        } else {
            M5.Lcd.fillRect(0, 80, M5.Lcd.width(), 55, BLUE);
            M5.Lcd.setTextColor(WHITE, BLUE);
            printEfont("未検出", 45, 85, 3);
        }
        shownLatch = latch;
    }
    // 中段: 現在時刻(秒が変わったときだけ更新)
    if (shownSec != timeinfo.tm_sec) {
        shownSec = timeinfo.tm_sec;
        M5.Lcd.setTextSize(2);
        M5.Lcd.setTextColor(WHITE, BLACK);
        M5.Lcd.setCursor(6, 54);
        if (timeValid()) {
            M5.Lcd.printf("%04d-%02d-%02d %02d:%02d:%02d",
                          timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                          timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        } else {
            M5.Lcd.print("  ---- ---- ----  ");
        }
    }
    mainDirty = false;
}

// 初期設定モードの画面(WiFi接続用QRコード)
void drawSetupScreen() {
    M5.Lcd.fillScreen(WHITE);
    // スマホのカメラで読むとAPに接続できるWiFi QRコード
    String qr = "WIFI:T:WPA;S:" + g_apSsid + ";P:" + String(AP_PASS) + ";;";
    M5.Lcd.qrcode(qr.c_str(), 6, 6, 123, 7);

    M5.Lcd.setTextColor(BLACK, WHITE);
    printEfont("初期設定", 138, 8, 1);
    printEfont("QRでWiFi接続", 138, 30, 1);

    M5.Lcd.setTextSize(1);
    M5.Lcd.setTextColor(0x39E7 /*濃いグレー*/, WHITE);
    M5.Lcd.setCursor(138, 56);
    M5.Lcd.print("SSID:");
    M5.Lcd.setCursor(138, 66);
    M5.Lcd.print(g_apSsid);
    M5.Lcd.setCursor(138, 80);
    M5.Lcd.print("PASS:");
    M5.Lcd.setCursor(138, 90);
    M5.Lcd.print(AP_PASS);
    M5.Lcd.setCursor(138, 104);
    M5.Lcd.print("URL:");
    M5.Lcd.setCursor(138, 114);
    M5.Lcd.print(AP_IP_ADDR.toString());
}

// 設定ページのQRコード画面(通常モードでボタンA短押し)
void drawInfoScreen() {
    M5.Lcd.fillScreen(WHITE);
    String url = "http://" + g_mdnsHost + ".local";
    M5.Lcd.qrcode(url.c_str(), 6, 6, 123, 7);

    M5.Lcd.setTextColor(BLACK, WHITE);
    printEfont("設定ページ", 138, 8, 1);

    M5.Lcd.setTextSize(1);
    M5.Lcd.setTextColor(0x39E7, WHITE);
    M5.Lcd.setCursor(138, 34);
    M5.Lcd.print("URL:");
    M5.Lcd.setCursor(138, 44);
    M5.Lcd.print(g_mdnsHost);
    M5.Lcd.setCursor(138, 54);
    M5.Lcd.print(".local");
    M5.Lcd.setCursor(138, 72);
    M5.Lcd.print("IP:");
    M5.Lcd.setCursor(138, 82);
    M5.Lcd.print(WiFi.localIP().toString());

    printEfont("短押しで戻る", 138, 112, 1);
}

// ============================================================
//  セットアップ
// ============================================================
void setup() {
    M5.begin();
    M5.Lcd.setRotation(3);
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextColor(WHITE, BLACK);

    loadAppSettings();
    initDeviceIdentity();

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);  // 消灯
    pinMode(PIR_PIN, INPUT_PULLUP);
    ledcAttach(BUZZER_PIN, 4000, 10);  // ブザー初期化(コア3.xのピン指定API)

    // 保存済みWiFiで接続を試みる
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(4, 4);
    M5.Lcd.print("WiFi connecting...");
    if (restoreConfig() && checkConnection()) {
        // 接続成功 → 通常モード
        deviceMode = APP_MODE;
        startWebServer();
        if (MDNS.begin(g_mdnsHost.c_str())) {
            MDNS.addService("http", "tcp", WEB_SERVER_PORT);
        }
        Serial.println("通常モード: http://" + g_mdnsHost + ".local  IP=" + WiFi.localIP().toString());
        // NTPで時刻同期(JST)
        configTime(gmtOffset_sec, 0, "ntp.nict.jp", "ntp.jst.mfeed.ad.jp");
        updateClock();
        last_t = sent_t = t;
        screenMode = SCREEN_MAIN;
        mainDirty = true;
    } else {
        // 未設定 or 接続失敗 → 初期設定モード(アクセスポイント)
        deviceMode = SETUP_MODE;
        setupMode();
        drawSetupScreen();
    }
}

// ============================================================
//  メインループ
// ============================================================
void loop() {
    if (deviceMode == SETUP_MODE) dnsServer.processNextRequest();
    webServer.handleClient();

    M5.update();
    updateBeep();

    // ===== ボタン操作 =====
    // ボタンB 5秒長押し: WiFi設定をリセットして初期設定モードへ
    if (M5.BtnB.wasReleasefor(RESET_HOLD_MS)) {
        M5.Lcd.fillScreen(BLACK);
        M5.Lcd.setTextColor(WHITE, BLACK);
        printEfont("設定リセット中...", 10, 60, 1);
        ringBeep(2000, 300);
        delay(400);
        updateBeep();     // ブザーを止めてから再起動
        resetSettings();  // クリアして再起動
    }
    if (deviceMode == APP_MODE) {
        // 長押し(1秒): 押している途中でビープと共に見守りON/OFFを即切り替え
        static bool longHandled = false;
        if (M5.BtnA.pressedFor(LONG_PRESS_MS) && !longHandled) {
            longHandled = true;
            setMonitoring(!AliveMonitoring);  // ビープはこの中で鳴る
            if (screenMode != SCREEN_MAIN) screenMode = SCREEN_MAIN;
            updateClock();
            updateMainScreen();  // ボタンを離すのを待たずに即画面へ反映
        }
        if (M5.BtnA.wasReleased()) {
            if (longHandled) {
                longHandled = false;  // 長押し処理済みなら短押し動作はしない
            } else {
                // 短押し: 設定ページのQRコード表示 ⇔ メイン画面
                ringBeep(2000, 50);  // 操作音
                if (screenMode == SCREEN_MAIN) {
                    screenMode = SCREEN_INFO;
                    infoScreenSince = millis();
                    drawInfoScreen();
                } else {
                    screenMode = SCREEN_MAIN;
                    mainDirty = true;
                }
            }
        }
        // QR画面は一定時間で自動的にメイン画面へ戻る
        if (screenMode == SCREEN_INFO && millis() - infoScreenSince > INFO_SCREEN_TIMEOUT_MS) {
            screenMode = SCREEN_MAIN;
            mainDirty = true;
        }
    }

    // ===== 監視ロジック(通常モードのみ) =====
    if (deviceMode == APP_MODE) {
        // PIRセンサ読み取り(LEDは生のセンサ値にリアルタイム連動)
        bool raw = digitalRead(PIR_PIN);
        digitalWrite(LED_PIN, raw ? LOW : HIGH);  // LOWで点灯
        if (raw && !rawDetect) Serial.println("Detect!");
        if (!raw && rawDetect) Serial.println("Lost");
        rawDetect = raw;
        if (raw) {
            lastRawDetectMs = millis();
            if (timeValid()) {
                // 未検出通知を送った後に動きを再検出した場合は「生存確認」を通知
                if (LastSent && AliveMonitoring) {
                    long elapsed = (long)difftime(t, last_t);
                    sendIfttt(cfgEventDetect, cfgPlaceName, SecondsToTimeString(elapsed));
                    LastSent = false;
                }
                last_t = sent_t = t;
            }
        }

        // 500msごとの定期処理(時刻更新・画面更新・通知判定)
        static unsigned long lastTickMs = 0;
        if (millis() - lastTickMs >= 500) {
            lastTickMs = millis();
            updateClock();

            if (!AliveMonitoring) {
                // 停止中はタイマーを進めない(再開時にゼロから計測)
                last_t = sent_t = t;
                LastSent = false;
            }

            // 一定時間以上動きが検出されていない場合にIFTTTへ通知
            if (AliveMonitoring && timeValid() && difftime(t, sent_t) >= limitTimeSec()) {
                long total = (long)difftime(t, last_t);
                sendIfttt(cfgEventAlive, cfgPlaceName, SecondsToTimeString(total));
                sent_t = t;  // タイマーリセット(通知の連続送信を防止)
                ringBeep(3000, 500);
                LastSent = true;
            }

            // WiFiが切れていたら定期的に再接続を試みる
            if (WiFi.status() != WL_CONNECTED && millis() - lastWifiRetryMs > WIFI_RETRY_INTERVAL_MS) {
                lastWifiRetryMs = millis();
                Serial.println("WiFi再接続を試行");
                WiFi.reconnect();
            }

            // メイン画面の更新
            if (screenMode == SCREEN_MAIN) updateMainScreen();
        }
    }

    delay(5);
}
