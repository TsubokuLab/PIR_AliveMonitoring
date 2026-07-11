// settings.h - 監視設定の保存・読み込み(NVS/Preferences)
// 場所の名前・通知までの時間・IFTTTキー・イベント名を本体に保存する。

#ifndef SETTINGS_H
#define SETTINGS_H

#include <Preferences.h>
#include "config.h"

// 設定保存用(このファイルで一度だけ定義。他ヘッダは extern で参照)
Preferences preferences;

// ===== 現在の設定値(loadAppSettingsで復元) =====
String cfgPlaceName  = DEFAULT_PLACE_NAME;
int    cfgLimitHours = DEFAULT_LIMIT_HOURS;
String cfgIftttKey   = "";
String cfgEventAlive = DEFAULT_EVENT_ALIVE;
String cfgEventDetect = DEFAULT_EVENT_DETECT;

// IFTTTキーが設定済みかどうか(未設定なら通知はスキップされる)
bool hasIftttKey() { return cfgIftttKey.length() > 0; }

// 通知までの秒数
long limitTimeSec() { return (long)cfgLimitHours * 3600; }

// 設定のロード
void loadAppSettings() {
    preferences.begin("pir-monitor", true);
    cfgPlaceName   = preferences.getString("place", DEFAULT_PLACE_NAME);
    cfgLimitHours  = preferences.getInt("limit_h", DEFAULT_LIMIT_HOURS);
    cfgIftttKey    = preferences.getString("ifttt_key", "");
    cfgEventAlive  = preferences.getString("ev_alive", DEFAULT_EVENT_ALIVE);
    cfgEventDetect = preferences.getString("ev_detect", DEFAULT_EVENT_DETECT);
    preferences.end();
    if (cfgLimitHours < 1 || cfgLimitHours > 72) cfgLimitHours = DEFAULT_LIMIT_HOURS;
}

// 監視設定(場所・時間)の保存
void saveMonitorSettings(const String& place, int limitHours) {
    if (limitHours < 1) limitHours = 1;
    if (limitHours > 72) limitHours = 72;
    preferences.begin("pir-monitor", false);
    preferences.putString("place", place);
    preferences.putInt("limit_h", limitHours);
    preferences.end();
    cfgPlaceName = place;
    cfgLimitHours = limitHours;
}

// IFTTT設定の保存(キーが空文字列の場合はキーを変更しない=マスク表示対応)
void saveIftttSettings(const String& key, const String& evAlive, const String& evDetect) {
    preferences.begin("pir-monitor", false);
    if (key.length() > 0) {
        preferences.putString("ifttt_key", key);
        cfgIftttKey = key;
    }
    if (evAlive.length() > 0) {
        preferences.putString("ev_alive", evAlive);
        cfgEventAlive = evAlive;
    }
    if (evDetect.length() > 0) {
        preferences.putString("ev_detect", evDetect);
        cfgEventDetect = evDetect;
    }
    preferences.end();
}

// 設定ページの「通知までの時間」ドロップダウン用 <option> 群
String limitHoursOptionsHtml() {
    // よく使いそうな時間だけを候補に(1〜72時間)
    const int opts[] = {1, 2, 3, 6, 12, 24, 36, 48, 72};
    const int n = sizeof(opts) / sizeof(opts[0]);
    String s = "";
    bool matched = false;
    for (int i = 0; i < n; i++) {
        s += "<option value='" + String(opts[i]) + "'";
        if (opts[i] == cfgLimitHours) { s += " selected"; matched = true; }
        s += ">" + String(opts[i]) + " 時間</option>";
    }
    // 候補に無い値が保存されている場合は先頭に追加
    if (!matched) {
        s = "<option value='" + String(cfgLimitHours) + "' selected>" +
            String(cfgLimitHours) + " 時間</option>" + s;
    }
    return s;
}

#endif // SETTINGS_H
