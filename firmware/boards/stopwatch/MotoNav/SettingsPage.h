#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include "UiModern.h"

// 设置项全局状态
extern int qspiMHz;
extern int screenRot;
extern void setScreenDeg(int deg, bool save);

// 1. 亮度设置 (25%, 50%, 75%, 100%)
static const uint8_t brightnessLevels[] = {64, 128, 192, 255};
static const char* const brightnessLabels[] = {"25%", "50%", "75%", "100%"};
static int brightnessIdx = 2; // 默认 75% (192)

// 2. 地图主题样式
static const char* const mapStyleLabels[THEME_N] = {
  "CLASSIC", "VICE CITY", "GTA5", "CYBER",
  "PAPER", "MINIMAL", "BLUEPRINT", "PIXEL",
  "RADAR", "COMPASS", "SATELLITE", "DRAGON"
};
extern int themeIdx;

// 3. 屏幕旋转角度 (0°, 90°, 180°, 270°)
static const int rotationAngles[] = {0, 90, 180, 270};
static const char* const rotationLabels[] = {"0*", "90*", "180*", "270*"};
static int rotationIdx = 0;

// 4. 待机休眠时间 (3分钟, 5分钟, 10分钟, 常亮)
static const uint32_t sleepTimeouts[] = {180, 300, 600, 0};
static const char* const sleepLabels[] = {"3 MIN", "5 MIN", "10 MIN", "NEVER"};
static int sleepIdx = 1; // 默认 5 分钟

inline void settingsInit() {
  Preferences pr;
  if (pr.begin("motonav", true)) {
    int b = pr.getInt("brightness", 192);
    for (int i = 0; i < 4; i++) {
      if (abs((int)brightnessLevels[i] - b) < 32) { brightnessIdx = i; break; }
    }
    int deg = pr.getInt("rotd", 0);
    for (int i = 0; i < 4; i++) {
      if (rotationAngles[i] == (deg % 360)) { rotationIdx = i; break; }
    }
    int s = pr.getInt("autosleep", 300);
    for (int i = 0; i < 4; i++) {
      if (sleepTimeouts[i] == (uint32_t)s) { sleepIdx = i; break; }
    }
    pr.end();
  }
}

// 绘制设置页矢量图标
static void drawSettingsIcon(int id, int cx, int cy, uint16_t color) {
  switch (id) {
    case 0: // 太阳 (亮度)
      aaDisc(cx, cy, 5.0f, color);
      for (int a = 0; a < 360; a += 45) {
        float rad = a * (float)M_PI / 180.0f;
        gfx->drawLine((int)lroundf(cx + 8 * cosf(rad)), (int)lroundf(cy + 8 * sinf(rad)),
                      (int)lroundf(cx + 12 * cosf(rad)), (int)lroundf(cy + 12 * sinf(rad)), color);
      }
      break;

    case 1: // 地图/图层 (地图风格)
      gfx->drawRoundRect(cx - 10, cy - 8, 20, 16, 2, color);
      gfx->drawLine(cx - 10, cy - 1, cx + 10, cy - 1, color);
      gfx->drawLine(cx - 3, cy - 8, cx - 3, cy + 8, color);
      aaDisc(cx + 4, cy + 3, 2.0f, 0xFDE0);
      break;

    case 2: // 旋转箭头 (屏幕角度)
      gfx->drawCircle(cx, cy, 9, color);
      gfx->drawCircle(cx, cy, 10, color);
      // 右上方缺口与箭头
      gfx->fillRect(cx + 4, cy - 11, 8, 8, C_BLACK);
      gfx->fillTriangle(cx + 3, cy - 11, cx + 11, cy - 11, cx + 7, cy - 5, color);
      break;

    case 3: // 月牙/休眠 (待机时间)
      aaDisc(cx, cy, 8.5f, color);
      aaDisc(cx + 4.0f, cy - 3.0f, 7.5f, C_BLACK);
      break;
  }
}

