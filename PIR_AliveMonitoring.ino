// ============================================================
//  PIR_AliveMonitoring
//  M5StickC Plus + PIRセンサによる独り暮らし向け「死活監視システム」
//
//  一定時間(デフォルト24時間)動きが検出されなかった場合に
//  IFTTT経由で通知を送ります。動きが再検出された際には
//  「生存確認」の通知を送ります。
//
//  https://github.com/TsubokuLab/PIR_AliveMonitoring
//  https://protopedia.net/prototype/2432
// ============================================================
#include <WiFi.h>
#include <WiFiClient.h>
#include <M5StickCPlus.h>
#include "efont.h"
#include "efontM5StickCPlus.h"
#include "efontEnableJaMini.h"
#include "time.h"
#include "config.h" // WiFi・IFTTT・監視設定(config.h.exampleをコピーして作成)

#define DAY_SEC 86400
#define HOUR_SEC 3600

// IFTTT設定
const char* server = "maker.ifttt.com"; // IFTTT Webhooks サーバー
WiFiClient client;

// 時刻管理
const long gmtOffset_sec = 3600 * 9; // JST (UTC+9)
const int daylightOffset_sec = 0;
time_t t, sent_t, last_t;
struct tm timeinfo;

// ピン設定
const int PIR_PIN = 36;      // PIR Hat の信号ピン (G36)
const int LED_PIN = 10;      // 本体内蔵の赤色LED (LOWで点灯)

// 状態フラグ
bool detect = false, prev_detect = false;
bool AliveMonitoring = true; // 見守り中かどうか(ボタンAで切り替え)
bool LastSent = false;       // 未検出通知を送信済みかどうか
bool force_redraw = true;    // 起動直後の初回描画用

// WiFi接続待ち
bool checkWifiConnected() {
  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setCursor(0, 0);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    M5.Lcd.print(".");
    delay(500);
  }
  Serial.print("Connected to ");
  Serial.println(WIFI_SSID);
  M5.Lcd.println("WiFi connected");
  M5.Lcd.print("IP address = ");
  M5.Lcd.println(WiFi.localIP());

  delay(1000);
  return true;
}

