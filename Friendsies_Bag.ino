#include <WiFi.h>
#include <WebServer.h>
#include <HX711_ADC.h>

// ==========================================
// Network Configuration
// ==========================================
const char* ssid     = "Friendsies_Bag";
const char* password = "12345678";

WebServer server(80);

// ==========================================
// Hardware Pin Configuration
// ==========================================
const int HX711_dout = 4;
const int HX711_sck  = 5;
const int buzzerPin  = 15;

HX711_ADC LoadCell(HX711_dout, HX711_sck);

// ==========================================
// Moving Average Filter
// ==========================================
const int   numReadings     = 20;
float       readings[numReadings];
int         readIndex       = 0;
float       total           = 0;
float       stableValue     = 0;
float       lastStableValue = 0;
const float threshold       = 2.0;
unsigned long t             = 0;
boolean     isMeasuring     = false;

// ==========================================
// HTML Page — Friendsies Bag (v8 — centered mobile)
// ==========================================
const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0,
        maximum-scale=1.0, user-scalable=no">
  <title>Friendsies Bag</title>
  <style>
    *, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }
    :root {
      --bg:      #eef0e2;
      --surface: #ffffff;
      --surface2:#d4dbc4;
      --border:  rgba(0,0,0,0.08);
      --text:    #2d3a2a;
      --muted:   #6b7560;
      --blue:    #2e9d4a;
      --green:   #2e9d4a;
      --amber:   #d68f1f;
      --red:     #c4452d;
      --purple:  #8b6fc7;
      --gold:    #d68f1f;
    }
    body {
      background: var(--bg); color: var(--text);
      font-family: system-ui,-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;
      min-height: 100vh;
      display: flex;
      flex-direction: column;
      align-items: center;
    }

    /* ── Page system ── */
    .page {
      display: none; flex-direction: column; align-items: center;
      padding: 24px 20px 44px; min-height: 100vh;
      width: 100%; max-width: 420px;
      animation: fadeIn .3s ease;
    }
    .page.active { display: flex; }
    @keyframes fadeIn {
      from { opacity:0; transform:translateY(8px); }
      to   { opacity:1; transform:translateY(0);   }
    }

    /* ══════════════════════════════════════
       LOGIN PAGE
    ══════════════════════════════════════ */
    #login-page {
      justify-content: center;
      background: radial-gradient(ellipse at 50% 0%, rgba(46,157,74,.08) 0%, transparent 70%),
                  var(--bg);
    }

    .login-brand {
      text-align: center;
      margin-bottom: 36px;
    }
    .login-logo-wrap {
      width: 80px; height: 80px; border-radius: 24px;
      background: linear-gradient(135deg, rgba(46,157,74,.2), rgba(139,111,199,.2));
      border: 1px solid rgba(46,157,74,.25);
      display: flex; align-items: center; justify-content: center;
      font-size: 36px; margin: 0 auto 16px;
      box-shadow: 0 0 40px rgba(46,157,74,.1);
    }
    .login-app-name {
      font-size: 28px; font-weight: 900;
      background: linear-gradient(135deg, var(--blue), var(--purple));
      -webkit-background-clip: text; color: transparent;
      letter-spacing: -0.5px;
    }
    .login-tagline {
      font-size: 12px; color: var(--muted);
      letter-spacing: 2px; text-transform: uppercase;
      margin-top: 6px;
    }

    .login-card {
      background: var(--surface);
      border: 1px solid var(--border);
      border-radius: 28px;
      padding: 32px 28px;
      width: 100%; max-width: 360px;
    }
    .login-title {
      font-size: 18px; font-weight: 700;
      margin-bottom: 6px;
    }
    .login-sub {
      font-size: 12px; color: var(--muted);
      margin-bottom: 24px; line-height: 1.6;
    }

    .field-label {
      font-size: 11px; font-weight: 700; letter-spacing: 1.5px;
      text-transform: uppercase; color: var(--muted);
      margin-bottom: 8px; display: block;
    }
    .field-input {
      width: 100%; background: var(--surface2);
      border: 1px solid var(--border);
      border-radius: 14px; padding: 14px 16px;
      font-size: 15px; color: var(--text);
      outline: none; margin-bottom: 16px;
      transition: border-color .2s, box-shadow .2s;
      -webkit-appearance: none;
    }
    .field-input:focus {
      border-color: rgba(46,157,74,.5);
      box-shadow: 0 0 0 3px rgba(46,157,74,.1);
    }
    .field-input::placeholder { color: var(--muted); }

    .login-error {
      background: rgba(196,69,45,.08);
      border: 1px solid rgba(196,69,45,.3);
      border-radius: 12px; padding: 10px 14px;
      font-size: 12px; font-weight: 600; color: var(--red);
      margin-bottom: 16px;
      display: none;
    }
    .login-error.show { display: block; }

    .btn-login {
      width: 100%; padding: 17px;
      background: linear-gradient(135deg, var(--green), #267d3b);
      border: none; border-radius: 16px;
      font-size: 16px; font-weight: 700; color: #ffffff;
      cursor: pointer; letter-spacing: 0.3px;
      transition: transform .15s, filter .15s;
      margin-top: 4px;
    }
    .btn-login:active { transform: scale(.97); filter: brightness(.9); }

    .login-version {
      margin-top: 24px;
      font-size: 11px; color: var(--muted);
      text-align: center; letter-spacing: 1px;
    }

    /* ── Header ── */
    .header { text-align: center; margin-bottom: 24px; width: 100%; max-width: 360px; }
    .logo   { font-size:11px; font-weight:600; letter-spacing:3px;
              text-transform:uppercase; color:var(--muted); margin-bottom:6px; }
    .title  { font-size:26px; font-weight:800;
              background:linear-gradient(135deg,var(--green),var(--purple));
              -webkit-background-clip:text; color:transparent; }

    /* ── Top bar with settings icon ── */
    .top-bar {
      display: flex; justify-content: space-between; align-items: flex-start;
      width: 100%; max-width: 360px; margin-bottom: 20px;
    }
    .top-bar .header { margin-bottom: 0; text-align: left; flex: 1; }
    .btn-settings {
      background: var(--surface2); border: 1px solid var(--border);
      border-radius: 14px; padding: 10px 14px;
      display: flex; align-items: center; gap: 6px;
      font-size: 12px; font-weight: 600; color: var(--muted);
      cursor: pointer; white-space: nowrap; flex-shrink: 0;
      transition: color .2s, border-color .2s, background .2s;
      margin-top: 4px;
    }
    .btn-settings:hover {
      color: var(--text);
      border-color: rgba(0,0,0,.15);
      background: var(--surface);
    }

    /* ── Status pill ── */
    .status-pill {
      display:inline-flex; align-items:center; gap:7px;
      margin-top:10px; padding:6px 16px; border-radius:20px;
      font-size:12px; font-weight:500;
      background:var(--surface2); color:var(--muted);
      border:1px solid var(--border); transition:all .3s;
    }
    .status-dot {
      width:6px; height:6px; border-radius:50%;
      background:var(--muted); transition:all .3s;
    }
    .status-pill.running {
      color:#7a5a00; border-color:rgba(214,143,31,.4);
      background:rgba(214,143,31,.12);
    }
    .status-pill.running .status-dot {
      background:#d68f1f; box-shadow:0 0 6px #d68f1f;
      animation:blink 1.2s infinite;
    }
    .status-pill.overlimit {
      color:var(--red); border-color:rgba(196,69,45,.4);
      background:rgba(196,69,45,.08);
    }
    .status-pill.overlimit .status-dot {
      background:var(--red); box-shadow:0 0 6px var(--red);
      animation:blink .5s infinite;
    }
    @keyframes blink { 0%,100%{opacity:1} 50%{opacity:.15} }

    /* ── Gauge card ── */
    .gauge-card {
      background:var(--surface2); border:1px solid var(--border);
      border-radius:28px; padding:32px 24px 28px;
      width:100%; max-width:360px; text-align:center;
      transition:border-color .4s, box-shadow .4s;
    }
    .gauge-card.measuring {
      border-color:rgba(46,157,74,.45);
      box-shadow:0 0 40px rgba(46,157,74,.1);
    }
    .gauge-card.overlimit {
      border-color:rgba(196,69,45,.5);
      box-shadow:0 0 40px rgba(196,69,45,.15);
    }

    /* ── SVG gauge ── */
    .gauge-wrap {
      position:relative; width:220px; height:220px; margin:0 auto 6px;
    }
    .gauge-wrap svg { width:100%; height:100%; overflow:visible; }
    #gauge-fill {
      fill:none; stroke:#2e9d4a; stroke-width:14; stroke-linecap:round;
      transition: stroke-dashoffset .45s cubic-bezier(.4,0,.2,1), stroke .3s;
    }
    .gauge-centre {
      position:absolute; inset:0;
      display:flex; flex-direction:column;
      align-items:center; justify-content:center; gap:2px;
    }
    .gauge-val {
      font-size:46px; font-weight:900; letter-spacing:-2px;
      line-height:1; font-variant-numeric:tabular-nums;
      color:var(--green); transition:color .3s;
    }
    .gauge-val.running   { color:var(--green); }
    .gauge-val.overlimit { color:var(--red);  }
    .gauge-unit {
      font-size:11px; font-weight:600; letter-spacing:3px;
      text-transform:uppercase; color:var(--muted);
    }
    .gauge-minmax {
      display:flex; justify-content:space-between;
      width:220px; margin:0 auto 4px;
      font-size:10px; color:var(--muted); font-weight:500;
    }

    /* ── Alert strip ── */
    .alert-strip {
      margin-top:14px; padding:10px 14px; border-radius:10px;
      background:rgba(196,69,45,.08); border:1px solid rgba(196,69,45,.3);
      color:var(--red); font-size:13px; font-weight:600;
      opacity:0; transform:translateY(6px);
      transition:all .3s; pointer-events:none;
    }
    .alert-strip.show { opacity:1; transform:translateY(0); }

    /* ── Controls ── */
    .controls {
      display:flex; gap:12px;
      width:100%; max-width:360px; margin-top:20px;
    }
    .btn {
      flex:1; padding:17px 0; border:none; border-radius:16px;
      font-size:15px; font-weight:700; color:#fff; cursor:pointer;
      display:flex; align-items:center; justify-content:center; gap:8px;
      transition:transform .15s, filter .15s, opacity .25s;
    }
    .btn:active  { transform:scale(.96); filter:brightness(.88); }
    .btn-start   { background:linear-gradient(135deg,#3aa856,#267d3b); }
    .btn-stop    { background:linear-gradient(135deg,#c4452d,#9c3422); }
    .btn:disabled {
      opacity:.3; cursor:not-allowed; filter:grayscale(.7);
      transform:none !important;
    }

    /* ── Secondary action buttons row ── */
    .secondary-btns {
      display:grid; grid-template-columns:1fr 1fr;
      gap:12px; width:100%; max-width:360px; margin-top:12px;
    }
    .btn-secondary {
      padding:15px 0; border-radius:16px;
      font-size:14px; font-weight:700; cursor:pointer;
      display:flex; align-items:center; justify-content:center; gap:8px;
      transition:transform .15s, background .2s, border-color .2s;
    }
    .btn-secondary:active { transform:scale(.97); }
    .btn-trips {
      border:1px solid rgba(139,111,199,.3);
      color:var(--purple); background:rgba(139,111,199,.06);
    }
    .btn-trips:hover { background:rgba(139,111,199,.12); border-color:rgba(139,111,199,.5); }
    .btn-trophy {
      border:1px solid rgba(214,143,31,.3);
      color:var(--gold); background:rgba(214,143,31,.06);
    }
    .btn-trophy:hover { background:rgba(214,143,31,.12); border-color:rgba(214,143,31,.5); }
    .sec-badge {
      font-size:10px; font-weight:800;
      padding:2px 7px; border-radius:20px; min-width:20px; text-align:center;
    }
    .trips-badge  { background:var(--purple); color:#ffffff; }
    .trophy-badge { background:var(--gold);   color:#ffffff; }

    /* ── Final label ── */
    .final-label {
      min-height:26px; margin-top:12px;
      font-size:13px; font-weight:500; color:var(--amber); text-align:center;
    }

    /* ── Impact section ── */
    .impact-section { width:100%; max-width:360px; margin-top:28px; }
    .section-label  {
      font-size:10px; font-weight:700; letter-spacing:2.5px;
      text-transform:uppercase; color:var(--muted);
      margin-bottom:12px; text-align:center;
    }
    .impact-grid { display:grid; grid-template-columns:1fr 1fr 1fr; gap:10px; }
    .impact-btn {
      background:var(--surface); border:1px solid var(--border);
      border-radius:16px; padding:16px 6px; cursor:pointer;
      display:flex; flex-direction:column; align-items:center; gap:6px;
      transition:border-color .2s, background .2s, transform .15s;
      text-align:center;
    }
    .impact-btn:active { transform:scale(.95); }
    .impact-btn .ib-icon  { font-size:22px; }
    .impact-btn .ib-label { font-size:10px; font-weight:600; line-height:1.3; }
    .impact-btn .ib-arrow { font-size:10px; color:var(--muted); margin-top:2px; }
    .impact-btn.green { border-color:rgba(46,157,74,.25); }
    .impact-btn.green:hover { background:rgba(46,157,74,.07); border-color:rgba(46,157,74,.5); }
    .impact-btn.green .ib-label { color:var(--green); }
    .impact-btn.blue  { border-color:rgba(46,157,74,.25); }
    .impact-btn.blue:hover  { background:rgba(46,157,74,.07); border-color:rgba(46,157,74,.5); }
    .impact-btn.blue .ib-label  { color:var(--green); }
    .impact-btn.amber { border-color:rgba(214,143,31,.25); }
    .impact-btn.amber:hover { background:rgba(214,143,31,.07); border-color:rgba(214,143,31,.5); }
    .impact-btn.amber .ib-label { color:var(--amber); }

    /* ── Detail / generic back page ── */
    .detail-page { padding:24px 20px 44px; }
    .back-btn {
      display:inline-flex; align-items:center; gap:7px;
      background:var(--surface); border:1px solid var(--border);
      border-radius:12px; padding:9px 16px;
      font-size:13px; font-weight:500; color:var(--muted);
      cursor:pointer; margin-bottom:24px;
      transition:color .2s, border-color .2s;
    }
    .back-btn:hover { color:var(--text); border-color:rgba(0,0,0,.15); }
    .detail-card {
      background:var(--surface); border:1px solid var(--border);
      border-radius:24px; padding:28px;
      width:100%; max-width:360px;
    }
    .d-icon-wrap {
      width:56px; height:56px; border-radius:16px;
      display:flex; align-items:center; justify-content:center;
      font-size:26px; margin-bottom:16px;
    }
    .d-icon-wrap.green { background:rgba(46,157,74,.12); }
    .d-icon-wrap.blue  { background:rgba(46,157,74,.12); }
    .d-icon-wrap.amber { background:rgba(214,143,31,.12); }
    .d-title    { font-size:20px; font-weight:700; margin-bottom:4px; }
    .d-subtitle { font-size:13px; color:var(--muted); margin-bottom:22px; line-height:1.55; }
    .result-big { margin:0 0 20px; padding:26px; border-radius:16px; text-align:center; }
    .result-big.green { background:rgba(46,157,74,.08); border:1px solid rgba(46,157,74,.25); }
    .result-big.blue  { background:rgba(46,157,74,.08); border:1px solid rgba(46,157,74,.25); }
    .result-big.amber { background:rgba(214,143,31,.08); border:1px solid rgba(214,143,31,.25); }
    .result-value { font-size:56px; font-weight:900; letter-spacing:-2px; line-height:1; }
    .result-value.green { color:var(--green); }
    .result-value.blue  { color:var(--green);  }
    .result-value.amber { color:var(--amber); }
    .result-unit {
      font-size:12px; font-weight:600; letter-spacing:1.5px;
      text-transform:uppercase; color:var(--muted); margin-top:8px;
    }
    .formula-box { background:var(--surface2); border-radius:14px; padding:16px 18px; }
    .formula-title {
      font-size:10px; font-weight:700; letter-spacing:2px;
      text-transform:uppercase; color:var(--muted); margin-bottom:12px;
    }
    .f-row {
      display:flex; justify-content:space-between; align-items:center;
      padding:7px 0; border-bottom:1px solid var(--border); font-size:13px;
    }
    .f-row:last-child { border:none; font-weight:700; color:var(--text); }
    .f-key { color:var(--muted); }
    .no-data {
      text-align:center; padding:28px 10px;
      font-size:14px; color:var(--muted); line-height:1.7;
    }

    /* ══════════════════════════════════════
       TRIPS PAGE
    ══════════════════════════════════════ */
    #trips-page { align-items:center; }
    .trips-header {
      width:100%; max-width:360px;
      display:flex; align-items:center; justify-content:space-between;
      margin-bottom:20px;
    }
    .trips-title-block .trips-page-title { font-size:20px; font-weight:800; color:var(--green); }
    .trips-title-block .trips-page-sub   { font-size:12px; color:var(--muted); margin-top:2px; }
    .btn-clear-all {
      background:var(--surface); border:1px solid rgba(196,69,45,.4);
      border-radius:10px; padding:8px 13px;
      font-size:12px; font-weight:600; color:var(--red);
      cursor:pointer; transition:background .2s; white-space:nowrap;
    }
    .btn-clear-all:hover { background:rgba(196,69,45,.1); }
    .trips-summary {
      width:100%; max-width:360px;
      display:grid; grid-template-columns:1fr 1fr 1fr;
      gap:10px; margin-bottom:20px;
    }
    .sum-card {
      background:var(--surface); border:1px solid var(--border);
      border-radius:16px; padding:14px 10px; text-align:center;
    }
    .sum-val  { font-size:20px; font-weight:800; line-height:1; }
    .sum-val.purple { color:var(--purple); }
    .sum-val.green  { color:var(--green);  }
    .sum-val.amber  { color:var(--amber);  }
    .sum-lbl  { font-size:9px; font-weight:600; letter-spacing:1.5px;
                text-transform:uppercase; color:var(--muted); margin-top:5px; }
    .trips-list { width:100%; max-width:360px; display:flex; flex-direction:column; gap:12px; }
    .trip-card {
      background:var(--surface2); border:1px solid var(--border);
      border-radius:18px; padding:18px 18px 14px;
      animation: fadeIn .25s ease; position:relative; overflow:hidden;
    }
    .trip-card::before {
      content:''; position:absolute; left:0; top:0; bottom:0;
      width:3px; background:var(--purple); border-radius:3px 0 0 3px;
    }
    .trip-top {
      display:flex; justify-content:space-between; align-items:flex-start;
      margin-bottom:12px;
    }
    .trip-num  { font-size:10px; font-weight:700; letter-spacing:2px; text-transform:uppercase; color:var(--purple); }
    .trip-date { font-size:11px; color:var(--muted); margin-top:2px; }
    .trip-delete {
      background:var(--surface); border:1px solid rgba(196,69,45,.3);
      border-radius:8px; padding:5px 10px;
      font-size:11px; font-weight:600; color:var(--red);
      cursor:pointer; transition:background .2s; flex-shrink:0;
    }
    .trip-delete:hover { background:rgba(196,69,45,.1); }
    .trip-weight { font-size:32px; font-weight:900; letter-spacing:-1.5px; color:var(--text); line-height:1; }
    .trip-weight span { font-size:14px; font-weight:500; color:var(--muted); letter-spacing:0; }
    .trip-stats { display:flex; gap:8px; margin-top:12px; flex-wrap:wrap; }
    .trip-stat {
      background:var(--surface); border-radius:8px;
      padding:5px 10px; font-size:11px; font-weight:600;
      display:flex; align-items:center; gap:5px;
    }
    .trip-stat.g { color:var(--green); }
    .trip-stat.b { color:var(--green);  }
    .trip-stat.a { color:var(--amber); }
    .trips-empty {
      width:100%; max-width:360px;
      text-align:center; padding:48px 20px;
      color:var(--muted); font-size:14px; line-height:1.8;
    }
    .trips-empty .empty-icon { font-size:40px; margin-bottom:12px; }

    /* ══════════════════════════════════════
       TROPHY ROOM PAGE
    ══════════════════════════════════════ */
    #trophy-page { align-items:center; }
    .trophy-header { width:100%; max-width:360px; margin-bottom:8px; }
    .trophy-page-title {
      font-size:22px; font-weight:800;
      background:linear-gradient(135deg, var(--gold), #b06d10);
      -webkit-background-clip:text; color:transparent;
      display:flex; align-items:center; gap:10px;
    }
    .trophy-page-sub { font-size:12px; color:var(--muted); margin-top:4px; line-height:1.6; }
    .trophy-progress {
      width:100%; max-width:360px;
      background:var(--surface); border:1px solid var(--border);
      border-radius:18px; padding:16px 18px; margin:16px 0 20px;
    }
    .tp-row { display:flex; justify-content:space-between; align-items:center; margin-bottom:8px; }
    .tp-label { font-size:11px; font-weight:600; color:var(--muted); }
    .tp-val   { font-size:11px; font-weight:700; color:var(--gold); }
    .tp-bar-bg { height:6px; border-radius:6px; background:var(--surface2); overflow:hidden; }
    .tp-bar-fill {
      height:100%; border-radius:6px;
      background:linear-gradient(90deg,var(--gold),#b06d10);
      transition:width .6s cubic-bezier(.4,0,.2,1);
    }
    .badge-grid { width:100%; max-width:360px; display:grid; grid-template-columns:1fr 1fr; gap:14px; }
    .badge-card {
      background:var(--surface); border:1px solid var(--border);
      border-radius:20px; padding:20px 16px 18px;
      display:flex; flex-direction:column; align-items:center;
      text-align:center; gap:0;
      transition:transform .2s, border-color .3s, box-shadow .3s;
      position:relative; overflow:hidden;
    }
    .badge-card.earned {
      border-color:rgba(214,143,31,.4);
      box-shadow:0 0 24px rgba(214,143,31,.1);
      background:linear-gradient(145deg, var(--surface), rgba(214,143,31,.05));
    }
    .badge-card.earned:active { transform:scale(.97); }
    .badge-card.earned::after {
      content:''; position:absolute; top:-50%; left:-60%;
      width:40%; height:200%;
      background:linear-gradient(105deg,transparent,rgba(255,255,255,.4),transparent);
      animation:shimmer 3s infinite;
    }
    @keyframes shimmer { 0%{left:-60%} 60%{left:130%} 100%{left:130%} }
    .badge-icon-wrap {
      width:64px; height:64px; border-radius:20px;
      display:flex; align-items:center; justify-content:center;
      font-size:30px; margin-bottom:12px; transition:background .3s;
    }
    .badge-card.locked .badge-icon-wrap { background:var(--surface2); filter:grayscale(1) opacity(.45); }
    .badge-card.earned .badge-icon-wrap { background:rgba(214,143,31,.15); }
    .badge-name { font-size:13px; font-weight:700; margin-bottom:4px; color:var(--text); }
    .badge-card.locked .badge-name { color:var(--muted); }
    .badge-desc { font-size:10px; line-height:1.5; color:var(--muted); margin-bottom:10px; }
    .badge-progress {
      font-size:10px; font-weight:700; letter-spacing:.5px;
      padding:4px 10px; border-radius:20px; transition:background .3s, color .3s;
    }
    .badge-card.locked .badge-progress  { background:var(--surface2); color:var(--muted); }
    .badge-card.earned .badge-progress { background:rgba(214,143,31,.2); color:var(--gold); }
    .earned-stamp { position:absolute; top:10px; right:10px; font-size:14px; opacity:0; transition:opacity .3s; }
    .badge-card.earned .earned-stamp { opacity:1; }
    .badge-toast {
      position:fixed; bottom:28px; left:50%;
      transform:translateX(-50%) translateY(80px);
      background:linear-gradient(135deg,#ffffff,#f5f7eb);
      border:1px solid rgba(214,143,31,.5); border-radius:18px; padding:14px 20px;
      display:flex; align-items:center; gap:13px;
      box-shadow:0 8px 32px rgba(0,0,0,.2);
      z-index:1000; min-width:260px; max-width:320px;
      transition:transform .4s cubic-bezier(.34,1.56,.64,1), opacity .4s;
      opacity:0; pointer-events:none;
    }
    .badge-toast.show { transform:translateX(-50%) translateY(0); opacity:1; pointer-events:auto; }
    .toast-icon {
      width:44px; height:44px; border-radius:14px; background:rgba(214,143,31,.15);
      display:flex; align-items:center; justify-content:center; font-size:22px; flex-shrink:0;
    }
    .toast-title { font-size:11px; font-weight:700; letter-spacing:1.5px; text-transform:uppercase; color:var(--gold); margin-bottom:2px; }
    .toast-name  { font-size:14px; font-weight:700; color:var(--text); }
    .toast-desc  { font-size:11px; color:var(--muted); margin-top:1px; }

    /* ══════════════════════════════════════
       SETTINGS PAGE
    ══════════════════════════════════════ */
    #settings-page { align-items: center; }

    .settings-header {
      width: 100%; max-width: 360px; margin-bottom: 24px;
    }
    .settings-title {
      font-size: 22px; font-weight: 800; color: var(--text);
      display: flex; align-items: center; gap: 10px;
    }

    /* Profile card */
    .profile-card {
      width: 100%; max-width: 360px;
      background: var(--surface); border: 1px solid var(--border);
      border-radius: 24px; padding: 24px;
      margin-bottom: 16px;
    }
    .profile-card-label {
      font-size: 10px; font-weight: 700; letter-spacing: 2px;
      text-transform: uppercase; color: var(--muted); margin-bottom: 16px;
    }
    .profile-row {
      display: flex; align-items: center; gap: 16px;
    }
    .profile-avatar {
      width: 56px; height: 56px; border-radius: 18px; flex-shrink: 0;
      display: flex; align-items: center; justify-content: center;
      font-size: 24px; font-weight: 900;
      background: linear-gradient(135deg, rgba(46,157,74,.2), rgba(139,111,199,.2));
      border: 1px solid rgba(46,157,74,.2);
      color: var(--green);
    }
    .profile-info {}
    .profile-name { font-size: 18px; font-weight: 800; color: var(--text); }
    .profile-role { font-size: 12px; color: var(--muted); margin-top: 3px; }

    /* Info rows */
    .settings-card {
      width: 100%; max-width: 360px;
      background: var(--surface); border: 1px solid var(--border);
      border-radius: 24px; overflow: hidden; margin-bottom: 16px;
    }
    .settings-row {
      display: flex; justify-content: space-between; align-items: center;
      padding: 16px 20px; border-bottom: 1px solid var(--border);
      font-size: 14px;
    }
    .settings-row:last-child { border-bottom: none; }
    .s-key { color: var(--muted); font-weight: 500; display:flex; align-items:center; gap:8px; }
    .s-val { font-weight: 700; color: var(--text); }
    .s-val.green  { color: var(--green); }
    .s-val.purple { color: var(--purple); }
    .s-val.blue   { color: var(--green); }

    /* Logout button */
    .btn-logout {
      width: 100%; max-width: 360px; padding: 17px;
      background: var(--surface); border: 1px solid rgba(196,69,45,.4);
      border-radius: 16px; font-size: 15px; font-weight: 700; color: var(--red);
      cursor: pointer; display: flex; align-items: center; justify-content: center; gap: 9px;
      transition: background .2s, transform .15s;
      margin-top: 4px;
    }
    .btn-logout:active  { transform: scale(.97); }
    .btn-logout:hover   { background: rgba(196,69,45,.08); }
  </style>
</head>
<body>

<!-- ==================== LOGIN PAGE ==================== -->
<div class="page active" id="login-page">

  <div class="login-brand">
    <div class="login-logo-wrap">&#128717;</div>
    <div class="login-app-name">Friendsies Bag</div>
    <div class="login-tagline">Smart Weighing System</div>
  </div>

  <div class="login-card">
    <div class="login-title">Welcome back 👋</div>
    <div class="login-sub">Sign in to continue to your dashboard.</div>

    <label class="field-label" for="inp-user">Username</label>
    <input class="field-input" id="inp-user" type="text"
           placeholder="Enter your username" autocomplete="off" autocapitalize="none">

    <label class="field-label" for="inp-pass">Password</label>
    <input class="field-input" id="inp-pass" type="password"
           placeholder="Enter your password">

    <div class="login-error" id="login-error">
      ✕ &nbsp;Incorrect username or password. Please try again.
    </div>

    <button class="btn-login" onclick="doLogin()">Sign In</button>
  </div>

  <div class="login-version">Friendsies Bag &nbsp;·&nbsp; v1.0.0</div>

</div><!-- /login-page -->


<!-- ==================== MAIN PAGE ==================== -->
<div class="page" id="main-page">

  <!-- Top bar: branding left, settings right -->
  <div class="top-bar">
    <div class="header">
      <div class="logo">Smart Weighing System</div>
      <div class="title">Friendsies Bag</div>
      <div class="status-pill" id="status-pill">
        <span class="status-dot"></span>
        <span id="status-text">Ready to measure</span>
      </div>
    </div>
    <button class="btn-settings" onclick="showSettings()">
      <svg width="14" height="14" viewBox="0 0 24 24" fill="none"
           stroke="currentColor" stroke-width="2.5"
           stroke-linecap="round" stroke-linejoin="round">
        <circle cx="12" cy="12" r="3"/>
        <path d="M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 0 1-2.83 2.83l-.06-.06
                 a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 0 1-4 0v-.09
                 A1.65 1.65 0 0 0 9 19.4a1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 0 1-2.83-2.83
                 l.06-.06A1.65 1.65 0 0 0 4.68 15a1.65 1.65 0 0 0-1.51-1H3a2 2 0 0 1 0-4h.09
                 A1.65 1.65 0 0 0 4.6 9a1.65 1.65 0 0 0-.33-1.82l-.06-.06a2 2 0 0 1 2.83-2.83
                 l.06.06A1.65 1.65 0 0 0 9 4.68a1.65 1.65 0 0 0 1-1.51V3a2 2 0 0 1 4 0v.09
                 a1.65 1.65 0 0 0 1 1.51 1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 0 1 2.83 2.83
                 l-.06.06A1.65 1.65 0 0 0 19.4 9a1.65 1.65 0 0 0 1.51 1H21a2 2 0 0 1 0 4h-.09
                 a1.65 1.65 0 0 0-1.51 1z"/>
      </svg>
      Settings
    </button>
  </div>

  <div class="gauge-card" id="main-card">
    <div class="gauge-wrap">
      <svg viewBox="0 0 220 220" xmlns="http://www.w3.org/2000/svg">
        <path d="M 46.36 173.64 A 90 90 0 1 1 173.64 173.64"
              fill="none" stroke="#bcc4ad" stroke-width="14" stroke-linecap="round"/>
        <path id="gauge-fill"
              d="M 46.36 173.64 A 90 90 0 1 1 173.64 173.64"
              stroke-dasharray="424.12 424.12"
              stroke-dashoffset="424.12"/>
      </svg>
      <div class="gauge-centre">
        <div class="gauge-val" id="gauge-val">0.0</div>
        <div class="gauge-unit" id="gauge-unit">kg</div>
      </div>
    </div>
    <div class="gauge-minmax"><span>0 kg</span><span>7 kg</span></div>
    <div class="alert-strip" id="alert-strip">
      &#9888; Overload! Remove weight before stopping.
    </div>
  </div>

  <div class="controls">
    <button class="btn btn-start" id="btn-start" onclick="sendCmd('start')">
      <svg width="16" height="16" viewBox="0 0 24 24" fill="none"
           stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round">
        <polygon points="5 3 19 12 5 21 5 3"/>
      </svg>
      Start
    </button>
    <button class="btn btn-stop" id="btn-stop" onclick="sendCmd('stop')">
      <svg width="16" height="16" viewBox="0 0 24 24" fill="none"
           stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round">
        <rect x="3" y="3" width="18" height="18" rx="2" ry="2"/>
      </svg>
      Stop
    </button>
  </div>

  <div class="secondary-btns">
    <button class="btn-secondary btn-trips" onclick="showTrips()">
      <svg width="15" height="15" viewBox="0 0 24 24" fill="none"
           stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round">
        <path d="M3 9l9-7 9 7v11a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2z"/>
        <polyline points="9 22 9 12 15 12 15 22"/>
      </svg>
      Trips
      <span class="sec-badge trips-badge" id="trips-badge">0</span>
    </button>
    <button class="btn-secondary btn-trophy" onclick="showTrophyRoom()">
      <svg width="15" height="15" viewBox="0 0 24 24" fill="none"
           stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round">
        <path d="M6 9H4.5a2.5 2.5 0 0 1 0-5H6"/>
        <path d="M18 9h1.5a2.5 2.5 0 0 0 0-5H18"/>
        <path d="M4 22h16"/>
        <path d="M10 14.66V17c0 .55-.47.98-.97 1.21C7.85 18.75 7 20.24 7 22"/>
        <path d="M14 14.66V17c0 .55.47.98.97 1.21C16.15 18.75 17 20.24 17 22"/>
        <path d="M18 2H6v7a6 6 0 0 0 12 0V2z"/>
      </svg>
      Trophy Room
      <span class="sec-badge trophy-badge" id="trophy-badge">0</span>
    </button>
  </div>

  <div class="final-label" id="final-label"></div>

  <div class="impact-section">
    <div class="section-label">Environmental &amp; Financial Impact</div>
    <div class="impact-grid">
      <div class="impact-btn green" onclick="showDetail('bags')">
        <span class="ib-icon">&#128717;</span>
        <span class="ib-label">Plastic Bags Diverted</span>
        <span class="ib-arrow">&#8599;</span>
      </div>
      <div class="impact-btn blue" onclick="showDetail('co2')">
        <span class="ib-icon">&#127807;</span>
        <span class="ib-label">CO&#8322; Emissions Prevented</span>
        <span class="ib-arrow">&#8599;</span>
      </div>
      <div class="impact-btn amber" onclick="showDetail('money')">
        <span class="ib-icon">&#128176;</span>
        <span class="ib-label">Money Saved (SAR)</span>
        <span class="ib-arrow">&#8599;</span>
      </div>
    </div>
  </div>

</div><!-- /main-page -->


<!-- ==================== DETAIL PAGE ==================== -->
<div class="page detail-page" id="detail-page">
  <div style="width:100%;max-width:360px;">
    <button class="back-btn" onclick="goBack()">
      <svg width="14" height="14" viewBox="0 0 24 24" fill="none"
           stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round">
        <polyline points="15 18 9 12 15 6"/>
      </svg>
      Back
    </button>
    <div class="detail-card" id="detail-content"></div>
  </div>
</div>


<!-- ==================== TRIPS PAGE ==================== -->
<div class="page" id="trips-page">
  <div class="trips-header">
    <div class="trips-title-block">
      <div class="trips-page-title">&#128717; My Trips</div>
      <div class="trips-page-sub" id="trips-sub">0 trips recorded</div>
    </div>
    <button class="btn-clear-all" onclick="clearAllTrips()">Clear All</button>
  </div>
  <div class="trips-summary">
    <div class="sum-card"><div class="sum-val purple" id="sum-trips">0</div><div class="sum-lbl">Trips</div></div>
    <div class="sum-card"><div class="sum-val green"  id="sum-weight">0.0</div><div class="sum-lbl">Total kg</div></div>
    <div class="sum-card"><div class="sum-val amber"  id="sum-sar">0.00</div><div class="sum-lbl">SAR Saved</div></div>
  </div>
  <div class="trips-list" id="trips-list"></div>
  <div style="margin-top:20px;width:100%;max-width:360px;">
    <button class="back-btn" onclick="goBack()">
      <svg width="14" height="14" viewBox="0 0 24 24" fill="none"
           stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round">
        <polyline points="15 18 9 12 15 6"/>
      </svg>
      Back
    </button>
  </div>
</div>


<!-- ==================== TROPHY ROOM PAGE ==================== -->
<div class="page" id="trophy-page">
  <div class="trophy-header" style="width:100%;max-width:360px;">
    <button class="back-btn" onclick="goBack()" style="margin-bottom:16px;">
      <svg width="14" height="14" viewBox="0 0 24 24" fill="none"
           stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round">
        <polyline points="15 18 9 12 15 6"/>
      </svg>
      Back
    </button>
    <div class="trophy-page-title">&#127942; Trophy Room</div>
    <div class="trophy-page-sub">Complete eco-friendly trips to unlock badges and build a lasting habit.</div>
  </div>
  <div class="trophy-progress">
    <div class="tp-row">
      <span class="tp-label">Badges Earned</span>
      <span class="tp-val" id="tp-count">0 / 4</span>
    </div>
    <div class="tp-bar-bg"><div class="tp-bar-fill" id="tp-bar" style="width:0%"></div></div>
  </div>
  <div class="badge-grid" id="badge-grid"></div>
</div>


<!-- ==================== SETTINGS PAGE ==================== -->
<div class="page" id="settings-page">

  <div class="settings-header">
    <button class="back-btn" onclick="goBack()" style="margin-bottom:16px;">
      <svg width="14" height="14" viewBox="0 0 24 24" fill="none"
           stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round">
        <polyline points="15 18 9 12 15 6"/>
      </svg>
      Back
    </button>
    <div class="settings-title">
      <svg width="20" height="20" viewBox="0 0 24 24" fill="none"
           stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round">
        <circle cx="12" cy="12" r="3"/>
        <path d="M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 0 1-2.83 2.83l-.06-.06
                 a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 0 1-4 0v-.09
                 A1.65 1.65 0 0 0 9 19.4a1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 0 1-2.83-2.83
                 l.06-.06A1.65 1.65 0 0 0 4.68 15a1.65 1.65 0 0 0-1.51-1H3a2 2 0 0 1 0-4h.09
                 A1.65 1.65 0 0 0 4.6 9a1.65 1.65 0 0 0-.33-1.82l-.06-.06a2 2 0 0 1 2.83-2.83
                 l.06.06A1.65 1.65 0 0 0 9 4.68a1.65 1.65 0 0 0 1-1.51V3a2 2 0 0 1 4 0v.09
                 a1.65 1.65 0 0 0 1 1.51 1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 0 1 2.83 2.83
                 l-.06.06A1.65 1.65 0 0 0 19.4 9a1.65 1.65 0 0 0 1.51 1H21a2 2 0 0 1 0 4h-.09
                 a1.65 1.65 0 0 0-1.51 1z"/>
      </svg>
      Settings
    </div>
  </div>

  <!-- Profile -->
  <div class="profile-card">
    <div class="profile-card-label">Logged In As</div>
    <div class="profile-row">
      <div class="profile-avatar" id="settings-avatar">?</div>
      <div class="profile-info">
        <div class="profile-name" id="settings-username">—</div>
        <div class="profile-role" id="settings-role">User</div>
      </div>
    </div>
  </div>

  <!-- App info -->
  <div class="settings-card">
    <div class="settings-row">
      <span class="s-key">
        <svg width="14" height="14" viewBox="0 0 24 24" fill="none"
             stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round">
          <path d="M21 16V8a2 2 0 0 0-1-1.73l-7-4a2 2 0 0 0-2 0l-7 4A2 2 0 0 0 3 8v8a2 2 0 0 0 1 1.73l7 4a2 2 0 0 0 2 0l7-4A2 2 0 0 0 21 16z"/>
        </svg>
        App Name
      </span>
      <span class="s-val blue">Friendsies Bag</span>
    </div>
    <div class="settings-row">
      <span class="s-key">
        <svg width="14" height="14" viewBox="0 0 24 24" fill="none"
             stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round">
          <polyline points="16 18 22 12 16 6"/>
          <polyline points="8 6 2 12 8 18"/>
        </svg>
        Version
      </span>
      <span class="s-val green">v 1.0.0</span>
    </div>
    <div class="settings-row">
      <span class="s-key">
        <svg width="14" height="14" viewBox="0 0 24 24" fill="none"
             stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round">
          <rect x="2" y="3" width="20" height="14" rx="2" ry="2"/>
          <line x1="8" y1="21" x2="16" y2="21"/>
          <line x1="12" y1="17" x2="12" y2="21"/>
        </svg>
        Device
      </span>
      <span class="s-val">ESP32</span>
    </div>
    <div class="settings-row">
      <span class="s-key">
        <svg width="14" height="14" viewBox="0 0 24 24" fill="none"
             stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round">
          <path d="M22 11.08V12a10 10 0 1 1-5.93-9.14"/>
          <polyline points="22 4 12 14.01 9 11.01"/>
        </svg>
        Max Weight
      </span>
      <span class="s-val">7 kg</span>
    </div>
    <div class="settings-row" style="border-bottom:none;">
      <span class="s-key">
        <svg width="14" height="14" viewBox="0 0 24 24" fill="none"
             stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round">
          <circle cx="12" cy="12" r="10"/>
          <polyline points="12 6 12 12 16 14"/>
        </svg>
        Trips Saved
      </span>
      <span class="s-val purple" id="settings-trips-count">0</span>
    </div>
  </div>

  <!-- Logout -->
  <button class="btn-logout" onclick="doLogout()">
    <svg width="16" height="16" viewBox="0 0 24 24" fill="none"
         stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round">
      <path d="M9 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h4"/>
      <polyline points="16 17 21 12 16 7"/>
      <line x1="21" y1="12" x2="9" y2="12"/>
    </svg>
    Log Out
  </button>

</div><!-- /settings-page -->


<!-- ── Badge unlock toast ── -->
<div class="badge-toast" id="badge-toast">
  <div class="toast-icon" id="toast-icon">🏅</div>
  <div class="toast-body">
    <div class="toast-title">🎉 Badge Unlocked!</div>
    <div class="toast-name" id="toast-name">Badge Name</div>
    <div class="toast-desc" id="toast-desc">You earned a new badge!</div>
  </div>
</div>


<!-- ==================== JAVASCRIPT ==================== -->
<script>
  /* ══════════════════════════════════════
     USERS
  ══════════════════════════════════════ */
  var USERS = [
    { username: 'Lama',  password: 'lama123',  role: 'Primary User'   },
    { username: 'Taif',  password: 'taif123',  role: 'Secondary User' }
  ];

  /* ── Session ── */
  var currentUser = null;

  /* ── Constants ── */
  var BAG_CAPACITY  = 5.0;
  var CO2_FOOTPRINT = 1.58;
  var BAG_FEE       = 0.50;
  var MAX_G         = 7000;
  var LOCK_G        = 7000;
  var ARC_LEN       = 424.12;

  /* ── Runtime state (not persisted) ── */
  var isRunning     = false;
  var currentWeight = 0.0;
  var tripWeight    = 0.0;

  /* ── In-memory (loaded from localStorage on login) ── */
  var trips          = [];
  var earnedBadgeIds = {};

  /* ── DOM refs ── */
  var gaugeFill  = document.getElementById('gauge-fill');
  var gaugeVal   = document.getElementById('gauge-val');
  var gaugeUnit  = document.getElementById('gauge-unit');
  var mainCard   = document.getElementById('main-card');
  var statusPill = document.getElementById('status-pill');
  var statusTxt  = document.getElementById('status-text');
  var alertStrip = document.getElementById('alert-strip');
  var finalLabel = document.getElementById('final-label');
  var btnStart   = document.getElementById('btn-start');
  var btnStop    = document.getElementById('btn-stop');

  /* ══════════════════════════════════════
     localStorage HELPERS (per-user keys)
  ══════════════════════════════════════ */
  function storageKey(suffix) {
    return 'fb_' + currentUser.username.toLowerCase() + '_' + suffix;
  }

  function saveToStorage() {
    try {
      localStorage.setItem(storageKey('trips'),   JSON.stringify(trips));
      localStorage.setItem(storageKey('badges'),  JSON.stringify(earnedBadgeIds));
    } catch(e) {}
  }

  function loadFromStorage() {
    try {
      var t = localStorage.getItem(storageKey('trips'));
      var b = localStorage.getItem(storageKey('badges'));
      trips          = t ? JSON.parse(t) : [];
      earnedBadgeIds = b ? JSON.parse(b) : {};
    } catch(e) {
      trips          = [];
      earnedBadgeIds = {};
    }
  }

  /* ══════════════════════════════════════
     LOGIN / LOGOUT
  ══════════════════════════════════════ */
  function doLogin() {
    var u = document.getElementById('inp-user').value.trim();
    var p = document.getElementById('inp-pass').value;
    var err = document.getElementById('login-error');

    var found = null;
    for (var i = 0; i < USERS.length; i++) {
      if (USERS[i].username.toLowerCase() === u.toLowerCase()
          && USERS[i].password === p) {
        found = USERS[i]; break;
      }
    }

    if (!found) {
      err.classList.add('show');
      document.getElementById('inp-pass').value = '';
      return;
    }

    err.classList.remove('show');
    currentUser = found;

    /* load this user's persisted data */
    loadFromStorage();

    updateTripsBadge();
    updateTrophyBadge();
    navigateTo('main-page');
  }

  /* Allow Enter key */
  document.getElementById('inp-pass').addEventListener('keydown', function(e){
    if (e.key === 'Enter') doLogin();
  });
  document.getElementById('inp-user').addEventListener('keydown', function(e){
    if (e.key === 'Enter') document.getElementById('inp-pass').focus();
  });

  function doLogout() {
    /* save before leaving */
    if (currentUser) saveToStorage();

    currentUser   = null;
    trips         = [];
    earnedBadgeIds = {};
    isRunning     = false;
    currentWeight = 0;
    tripWeight    = 0;
    setGauge(0);
    updateTripsBadge();
    updateTrophyBadge();
    document.getElementById('inp-user').value = '';
    document.getElementById('inp-pass').value = '';
    document.getElementById('final-label').textContent = '';
    navigateTo('login-page');
  }

  /* ── Page navigation ── */
  function navigateTo(id) {
    document.querySelectorAll('.page').forEach(function(p){ p.classList.remove('active'); });
    document.getElementById(id).classList.add('active');
  }
  function goBack() { navigateTo('main-page'); }

  /* ══════════════════════════════════════
     SETTINGS
  ══════════════════════════════════════ */
  function showSettings() {
    if (!currentUser) return;
    document.getElementById('settings-username').textContent    = currentUser.username;
    document.getElementById('settings-role').textContent        = currentUser.role;
    document.getElementById('settings-avatar').textContent      = currentUser.username.charAt(0).toUpperCase();
    document.getElementById('settings-trips-count').textContent = trips.length;
    navigateTo('settings-page');
  }

  /* ── Gauge renderer ── */
  function setGauge(grams) {
    var fraction = Math.min(Math.max(grams / MAX_G, 0), 1);
    gaugeFill.style.strokeDashoffset = ARC_LEN * (1 - fraction);
    if      (fraction < 0.60) gaugeFill.style.stroke = '#2e9d4a';
    else if (fraction < 0.85) gaugeFill.style.stroke = '#d68f1f';
    else                      gaugeFill.style.stroke = '#c4452d';
    if (grams >= 1000) {
      gaugeVal.textContent  = (grams / 1000).toFixed(2);
      gaugeUnit.textContent = 'kg';
    } else {
      gaugeVal.textContent  = grams.toFixed(0);
      gaugeUnit.textContent = 'g';
    }
  }

  /* ── Send Start / Stop ── */
  function sendCmd(cmd) {
    if (cmd === 'stop' && currentWeight > LOCK_G) return;
    fetch('/command?action=' + cmd)
      .then(function(r){ return r.text(); })
      .then(function(state){
        isRunning = (state === 'running');
        updateUI();
      })
      .catch(function(){
        isRunning = !isRunning;
        updateUI();
      });
  }

  /* ── UI state sync ── */
  function updateUI() {
    if (isRunning) {
      statusPill.className   = 'status-pill running';
      statusTxt.textContent  = 'Measuring in progress...';
      mainCard.className     = 'gauge-card measuring';
      gaugeVal.className     = 'gauge-val running';
      finalLabel.textContent = '';
      tripWeight             = 0.0;
      alertStrip.classList.remove('show');
      btnStop.disabled       = false;
    } else {
      statusPill.className   = 'status-pill';
      statusTxt.textContent  = 'Measurement stopped';
      mainCard.className     = 'gauge-card';
      gaugeVal.className     = 'gauge-val';
      tripWeight             = currentWeight;
      alertStrip.classList.remove('show');
      btnStop.disabled       = false;
      setGauge(0);
      if (tripWeight > 0) {
        finalLabel.textContent = 'Final weight: ' + (tripWeight / 1000).toFixed(3) + ' kg';
        saveTrip(tripWeight);
      } else {
        finalLabel.textContent = '';
      }
    }
  }

  /* ── Polling loop (500 ms) ── */
  setInterval(function(){
    if (!isRunning) return;
    fetch('/weight')
      .then(function(r){ return r.text(); })
      .then(function(val){
        var v = parseFloat(val);
        if (isNaN(v) || v < 0) v = 0;
        currentWeight = v;
        setGauge(v);
        if (v > LOCK_G) {
          gaugeVal.className    = 'gauge-val overlimit';
          mainCard.className    = 'gauge-card overlimit';
          statusPill.className  = 'status-pill overlimit';
          statusTxt.textContent = 'Overload — remove weight!';
          alertStrip.classList.add('show');
          btnStop.disabled      = true;
        } else {
          gaugeVal.className    = 'gauge-val running';
          mainCard.className    = 'gauge-card measuring';
          statusPill.className  = 'status-pill running';
          statusTxt.textContent = 'Measuring in progress...';
          alertStrip.classList.remove('show');
          btnStop.disabled      = false;
        }
      })
      .catch(function(){});
  }, 500);

  /* ══════════════════════════════════════
     TRIPS LOGIC
  ══════════════════════════════════════ */
  function saveTrip(grams) {
    var now  = new Date();
    var date = now.toLocaleDateString('en-GB', { day:'2-digit', month:'short', year:'numeric' });
    var time = now.toLocaleTimeString('en-GB', { hour:'2-digit', minute:'2-digit' });
    var wKg  = grams / 1000;
    var bags = Math.ceil(wKg / BAG_CAPACITY);
    var co2  = (bags * CO2_FOOTPRINT).toFixed(2);
    var sar  = (bags * BAG_FEE).toFixed(2);
    trips.unshift({ id: Date.now(), date: date + ' · ' + time,
                    grams: grams, bags: bags, co2: co2, sar: sar });
    saveToStorage();
    updateTripsBadge();
    checkBadges();
  }

  function updateTripsBadge() {
    document.getElementById('trips-badge').textContent = trips.length;
  }

  function deleteTrip(id) {
    trips = trips.filter(function(t){ return t.id !== id; });
    saveToStorage();
    updateTripsBadge(); renderTrips();
  }

  function clearAllTrips() {
    if (trips.length === 0) return;
    if (!confirm('Delete all ' + trips.length + ' trips?')) return;
    trips = []; earnedBadgeIds = {};
    saveToStorage();
    updateTripsBadge(); renderTrips(); updateTrophyBadge();
  }

  function renderTrips() {
    var list = document.getElementById('trips-list');
    var totalKg = 0, totalSar = 0;
    for (var i = 0; i < trips.length; i++) {
      totalKg  += trips[i].grams / 1000;
      totalSar += parseFloat(trips[i].sar);
    }
    document.getElementById('sum-trips').textContent  = trips.length;
    document.getElementById('sum-weight').textContent = totalKg.toFixed(2);
    document.getElementById('sum-sar').textContent    = totalSar.toFixed(2);
    document.getElementById('trips-sub').textContent  =
      trips.length + ' trip' + (trips.length !== 1 ? 's' : '') + ' recorded';
    if (trips.length === 0) {
      list.innerHTML = '<div class="trips-empty"><div class="empty-icon">&#128717;</div>'
        + 'No trips yet.<br>Complete a measurement to save your first trip.</div>';
      return;
    }
    var html = '';
    for (var j = 0; j < trips.length; j++) {
      var tr  = trips[j];
      var wKg = (tr.grams / 1000).toFixed(3);
      html += '<div class="trip-card">'
        + '<div class="trip-top"><div>'
        + '<div class="trip-num">Trip #' + (trips.length - j) + '</div>'
        + '<div class="trip-date">' + tr.date + '</div></div>'
        + '<button class="trip-delete" onclick="deleteTrip(' + tr.id + ')">Delete</button>'
        + '</div><div class="trip-weight">' + wKg + ' <span>kg</span></div>'
        + '<div class="trip-stats">'
        + '<div class="trip-stat g">&#128717; ' + tr.bags + ' bags</div>'
        + '<div class="trip-stat b">&#127807; ' + tr.co2 + ' kg CO\u2082</div>'
        + '<div class="trip-stat a">&#128176; ' + tr.sar + ' SAR</div>'
        + '</div></div>';
    }
    list.innerHTML = html;
  }

  function showTrips() { renderTrips(); navigateTo('trips-page'); }

  /* ══════════════════════════════════════
     BADGE / TROPHY LOGIC
  ══════════════════════════════════════ */
  var BADGES = [
    { id:'first_trip',    icon:'\uD83C\uDF31', name:'First Trip',    desc:'Complete your very first weighing trip.',
      check:function(s){ return s.totalTrips>=1; },
      progress:function(s){ return Math.min(s.totalTrips,1)+' / 1 trip'; } },
    { id:'heavy_lifter',  icon:'\uD83D\uDCAA', name:'Heavy Lifter',  desc:'Complete 2 trips each weighing 6 kg or more.',
      check:function(s){ return s.heavyTrips>=2; },
      progress:function(s){ return Math.min(s.heavyTrips,2)+' / 2 heavy trips'; } },
    { id:'plastic_saver', icon:'\u267B\uFE0F', name:'Plastic Saver', desc:'Save a total of 5 plastic bags across all trips.',
      check:function(s){ return s.totalBags>=5; },
      progress:function(s){ return Math.min(s.totalBags,5)+' / 5 bags'; } },
    { id:'earth_hero',    icon:'\uD83C\uDF0D', name:'Earth Hero',    desc:'Save a total of 10 plastic bags across all trips.',
      check:function(s){ return s.totalBags>=10; },
      progress:function(s){ return Math.min(s.totalBags,10)+' / 10 bags'; } }
  ];

  function computeStats() {
    var t = trips.length, h = 0, b = 0;
    for (var i = 0; i < trips.length; i++) {
      if (trips[i].grams >= 6000) h++;
      b += trips[i].bags;
    }
    return { totalTrips: t, heavyTrips: h, totalBags: b };
  }

  function checkBadges() {
    var stats = computeStats(), newly = [];
    for (var i = 0; i < BADGES.length; i++) {
      if (!earnedBadgeIds[BADGES[i].id] && BADGES[i].check(stats)) {
        earnedBadgeIds[BADGES[i].id] = true;
        newly.push(BADGES[i]);
      }
    }
    saveToStorage();
    updateTrophyBadge();
    for (var j = 0; j < newly.length; j++) {
      (function(b,d){ setTimeout(function(){ showBadgeToast(b); }, d); })(newly[j], j*3200);
    }
  }

  function countEarned() {
    var s = computeStats(), c = 0;
    for (var i = 0; i < BADGES.length; i++) if (BADGES[i].check(s)) c++;
    return c;
  }

  function updateTrophyBadge() {
    document.getElementById('trophy-badge').textContent = countEarned();
  }

  var toastTimer = null;
  function showBadgeToast(badge) {
    document.getElementById('toast-icon').textContent = badge.icon;
    document.getElementById('toast-name').textContent = badge.name;
    document.getElementById('toast-desc').textContent = badge.desc;
    var toast = document.getElementById('badge-toast');
    toast.classList.add('show');
    if (toastTimer) clearTimeout(toastTimer);
    toastTimer = setTimeout(function(){ toast.classList.remove('show'); }, 3000);
  }

  function showTrophyRoom() {
    var stats  = computeStats();
    var earned = countEarned();
    var total  = BADGES.length;
    document.getElementById('tp-count').textContent = earned + ' / ' + total;
    document.getElementById('tp-bar').style.width   = Math.round(earned/total*100) + '%';
    var grid = document.getElementById('badge-grid'), html = '';
    for (var i = 0; i < BADGES.length; i++) {
      var b = BADGES[i], ok = b.check(stats);
      html += '<div class="badge-card '+(ok?'earned':'locked')+'">'
        + '<span class="earned-stamp">\u2728</span>'
        + '<div class="badge-icon-wrap"><span>'+b.icon+'</span></div>'
        + '<div class="badge-name">'+b.name+'</div>'
        + '<div class="badge-desc">'+b.desc+'</div>'
        + '<div class="badge-progress">'+b.progress(stats)+'</div>'
        + '</div>';
    }
    grid.innerHTML = html;
    navigateTo('trophy-page');
  }

  /* ── Detail screens ── */
  function showDetail(type) {
    var wG = (tripWeight > 0) ? tripWeight : currentWeight;
    var weight = wG / 1000;
    var bags   = Math.ceil(weight / BAG_CAPACITY);
    var co2    = (bags * CO2_FOOTPRINT).toFixed(2);
    var money  = (bags * BAG_FEE).toFixed(2);
    var ok     = weight > 0;
    var html   = '';
    if (type === 'bags') {
      html = '<div class="d-icon-wrap green">&#128717;</div>'
           + '<div class="d-title">Plastic Bags Diverted</div>'
           + '<div class="d-subtitle">Single-use plastic bags avoided based on your trip weight.</div>';
      if (ok) {
        html += '<div class="result-big green"><div class="result-value green">'+bags+'</div>'
              + '<div class="result-unit">bags avoided</div></div>'
              + buildFormula([['Trip weight',weight.toFixed(3)+' kg'],['Bag capacity',BAG_CAPACITY+' kg / bag'],
                              ['Formula','ceil( weight &divide; capacity )'],['Bags diverted',bags+' bags']]);
      } else { html += noData(); }
    } else if (type === 'co2') {
      html = '<div class="d-icon-wrap blue">&#127807;</div>'
           + '<div class="d-title">CO&#8322; Emissions Prevented</div>'
           + '<div class="d-subtitle">Carbon footprint avoided by reducing single-use plastic bag consumption.</div>';
      if (ok) {
        html += '<div class="result-big blue"><div class="result-value blue">'+co2
              + '<span style="font-size:26px;letter-spacing:0"> kg</span></div>'
              + '<div class="result-unit">CO&#8322; prevented</div></div>'
              + buildFormula([['Bags diverted',bags+' bags'],['CO&#8322; per bag',CO2_FOOTPRINT+' kg CO&#8322;'],
                              ['Formula','bags &times; CO&#8322; per bag'],['Total prevented',co2+' kg CO&#8322;']]);
      } else { html += noData(); }
    } else {
      html = '<div class="d-icon-wrap amber">&#128176;</div>'
           + '<div class="d-title">Money Saved</div>'
           + '<div class="d-subtitle">Financial savings from avoiding plastic bag fees (Saudi Riyal).</div>';
      if (ok) {
        html += '<div class="result-big amber"><div class="result-value amber">'+money+'</div>'
              + '<div class="result-unit">SAR saved</div></div>'
              + buildFormula([['Bags diverted',bags+' bags'],['Bag fee',BAG_FEE.toFixed(2)+' SAR / bag'],
                              ['Formula','bags &times; bag fee'],['Total saved',money+' SAR']]);
      } else { html += noData(); }
    }
    document.getElementById('detail-content').innerHTML = html;
    navigateTo('detail-page');
  }

  function buildFormula(rows) {
    var h = '<div class="formula-box"><div class="formula-title">Calculation Breakdown</div>';
    for (var i = 0; i < rows.length; i++)
      h += '<div class="f-row"><span class="f-key">'+rows[i][0]+'</span><span>'+rows[i][1]+'</span></div>';
    return h + '</div>';
  }
  function noData() {
    return '<div class="no-data">No measurement recorded yet.<br>Start a measurement first to see your impact.</div>';
  }
</script>

</body>
</html>
)rawliteral";


// ==========================================
// Web Server Handlers
// ==========================================

void handleRoot() {
  server.send(200, "text/html", htmlPage);
}

void handleWeight() {
  server.send(200, "text/plain", String(lastStableValue, 1));
}

void handleCommand() {
  String action = server.arg("action");
  if (action == "start") {
    isMeasuring = true;
    LoadCell.tareNoDelay();
  } else if (action == "stop") {
    if (lastStableValue <= 7000) {
      isMeasuring = false;
    }
  }
  server.send(200, "text/plain", isMeasuring ? "running" : "stopped");
}


// ==========================================
// Setup
// ==========================================
void setup() {
  Serial.begin(57600);

  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);

  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("http://"); Serial.println(IP);

  server.on("/",        handleRoot);
  server.on("/weight",  handleWeight);
  server.on("/command", handleCommand);
  server.begin();
  Serial.println("HTTP server started.");

  LoadCell.begin();
  LoadCell.setReverseOutput();
  LoadCell.start(10000, true);

  if (LoadCell.getTareTimeoutFlag()) {
    Serial.println("HX711 tare timeout — check wiring.");
    while (1);
  } else {
    LoadCell.setCalFactor(34.18);
    Serial.println("Load cell ready.");
  }

  while (!LoadCell.update());
  for (int i = 0; i < numReadings; i++) readings[i] = 0;
}


// ==========================================
// Loop
// ==========================================
void loop() {
  server.handleClient();

  static boolean newDataReady = false;
  if (LoadCell.update()) newDataReady = true;

  if (newDataReady && millis() > t + 100) {

    float raw = LoadCell.getData();

    total              -= readings[readIndex];
    readings[readIndex] = raw;
    total              += readings[readIndex];
    readIndex           = (readIndex + 1) % numReadings;
    stableValue         = total / numReadings;

    if (abs(stableValue - lastStableValue) >= threshold)
      lastStableValue = stableValue;

    if (lastStableValue < 0) lastStableValue = 0;

    digitalWrite(buzzerPin,
      (isMeasuring && lastStableValue > 7000) ? HIGH : LOW);

    newDataReady = false;
    t = millis();
  }
}