// 绘制设置页面 (466x466 AMOLED 圆屏规范)
void drawSettingsPage() {
  onWaitScreen = false;
  frameIsMap = false;

  // 1. 绘制暗黑罪恶都市氛围背景
  drawViceCityBackground();

  // 2. 顶部毛玻璃胶囊标题
  const int titleW = 160;
  const int titleH = 32;
  const int titleX = CX - titleW / 2;
  const int titleY = 36;
  drawTranslucentRoundRect(titleX, titleY, titleW, titleH, 16, 0x0000, 192);
  gfx->drawRoundRect(titleX, titleY, titleW, titleH, 16, 0x39E7);
  drawAaString(CX, titleY + 16, "SETTINGS", 1.8f, 1.0f, 12.0f, C_WHITE, true);

  // 3. 4 个半透明毛玻璃设置卡片 (2x2 对称排布在屏幕黄金可视区)
  const int cardW = 164;
  const int cardH = 108;
  const int gapX = 14;
  const int gapY = 16;
  const int leftX  = CX - cardW - gapX / 2; // 233 - 164 - 7 = 62
  const int rightX = CX + gapX / 2;          // 240
  const int topY   = 94;
  const int botY   = topY + cardH + gapY;   // 218

  struct CardConfig {
    int x, y;
    int iconId;
    const char *title;
    const char *val;
    uint16_t accent;
  } cards[4] = {
    {leftX,  topY, 0, "BRIGHTNESS", brightnessLabels[brightnessIdx], 0xFDE0},
    {rightX, topY, 1, "MAP STYLE",  mapStyleLabels[themeIdx % THEME_N], 0x4ECB},
    {leftX,  botY, 2, "ROTATION",   rotationLabels[rotationIdx],     0x29ED},
    {rightX, botY, 3, "AUTO SLEEP", sleepLabels[sleepIdx],           0xAFBD}
  };

  for (int i = 0; i < 4; i++) {
    int cx = cards[i].x;
    int cy = cards[i].y;

    // 半透明磨砂底座
    drawTranslucentRoundRect(cx, cy, cardW, cardH, 16, 0x0000, 175);
    gfx->drawRoundRect(cx, cy, cardW, cardH, 16, 0x31A6);

    // 图标 (居中偏上)
    drawSettingsIcon(cards[i].iconId, cx + cardW / 2, cy + 26, cards[i].accent);

    // 标题文字 (抗锯齿字符)
    drawAaString(cx + cardW / 2, cy + 54, cards[i].title, 1.3f, 0.8f, 9.0f, 0x7BEF, true);

    // 当前值高亮半透明胶囊
    const int valPillW = cardW - 32;
    const int valPillH = 26;
    const int valPillX = cx + 16;
    const int valPillY = cy + 72;
    drawTranslucentRoundRect(valPillX, valPillY, valPillW, valPillH, 13, cards[i].accent, 80);
    gfx->drawRoundRect(valPillX, valPillY, valPillW, valPillH, 13, cards[i].accent);
    drawAaString(cx + cardW / 2, valPillY + 13, cards[i].val, 1.6f, 0.95f, 10.5f, C_WHITE, true);
  }

  // 4. 底部微光操作提示
  drawAaString(CX, 386, "TAP CARD TO ADJUST", 1.2f, 0.75f, 8.5f, 0x4A69, true);

  // 5. 底部页面指示点
  drawPageDots();
  needFlush = true;
}

// 设置页面点击处理
void settingsPageTap(int x, int y) {
  if (page != 1 || callOn || pairOn || sliding) return;

  const int cardW = 164;
  const int cardH = 108;
  const int gapX = 14;
  const int gapY = 16;
  const int leftX  = CX - cardW - gapX / 2;
  const int rightX = CX + gapX / 2;
  const int topY   = 94;
  const int botY   = topY + cardH + gapY;

  int clickedCard = -1;
  if (x >= leftX && x <= leftX + cardW) {
    if (y >= topY && y <= topY + cardH) clickedCard = 0;       // 亮度
    else if (y >= botY && y <= botY + cardH) clickedCard = 2;  // 旋转
  } else if (x >= rightX && x <= rightX + cardW) {
    if (y >= topY && y <= topY + cardH) clickedCard = 1;       // 地图风格
    else if (y >= botY && y <= botY + cardH) clickedCard = 3;  // 休眠
  }

  if (clickedCard < 0) return;

  // 震动触感反馈
  CompanionMode::startVibration(50, 20);

  switch (clickedCard) {
    case 0: { // 亮度调节 (25% -> 50% -> 75% -> 100%)
      brightnessIdx = (brightnessIdx + 1) % 4;
      uint8_t b = brightnessLevels[brightnessIdx];
      panel->setBrightness(b);
      Preferences pr;
      if (pr.begin("motonav", false)) {
        pr.putInt("brightness", b);
        pr.end();
      }
      break;
    }
    case 1: { // 地图风格切换
      themeIdx = (themeIdx + 1) % THEME_N;
      Preferences pr;
      if (pr.begin("motonav", false)) {
        pr.putInt("maptheme", themeIdx);
        pr.end();
      }
      // 通知手机端同步地图风格
      if (bleConnected) {
        pendingEvt = "T,SET_THEME," + String(themeIdx);
      }
      break;
    }
    case 2: { // 屏幕旋转 (0° -> 90° -> 180° -> 270°)
      rotationIdx = (rotationIdx + 1) % 4;
      int deg = rotationAngles[rotationIdx];
      setScreenDeg(deg, true);
      break;
    }
    case 3: { // 待机休眠
      sleepIdx = (sleepIdx + 1) % 4;
      uint32_t s = sleepTimeouts[sleepIdx];
      Preferences pr;
      if (pr.begin("motonav", false)) {
        pr.putInt("autosleep", s);
        pr.end();
      }
      break;
    }
  }

  drawSettingsPage();
}