// NTPサーバーから現在時刻を取得
void checkLocalTime() {
  while (!getLocalTime(&timeinfo)) {
    M5.Lcd.println("Failed to obtain time");
    Serial.println("Failed to obtain time");
    configTime(gmtOffset_sec, daylightOffset_sec, "ntp.nict.jp", "ntp.jst.mfeed.ad.jp");
    delay(1000);
  }
  time(&t);
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

// IFTTT Webhooksへ通知を送信
void send(String trigger, String value1, String value2) {
  while (!checkWifiConnected()) {
    Serial.print("Attempting to connect to WiFi");
    WiFi.begin(WIFI_SSID, WIFI_PASS);
  }

  Serial.println("\nStarting connection to server...");
  if (!client.connect(server, 80)) {
    Serial.println("Connection failed!");
  } else {
    Serial.println("Connected to server!");
    String url = "/trigger/" + trigger + "/with/key/" + IFTTT_KEY;
    url += "?value1=" + urlEncode(value1) + "&value2=" + urlEncode(value2);
    client.println("GET " + url + " HTTP/1.1");
    client.print("Host: ");
    client.println(server);
    client.println("Connection: close");
    client.println();
    Serial.print("Waiting for response ");

    // 応答が無い場合は10秒でタイムアウト(本体が固まるのを防止)
    unsigned long start_ms = millis();
    while (!client.available()) {
      if (millis() - start_ms > 10000) {
        Serial.println("\nResponse timeout.");
        client.stop();
        break;
      }
      delay(50);
      Serial.print(".");
    }
    while (client.available()) {
      char c = client.read();
      Serial.write(c);
    }

    if (!client.connected()) {
      Serial.println();
      Serial.println("disconnecting from server.");
      client.stop();
    }
  }
  // 送信中に画面へ描かれたWiFi接続メッセージを消して再描画させる
  M5.Lcd.fillScreen(BLACK);
  force_redraw = true;
}

void setup() {
  M5.begin();
  M5.Lcd.setRotation(3);
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.fillScreen(BLACK);

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (!checkWifiConnected()) {
    WiFi.begin(WIFI_SSID, WIFI_PASS);
  }
  delay(1000);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  AliveMonitoring = true;
  // 現在時刻を取得してタイマーを初期化
  checkLocalTime();
  last_t = sent_t = t;
  pinMode(PIR_PIN, INPUT_PULLUP);
  // 起動時のWiFi接続メッセージを消してから通常画面へ
  M5.Lcd.fillScreen(BLACK);
}

void loop() {
  M5.update();
  M5.Beep.update();
  // ボタンAで見守りのON/OFFを切り替え
  if (M5.BtnA.wasPressed()) {
    ringBeep(3000, 100);
    AliveMonitoring = !AliveMonitoring;
  }
  // 見守り状態の表示(ちらつき防止のため状態が変わったときだけ再描画)
  static bool prev_alive = true;
  if (AliveMonitoring != prev_alive || force_redraw) {
    if (AliveMonitoring) {
      M5.Lcd.fillRect(0, 0, M5.Lcd.width(), 40, GREEN);
      M5.Lcd.setTextSize(2);
      M5.Lcd.setTextColor(BLACK, GREEN);
      printEfont("見守り中", 55, 4, 2);
    } else {
      M5.Lcd.fillRect(0, 0, M5.Lcd.width(), 40, RED);
      M5.Lcd.setTextSize(2);
      M5.Lcd.setTextColor(BLACK, RED);
      printEfont("停止中", 70, 4, 2);
    }
    prev_alive = AliveMonitoring;
  }
  if (!AliveMonitoring) {
    // 停止中はタイマーを進めない(再開時にゼロから計測)
    last_t = sent_t = t;
    LastSent = false;
  }
  // PIRセンサ読み取り
  detect = digitalRead(PIR_PIN);
  digitalWrite(LED_PIN, detect ? LOW : HIGH); // LOWで点灯
  // 検出状態の表示(こちらも変化したときだけ再描画)
  if (detect != prev_detect || force_redraw) {
    if (detect) {
      M5.Lcd.fillRect(0, 80, M5.Lcd.width(), 80, GREEN);
      M5.Lcd.setTextColor(BLACK, GREEN);
      printEfont("検出", 70, 85, 3);
    } else {
      M5.Lcd.fillRect(0, 80, M5.Lcd.width(), 80, BLUE);
      M5.Lcd.setTextColor(WHITE, BLUE);
      printEfont("未検出", 45, 85, 3);
    }
    Serial.println(detect ? "Detect!" : "Lost");
  }
  prev_detect = detect;
  force_redraw = false; // ここから先でsend()が呼ばれると再度trueになり、次のループで再描画される
  if (detect) {
    // 未検出通知を送った後に動きを再検出した場合は「生存確認」を通知
    if (LastSent) {
      int _seconds = difftime(t, last_t);
      send(EVENT_DETECT, PLACE_NAME, SecondsToTimeString(_seconds));
      LastSent = false;
    }
    last_t = sent_t = t;
  }
  // 一定時間以上動きが検出されていない場合にIFTTTへ通知
  double diff = difftime(t, sent_t);
  double diff_total = difftime(t, last_t);
  if (AliveMonitoring && diff >= LIMIT_TIME_SEC) {
    send(EVENT_ALIVE, PLACE_NAME, SecondsToTimeString((int)diff_total));
    sent_t = t; // タイマーリセット(通知の連続送信を防止)
    ringBeep(3000, 500);
    LastSent = true;
  }
  // 画面の時刻表示を更新
  drawTime();
  delay(500);
}

// 画面中央に現在時刻を表示(秒が変わったときだけ更新してちらつきを防止)
void drawTime() {
  static int prev_sec = -1;
  checkLocalTime();
  if (timeinfo.tm_sec == prev_sec) return;
  prev_sec = timeinfo.tm_sec;
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(WHITE, BLACK); // 背景色付きで上書き描画(fillRect不要)
  M5.Lcd.setCursor(6, 54);
  M5.Lcd.printf("%04d-%02d-%02d %02d:%02d:%02d\n",
                timeinfo.tm_year + 1900,
                timeinfo.tm_mon + 1,
                timeinfo.tm_mday,
                timeinfo.tm_hour,
                timeinfo.tm_min,
                timeinfo.tm_sec);
}

// 経過秒数を「3日と5時間」のような文字列に変換
String SecondsToTimeString(int _seconds) {
  String _txt = "";
  int _days = floor(_seconds / DAY_SEC);
  if (_days > 0) {
    _txt = String(_days) + "日";
    int _hour = (int)(floor(_seconds % DAY_SEC) / HOUR_SEC);
    if (_hour >= 1) _txt += "と" + String(_hour) + "時間";
  } else {
    _txt = String((int)(_seconds / HOUR_SEC)) + "時間";
  }
  return _txt;
}

// ブザーを鳴らす
void ringBeep(int pwm, int delaytime) {
  M5.Beep.tone(pwm, delaytime);
}
