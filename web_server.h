// web_server.h - Webサーバー機能
//  設定モード: キャプティブポータルでWiFi設定(スマホから自宅WiFiを選んで保存)
//  通常モード : 状態表示+監視設定(場所・時間)+IFTTT設定+テスト通知+WiFiリセット

#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <Preferences.h>
#include "config.h"
#include "styles.h"
#include "settings.h"

// ==== 外部変数・関数の宣言 ====
extern String ssidList;
extern int networkCount;
extern DNSServer dnsServer;
extern WebServer webServer;
extern DeviceMode deviceMode;
extern bool AliveMonitoring;

extern void updateNetworkList();
extern void resetSettings();
extern String g_apSsid;    // 個体固有のAP SSID
extern String g_mdnsHost;  // 個体固有のmDNSホスト名(小文字、.local無し)
extern void setMonitoring(bool on);         // 見守りON/OFF(.inoで定義)
extern bool sendIftttTest();                // テスト通知送信(.inoで定義)
extern String lastDetectElapsedString();    // 最終検出からの経過(.inoで定義)

// URLコピー用の共通スクリプト(HTTP=非セキュアコンテキストでも動くようフォールバック付き)
String copyUrlScript() {
    String s = "<script>function copyUrl(){var t=document.getElementById('localurl').textContent;";
    s += "if(navigator.clipboard){navigator.clipboard.writeText(t).then(show).catch(function(){fb(t);});}else{fb(t);}";
    s += "function fb(x){var ta=document.createElement('textarea');ta.value=x;ta.style.position='fixed';ta.style.opacity='0';";
    s += "document.body.appendChild(ta);ta.select();try{document.execCommand('copy');show();}catch(e){}document.body.removeChild(ta);}";
    s += "function show(){document.getElementById('copied').style.display='block';}}</script>";
    return s;
}

// 設定モードのWiFi設定ページ(1画面)。キャプティブポータルが直接これを表示する。
String wifiSetupHtml() {
    String s = "<h1>👀 " + String(APP_TITLE) + "</h1>";
    s += "<div class='sub'>初期設定(1/2): WiFi接続</div>";
    s += "<div class='info'>この本体: <strong>" + g_apSsid + "</strong>(" + g_mdnsHost + ".local)<br>";
    s += "接続するWi-Fiを選んでパスワードを入力してください</div>";
    s += "<form method='get' action='setap'>";
    s += "<div style='display:flex;align-items:center;gap:10px;margin-bottom:8px;'>";
    s += "<label style='flex:1;margin:0;'>ネットワークを選択:</label>";
    s += "<button type='button' onclick='refreshNetworks()' class='btn' style='width:auto;padding:8px 16px;margin:0;font-size:14px;'>🔄 更新</button>";
    s += "</div>";
    s += "<select id='networkSelect' name='ssid' style='margin-bottom:20px;'>" + ssidList + "</select>";
    s += "<label>パスワード:</label>";
    s += "<input name='pass' type='password' placeholder='ネットワークパスワードを入力' maxlength='64'>";

    // 保存後の流れの案内。再起動でこの画面は閉じるため、先に本体URLをコピーさせる。
    String localUrl = "http://" + g_mdnsHost + ".local";
    s += "<div class='howto'>";
    s += "<div class='howto-title'>📍 保存後の設定(先にURLをコピー)</div>";
    s += "<ol>";
    s += "<li>下の本体URLを<b>コピー</b>する</li>";
    s += "<li>「🛜 接続設定を保存」を押す(本体が再起動)</li>";
    s += "<li>スマホを<b>自宅のWi-Fi</b>に接続し直す</li>";
    s += "<li>コピーしたURLを開いて<b>通知の設定</b>をする</li>";
    s += "</ol>";
    s += "<div class='urlrow'><span class='u' id='localurl'>" + localUrl + "</span>";
    s += "<button type='button' class='btn-sm' onclick='copyUrl()'>📋 コピー</button></div>";
    s += "<div id='copied' style='display:none;color:#166534;font-size:13px;margin:4px 0 0;'>コピーしました</div>";
    s += "</div>";

    s += "<button type='submit' class='btn'>🛜 接続設定を保存</button></form>";
    s += "<script>function refreshNetworks(){var btn=event.target;btn.innerHTML='更新中...';btn.disabled=true;";
    s += "fetch('/refresh-networks').then(r=>r.json()).then(d=>{document.getElementById('networkSelect').innerHTML=d.networks;btn.innerHTML='🔄 更新';btn.disabled=false;})";
    s += ".catch(e=>{btn.innerHTML='🔄 更新';btn.disabled=false;alert('更新に失敗しました');});}</script>";
    s += copyUrlScript();
    return makePage("WiFi設定", s);
}

