// styles.h - 設定ページのHTML/CSS生成
// umbrella_alert のデザインに準拠(テーマカラーはグリーン系に変更)。

#ifndef STYLES_H
#define STYLES_H

#include "config.h"

// CSSスタイル生成
String generateCSS() {
    String css = "<style>";
    String P0 = String(THEME_PRIMARY_START);
    String P1 = String(THEME_PRIMARY_END);

    css += "*{box-sizing:border-box;}";

    // 本体
    css += "body{";
    css += "font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,Arial,sans-serif;";
    css += "margin:0;padding:16px;";
    css += "background:linear-gradient(135deg," + P0 + "," + P1 + ");";
    css += "min-height:100vh;color:#1f2937;";
    css += "}";

    // コンテナ
    css += ".container{";
    css += "max-width:" + String(CONTAINER_MAX_WIDTH) + ";margin:0 auto;";
    css += "background:rgba(255,255,255,0.97);border-radius:" + String(BORDER_RADIUS) + ";";
    css += "padding:24px;box-shadow:0 25px 50px rgba(0,0,0,0.25);";
    css += "}";

    // 見出し
    css += "h1{font-size:22px;text-align:center;margin:0 0 4px;color:#2d3748;font-weight:700;}";
    css += ".sub{text-align:center;color:#6b7280;font-size:13px;margin-bottom:16px;}";

    // ラベル
    css += "label{display:block;font-weight:600;font-size:14px;margin:10px 0 6px;color:#374151;}";

    // 入力
    css += "input,select{";
    css += "width:100%;padding:14px;border:2px solid #e2e8f0;border-radius:12px;";
    css += "font-size:16px;font-family:inherit;transition:border-color .2s;box-sizing:border-box;";
    css += "margin-bottom:12px;";
    css += "}";
    css += "input:focus,select:focus{outline:none;border-color:" + P0 + ";box-shadow:0 0 0 3px rgba(16,185,129,0.1);}";

    // 大きいボタン(主要アクション)
    css += ".btn{";
    css += "width:100%;padding:16px;border:none;border-radius:12px;";
    css += "font-size:16px;font-weight:700;font-family:inherit;line-height:1.5;cursor:pointer;margin-top:14px;";
    css += "color:#fff;background:linear-gradient(135deg," + P0 + "," + P1 + ");";
    css += "transition:transform .15s,box-shadow .15s;";
    css += "display:block;text-align:center;text-decoration:none;box-sizing:border-box;";
    css += "}";
    css += ".btn:hover{transform:translateY(-2px);box-shadow:0 8px 25px rgba(0,0,0,0.15);}";
    css += ".btn:active{transform:translateY(1px);}";
    css += ".btn:disabled{background:#cbd5e1;color:#f1f5f9;cursor:not-allowed;box-shadow:none;transform:none;}";
    css += ".btn:disabled:hover{transform:none;box-shadow:none;}";
    css += ".btn.secondary{background:#fff;color:" + P1 + ";border:2px solid " + P0 + ";}";
    css += ".btn-danger{background:linear-gradient(135deg," + String(THEME_DANGER_START) + "," + String(THEME_DANGER_END) + ");}";

    // 小さいボタン(コピー等の補助操作)
    css += ".btn-sm{";
    css += "flex:none;padding:8px 12px;border-radius:10px;border:2px solid " + P0 + ";";
    css += "background:#fff;color:" + P1 + ";font-size:13px;font-weight:700;font-family:inherit;";
    css += "cursor:pointer;white-space:nowrap;";
    css += "}";
    css += ".btn-sm:active{transform:translateY(1px);}";

    // 手順ボックス
    css += ".howto{";
    css += "background:#f8fafc;border:1px solid #e2e8f0;border-radius:12px;";
    css += "padding:8px 14px;margin-bottom:16px;font-size:14px;color:#374151;";
    css += "}";
    css += ".howto-title{font-weight:700;color:#047857;padding:4px 0;}";
    css += ".howto ol{margin:8px 0;padding-left:20px;line-height:1.7;}";
    css += ".howto-note{font-size:12px;color:#6b7280;margin-top:6px;}";

    // URL等の1行表示+小ボタン
    css += ".urlrow{display:flex;align-items:center;gap:8px;margin:8px 0 2px;}";
    css += ".urlrow .u{";
    css += "flex:1;font-size:13px;color:#065f46;word-break:break-all;";
    css += "background:rgba(16,185,129,0.08);border:1px solid #a7f3d0;";
    css += "border-radius:8px;padding:8px 10px;";
    css += "}";

    // 情報・成功メッセージ
    css += ".info{";
    css += "background:rgba(16,185,129,0.1);border:1px solid rgba(16,185,129,0.25);";
    css += "border-radius:10px;padding:10px 12px;margin:14px 0;color:#065f46;";
    css += "font-size:14px;word-break:break-all;";
    css += "}";
    css += ".success{";
    css += "background:rgba(34,197,94,0.12);border:1px solid rgba(34,197,94,0.3);";
    css += "border-radius:10px;padding:12px;margin:14px 0;color:#166534;font-size:14px;";
    css += "}";

    // 詳細設定の折りたたみ
    css += "details{margin:10px 0;}";
    css += "summary{cursor:pointer;font-weight:600;font-size:14px;color:#374151;padding:6px 0;}";

    // フッター
    css += ".footer{text-align:center;margin-top:20px;padding-top:16px;";
    css += "border-top:1px solid #e2e8f0;color:#9ca3af;font-size:12px;}";
    css += ".footer a{color:" + P1 + ";text-decoration:none;}";

    // トースト通知(保存成功などを画面下に短時間表示)
    css += ".toast{position:fixed;left:50%;bottom:24px;transform:translateX(-50%) translateY(20px);";
    css += "background:#1f2937;color:#fff;padding:12px 18px;border-radius:10px;font-size:14px;";
    css += "box-shadow:0 8px 25px rgba(0,0,0,0.25);opacity:0;pointer-events:none;";
    css += "transition:opacity .25s,transform .25s;z-index:1000;max-width:90%;text-align:center;}";
    css += ".toast.show{opacity:1;transform:translateX(-50%) translateY(0);}";
    css += ".toast.err{background:#991b1b;}";

    // レスポンシブ
    css += "@media (max-width:600px){.container{padding:20px;}h1{font-size:20px;}}";

    css += "</style>";
    return css;
}

// フッター生成
String generateFooter() {
    String footer = "<div class='footer'>";
    String author_tag = String(AUTHOR_NAME);
    if (String(AUTHOR_URL) != "") {
        author_tag = "<a href='" + String(AUTHOR_URL) + "' target='_blank'>" + String(AUTHOR_NAME) + "</a>";
    }
    footer += String(APP_TITLE) + " " + String(APP_VERSION) + " · © " + author_tag;
    footer += "</div>";
    return footer;
}

// 基本的なHTMLページ生成
String makePage(String title, String contents) {
    String s = "<!DOCTYPE html><html lang='ja'><head>";
    s += "<meta name='viewport' content='width=device-width,user-scalable=0'>";
    s += "<meta charset='UTF-8'>";
    s += "<title>" + title + " - " + String(APP_TITLE) + "</title>";
    s += "<link rel='icon' href='data:image/svg+xml,<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 100 100\"><text y=\".9em\" font-size=\"90\">👀</text></svg>'>";
    s += generateCSS();
    s += "</head><body>";
    s += "<div class='container'>";
    s += contents;
    s += generateFooter();
    s += "</div>";
    s += "</body></html>";
    return s;
}

#endif // STYLES_H