// 通常モードの設定ページ(状態表示+監視設定+IFTTT設定)
String settingsPageHtml() {
    String s = "<h1>👀 " + String(APP_TITLE) + "</h1>";
    s += "<div class='sub'>" + g_mdnsHost + ".local</div>";

    // ===== 状態表示 =====
    s += "<div class='info'>";
    s += "状態: <strong id='monState'>" + String(AliveMonitoring ? "✅ 見守り中" : "⏸ 停止中") + "</strong><br>";
    s += "最終検出: <strong>" + lastDetectElapsedString() + "</strong><br>";
    s += "通知: <strong>" + String(cfgLimitHours) + "時間</strong>動きが無ければ送信";
    if (!hasIftttKey()) {
        s += "<br><span style='color:#b45309;'>⚠ IFTTTキーが未設定のため通知は送信されません</span>";
    }
    s += "<div style='margin-top:10px;padding-top:10px;border-top:1px solid rgba(16,185,129,0.2);font-size:13px;color:#6b7280;'>";
    s += "WiFi: " + WiFi.SSID() + " / IP: " + WiFi.localIP().toString() + "</div>";
    s += "</div>";
    s += "<button type='button' id='toggleBtn' class='btn secondary' onclick='toggleMon()'>" +
         String(AliveMonitoring ? "⏸ 見守りを停止する" : "▶ 見守りを開始する") + "</button>";

    // ===== 監視設定 =====
    s += "<label style='margin-top:20px;'>📍 場所の名前(通知に含まれます):</label>";
    s += "<input id='placeIn' maxlength='32' value='" + cfgPlaceName + "' placeholder='例: リビング' oninput='onMonChange()'>";
    s += "<label>⏰ 通知までの時間:</label>";
    s += "<div style='display:flex;align-items:center;gap:8px;'>";
    s += "<select id='hoursSel' onchange='onMonChange()' style='width:auto;flex:none;margin:0;'>" + limitHoursOptionsHtml() + "</select>";
    s += "<span style='font-size:14px;color:#374151;'>動きが無ければ通知する</span>";
    s += "</div>";
    s += "<button type='button' id='monBtn' class='btn' disabled onclick='saveMon()'>💾 監視設定を保存</button>";

    // ===== IFTTT設定 =====
    // 保存済みのキーはマスク("********")で表示。マスクのまま保存してもキーは変更されない。
    String keyMask = hasIftttKey() ? "********" : "";
    s += "<label style='margin-top:24px;'>🔔 IFTTT設定:</label>";
    s += "<div style='font-size:13px;margin:-2px 0 8px;'><a href='https://ifttt.com/maker_webhooks' target='_blank' rel='noopener' style='color:#059669;'>🔗 Webhooksキーの確認ページを開く</a><span style='color:#6b7280;'>(Documentation をクリックするとキーが表示されます)</span></div>";
    s += "<input id='keyIn' type='password' value='" + keyMask + "' placeholder='IFTTTのWebhooksキーを入力' maxlength='64' oninput='onIftttChange()'>";
    s += "<details><summary>詳細設定(イベント名)</summary>";
    s += "<label>未検出通知のイベント名:</label>";
    s += "<input id='ev1In' value='" + cfgEventAlive + "' maxlength='32' oninput='onIftttChange()'>";
    s += "<label>生存確認通知のイベント名:</label>";
    s += "<input id='ev2In' value='" + cfgEventDetect + "' maxlength='32' oninput='onIftttChange()'>";
    s += "</details>";
    s += "<button type='button' id='iftttBtn' class='btn' disabled onclick='saveIfttt()'>💾 IFTTT設定を保存</button>";
    s += "<button type='button' id='testBtn' class='btn secondary' onclick='sendTest()'" + String(hasIftttKey() ? "" : " disabled") + ">📨 テスト通知を送信</button>";

    // ===== WiFiリセット =====
    s += "<button type='button' class='btn btn-danger' onclick='if(confirm(\"WiFi設定を消去して初期設定モードに戻します。よろしいですか?\"))location.href=\"/reset\";' style='margin-top:24px;'>🔄 WiFi設定をやり直す</button>";

    // ===== スクリプト(Ajax保存+トースト) =====
    s += "<div id='toast' class='toast'></div>";
    s += "<script>";
    s += "var _tt;function toast(m,e){var t=document.getElementById('toast');t.textContent=m;t.className='toast show'+(e?' err':'');clearTimeout(_tt);_tt=setTimeout(function(){t.className='toast'+(e?' err':'');},2600);}";
    s += "var placeBase=document.getElementById('placeIn').value,hoursBase=document.getElementById('hoursSel').value;";
    s += "var keyBase=document.getElementById('keyIn').value;";
    s += "var ev1Base=document.getElementById('ev1In').value,ev2Base=document.getElementById('ev2In').value;";
    s += "function onMonChange(){document.getElementById('monBtn').disabled=(document.getElementById('placeIn').value==placeBase&&document.getElementById('hoursSel').value==hoursBase);}";
    s += "function onIftttChange(){document.getElementById('iftttBtn').disabled=(document.getElementById('keyIn').value==keyBase&&document.getElementById('ev1In').value==ev1Base&&document.getElementById('ev2In').value==ev2Base);}";
    s += "function saveMon(){var p=document.getElementById('placeIn').value.trim(),h=document.getElementById('hoursSel').value,btn=document.getElementById('monBtn');";
    s += "if(!p){toast('場所の名前を入力してください',1);return;}btn.disabled=true;btn.textContent='保存中...';";
    s += "fetch('/setmonitor?place='+encodeURIComponent(p)+'&hours='+encodeURIComponent(h)).then(function(r){if(!r.ok)throw 0;}).then(function(){placeBase=p;hoursBase=h;btn.textContent='💾 監視設定を保存';toast('📍 監視設定を保存しました');}).catch(function(){btn.disabled=false;btn.textContent='💾 監視設定を保存';toast('保存に失敗しました',1);});}";
    s += "function saveIfttt(){var k=document.getElementById('keyIn').value.trim(),e1=document.getElementById('ev1In').value.trim(),e2=document.getElementById('ev2In').value.trim(),btn=document.getElementById('iftttBtn');";
    s += "if(k==keyBase||k=='********')k='';";  // マスクのまま=キー変更なし
    s += "btn.disabled=true;btn.textContent='保存中...';";
    s += "fetch('/setifttt?key='+encodeURIComponent(k)+'&ev1='+encodeURIComponent(e1)+'&ev2='+encodeURIComponent(e2)).then(function(r){if(!r.ok)throw 0;}).then(function(){ev1Base=e1;ev2Base=e2;";
    s += "if(k){keyBase='********';}document.getElementById('keyIn').value=keyBase;";  // 保存後はマスク表示に戻す
    s += "btn.textContent='💾 IFTTT設定を保存';if(keyBase=='********'){document.getElementById('testBtn').disabled=false;}toast('🔔 IFTTT設定を保存しました');}).catch(function(){btn.disabled=false;btn.textContent='💾 IFTTT設定を保存';toast('保存に失敗しました',1);});}";
    s += "function sendTest(){var btn=document.getElementById('testBtn');btn.disabled=true;btn.textContent='送信中...';";
    s += "fetch('/test').then(function(r){if(!r.ok)throw 0;return r.text();}).then(function(x){btn.disabled=false;btn.textContent='📨 テスト通知を送信';if(x=='ok'){toast('📨 テスト通知を送信しました。スマホに届けば設定OKです');}else{toast('送信に失敗しました。キーやネット接続を確認してください',1);}}).catch(function(){btn.disabled=false;btn.textContent='📨 テスト通知を送信';toast('送信に失敗しました',1);});}";
    s += "function toggleMon(){var btn=document.getElementById('toggleBtn');btn.disabled=true;";
    s += "fetch('/toggle').then(function(r){if(!r.ok)throw 0;return r.text();}).then(function(x){var on=(x=='1');document.getElementById('monState').textContent=on?'✅ 見守り中':'⏸ 停止中';btn.textContent=on?'⏸ 見守りを停止する':'▶ 見守りを開始する';btn.disabled=false;toast(on?'✅ 見守りを開始しました':'⏸ 見守りを停止しました');}).catch(function(){btn.disabled=false;toast('切り替えに失敗しました',1);});}";
    s += "</script>";
    return makePage("設定", s);
}

// Webサーバー開始(モードに応じたルート設定)
void startWebServer() {
    if (deviceMode == SETUP_MODE) {
        // ===== 設定モード(アクセスポイント) =====
        Serial.print("Webサーバー開始: ");
        Serial.println(WiFi.softAPIP());
        dnsServer.start(DNS_SERVER_PORT, "*", WiFi.softAPIP());

        // キャプティブポータルは未知のURLを叩くので onNotFound で直接WiFi設定画面を出す
        webServer.onNotFound([]() { webServer.send(200, "text/html", wifiSetupHtml()); });
        webServer.on("/settings", []() { webServer.send(200, "text/html", wifiSetupHtml()); });

        // ネットワーク再スキャン(Ajax)
        webServer.on("/refresh-networks", []() {
            updateNetworkList();
            String escaped = ssidList;
            escaped.replace("\\", "\\\\");
            escaped.replace("\"", "\\\"");
            webServer.send(200, "application/json",
                           "{\"networks\":\"" + escaped + "\",\"count\":" + String(networkCount) + "}");
        });

        // WiFi設定保存 → 再起動(通知設定は接続後に本体URLで行う)
        webServer.on("/setap", []() {
            String ssid = webServer.arg("ssid");
            String pass = webServer.arg("pass");
            Serial.printf("SSID: %s\n", ssid.c_str());

            preferences.begin("wifi-config", false);
            preferences.putString("WIFI_SSID", ssid);
            preferences.putString("WIFI_PASSWD", pass);
            preferences.end();

            String localUrl = "http://" + g_mdnsHost + ".local";
            String s = "<h1>✅ WiFi設定完了</h1>";
            s += "<div class='success'>この本体(<strong>" + g_mdnsHost + ".local</strong>)が再起動して接続します。</div>";
            s += "<div class='howto'>";
            s += "<div class='howto-title'>📍 次に: 通知の設定(2/2)</div>";
            s += "<ol>";
            s += "<li>下の本体URLを<b>コピー</b>する</li>";
            s += "<li>スマホを<b>自宅のWi-Fi</b>に接続し直す</li>";
            s += "<li>コピーしたURLを開いて<b>場所やIFTTTキーを設定</b>する</li>";
            s += "</ol>";
            s += "<div class='howto-note'>本体のボタンを短押しすると、設定ページのQRコードをいつでも表示できます。</div>";
            s += "<div class='urlrow'><span class='u' id='localurl'>" + localUrl + "</span>";
            s += "<button type='button' class='btn-sm' onclick='copyUrl()'>📋 コピー</button></div>";
            s += "<div id='copied' style='display:none;color:#166534;font-size:13px;margin:4px 0 0;'>コピーしました</div>";
            s += "</div>";
            s += "<a href='" + localUrl + "' class='btn' target='_blank' rel='noopener'>🔗 本体URLを開く</a>";
            s += copyUrlScript();
            webServer.send(200, "text/html", makePage("設定完了", s));
            delay(2000);
            ESP.restart();
        });

    } else {
        // ===== 通常モード(WiFi接続済み) =====
        Serial.print("Webサーバー開始: ");
        Serial.println(WiFi.localIP());

        // 設定ページ
        webServer.on("/", []() { webServer.send(200, "text/html", settingsPageHtml()); });

        // 監視設定の保存(Ajax)
        webServer.on("/setmonitor", []() {
            String place = webServer.arg("place");
            int hours = webServer.arg("hours").toInt();
            if (place.length() == 0) {
                webServer.send(400, "text/plain", "place required");
                return;
            }
            saveMonitorSettings(place, hours);
            Serial.printf("監視設定を保存: 場所=%s 時間=%d\n", cfgPlaceName.c_str(), cfgLimitHours);
            webServer.sendHeader("Cache-Control", "no-store");
            webServer.send(200, "text/plain", "ok");
        });

        // IFTTT設定の保存(Ajax)。keyが空またはマスクのままなら既存キーを維持。
        webServer.on("/setifttt", []() {
            String key = webServer.arg("key");
            if (key == "********") key = "";
            saveIftttSettings(key, webServer.arg("ev1"), webServer.arg("ev2"));
            Serial.println("IFTTT設定を保存");
            webServer.sendHeader("Cache-Control", "no-store");
            webServer.send(200, "text/plain", "ok");
        });

        // テスト通知(Ajax)
        webServer.on("/test", []() {
            bool ok = sendIftttTest();
            webServer.sendHeader("Cache-Control", "no-store");
            webServer.send(200, "text/plain", ok ? "ok" : "err");
        });

        // 見守りON/OFF切り替え(Ajax)。新しい状態("1"/"0")を返す。
        webServer.on("/toggle", []() {
            setMonitoring(!AliveMonitoring);
            webServer.sendHeader("Cache-Control", "no-store");
            webServer.send(200, "text/plain", AliveMonitoring ? "1" : "0");
        });

        // WiFi設定リセット
        webServer.on("/reset", []() {
            String s = "<h1>🔄 リセット完了</h1>";
            s += "<div class='success'>WiFi設定をリセットしました。<br>再起動後、初期設定モードになります。</div>";
            webServer.send(200, "text/html", makePage("リセット", s));
            resetSettings();
        });
    }
    webServer.begin();
}

#endif // WEB_SERVER_H
