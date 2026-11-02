#pragma once
#include <Arduino.h>

extern uint16_t *coverBuf;
extern bool coverValid;
extern uint32_t musicDuration;
extern uint32_t musicPos;
extern uint32_t musicPosAt;
extern bool musicLiked;
extern uint8_t musicRepeatMode;
extern int8_t musicVolume;
extern bool volSliding;
extern uint32_t volActiveUntil;

static inline void formatMusicTime(uint32_t sec, char *buf, size_t len) {
  uint32_t m = sec / 60;
  uint32_t s = sec % 60;
  snprintf(buf, len, "%02lu:%02lu", (unsigned long)m, (unsigned long)s);
}

// 绘制暗色半透明氛围背景：将 180x180 专辑封面拉伸至 466x466 全屏并暗化
static void drawCoverBackgroundDimmed() {
  if (!coverValid || coverBuf == nullptr) {
    gfx->fillScreen(C_BLACK);
    return;
  }
  uint16_t *fb = gfx->getFramebuffer();
  if (!fb) {
    gfx->fillScreen(C_BLACK);
    return;
  }
  const int W = LCD_WIDTH;  // 466
  const int H = LCD_HEIGHT; // 466
  for (int y = 0; y < H; y++) {
    int srcY = (y * 180) / H;
    const uint16_t *srcRow = &coverBuf[srcY * 180];
    uint16_t *dstRow = &fb[y * W];
    for (int x = 0; x < W; x++) {
      int srcX = (x * 180) / W;
      uint16_t c = srcRow[srcX];
      // 快速暗化到约 28% 亮度 (保留色相，与纯黑半透明混叠)
      uint32_t rb = (c & 0xF81F) * 70;
      uint32_t g  = (c & 0x07E0) * 70;
      dstRow[x] = (uint16_t)(((rb >> 8) & 0xF81F) | ((g >> 8) & 0x07E0));
    }
  }
}

// 绘制 Like 红心图标
static inline void drawHeartIcon(int cx, int cy, uint16_t color, bool filled) {
  if (filled) {
    gfx->fillCircle(cx - 5, cy - 3, 5, color);
    gfx->fillCircle(cx + 5, cy - 3, 5, color);
    gfx->fillTriangle(cx - 10, cy - 1, cx + 10, cy - 1, cx, cy + 9, color);
    gfx->fillRect(cx - 5, cy - 4, 11, 4, color);
  } else {
    gfx->drawCircle(cx - 5, cy - 3, 5, color);
    gfx->drawCircle(cx + 5, cy - 3, 5, color);
    gfx->drawLine(cx - 10, cy - 2, cx, cy + 9, color);
    gfx->drawLine(cx + 10, cy - 2, cx, cy + 9, color);
  }
}

// 绘制循环模式图标 (0: 列表循环, 1: 单曲循环, 2: 随机播放)
static inline void drawRepeatIcon(int cx, int cy, uint8_t mode) {
  uint16_t c = (mode == 1) ? 0xFFE0 : (mode == 2 ? 0x07E0 : C_WHITE);
  if (mode == 2) {
    // 随机播放：交叉双箭头
    gfx->drawLine(cx - 8, cy - 5, cx + 2, cy + 5, c);
    gfx->drawLine(cx + 2, cy + 5, cx + 8, cy + 5, c);
    gfx->drawLine(cx - 8, cy + 5, cx + 2, cy - 5, c);
    gfx->drawLine(cx + 2, cy - 5, cx + 8, cy - 5, c);
    gfx->drawLine(cx + 5, cy - 8, cx + 8, cy - 5, c);
    gfx->drawLine(cx + 5, cy - 2, cx + 8, cy - 5, c);
    gfx->drawLine(cx + 5, cy + 2, cx + 8, cy + 5, c);
    gfx->drawLine(cx + 5, cy + 8, cx + 8, cy + 5, c);
  } else {
    // 循环外框环
    gfx->drawRoundRect(cx - 8, cy - 6, 16, 12, 3, c);
    gfx->fillTriangle(cx + 6, cy - 9, cx + 10, cy - 6, cx + 6, cy - 3, c);
    if (mode == 1) {
      gfx->setTextColor(0xFFE0);
      gfx->setTextSize(1);
      gfx->setCursor(cx - 2, cy - 3);
      gfx->print("1");
    }
  }
}

// VOLUME 字符抗锯齿切线旋转排布 (5x7 矢量点阵)
static const uint8_t volumeCharBitmaps[6][5] = {
  {0x1F, 0x20, 0x40, 0x20, 0x1F}, // V
  {0x3E, 0x41, 0x41, 0x41, 0x3E}, // O
  {0x7F, 0x40, 0x40, 0x40, 0x40}, // L
  {0x3F, 0x40, 0x40, 0x40, 0x3F}, // U
  {0x7F, 0x02, 0x1C, 0x02, 0x7F}, // M
  {0x7F, 0x49, 0x49, 0x49, 0x41}  // E
};

static void drawVolumeTextRotated(uint16_t color) {
  const float R_text = 196.0f;
  const float p_v = 0.685f; // V 在左上方 (8点半)
  const float p_e = 0.615f; // E 在右下方 (7点)
  const float scale = 1.6f;

  for (int i = 0; i < 6; i++) {
    float p = p_v - i * ((p_v - p_e) / 5.0f);
    float a = p * 2.0f * (float)M_PI;
    float cx = CX + R_text * sinf(a);
    float cy = CX - R_text * cosf(a);
    const uint8_t *bmp = volumeCharBitmaps[i];

    float cosA = cosf(a), sinA = sinf(a);
    for (int col = 0; col < 5; col++) {
      uint8_t b = bmp[col];
      for (int row = 0; row < 7; row++) {
        if ((b >> row) & 1) {
          float lx = (col - 2.0f) * scale;
          float ly = (row - 3.0f) * scale;
          float px = cx - lx * cosA + ly * sinA;
          float py = cy - lx * sinA - ly * cosA;
          aaDisc(px, py, 1.15f, color);
        }
      }
    }
  }
}

// 绘制左下贴合圆屏弧度的无锯齿音量条 (默认仅显示VOLUME，按住VOLUME触发显示)
static void drawVolumeArc(int vol, bool showActive) {
  // 1. 始终以优雅的圆弧切线旋转呈现 "VOLUME" (浅灰半透明)
  drawVolumeTextRotated(showActive ? 0xFFFF : 0xCE79);

  if (!showActive) return;

  // 2. 激活状态：采用亚像素抗锯齿圆环 fillRingBand 绘制胶囊圆弧轨 (无锯齿)
  // 圆周比例范围：底端 7 点钟 (0.575) 到 顶端 8 点 40 分 (0.725)
  const float p_min = 0.575f;
  const float p_max = 0.725f;
  const float r_in = RING_R - 5;  // 217
  const float r_out = RING_R + 5; // 227
  const float half_th = 5.0f;

  // 绘制底轨槽 (深灰抗锯齿)
  fillRingBand(p_min, p_max, r_in, r_out, 0x2124);
  float a_min = p_min * 2.0f * (float)M_PI;
  aaDisc(CX + RING_R * sinf(a_min), CX - RING_R * cosf(a_min), half_th, 0x2124);
  float a_max = p_max * 2.0f * (float)M_PI;
  aaDisc(CX + RING_R * sinf(a_max), CX - RING_R * cosf(a_max), half_th, 0x2124);

  // 绘制音量填充弧：从底端 (0%) 顺时针向上生长到顶端 (100%)，杜绝上下颠倒
  float p_fill = p_min + (p_max - p_min) * (constrain(vol, 0, 100) / 100.0f);
  if (vol > 0) {
    fillRingBand(p_min, p_fill, r_in, r_out, 0x07FF); // 高亮青色
    aaDisc(CX + RING_R * sinf(a_min), CX - RING_R * cosf(a_min), half_th, 0x07FF);
    float a_fill = p_fill * 2.0f * (float)M_PI;
    aaDisc(CX + RING_R * sinf(a_fill), CX - RING_R * cosf(a_fill), half_th, 0x07FF);
    // 滑块高亮小白点
    aaDisc(CX + RING_R * sinf(a_fill), CX - RING_R * cosf(a_fill), 6.5f, C_WHITE);
  }

  // 弧线内侧显示实时百分比大数字
  char vbuf[16];
  snprintf(vbuf, sizeof(vbuf), "%d%%", vol);
  gfx->setTextColor(0x07FF);
  gfx->setTextSize(2);
  gfx->setCursor(32, CX + 35);
  gfx->print(vbuf);
}

// 音乐页：沉浸式半透明背景、大专辑封面、内嵌播放键、封面右下角Like、弧形贴边音量、滑动切歌
void drawMusic() {
  drawCoverBackgroundDimmed();
  onWaitScreen = false;
  frameIsMap = false;

  bool linked = bleConnected && bleAuthed;

  // 1. 贴合圆弧的音量条 (默认仅显示VOLUME，按住触发抗锯齿圆弧显示)
  bool showVol = volSliding || (millis() < volActiveUntil);
  drawVolumeArc(musicVolume, showVol);

  // 2. 专辑封面区 (位置下移至 Y: 56 ~ 236，大小 180x180，离线也保留显示)
  const int covSize = 180;
  const int covX = CX - covSize / 2; // 143
  const int covY = 56;

  if (coverValid && coverBuf != nullptr) {
    gfx->fillRoundRect(covX - 3, covY - 3, covSize + 6, covSize + 6, 14, 0x18C3);
    gfx->drawRoundRect(covX - 3, covY - 3, covSize + 6, covSize + 6, 14, 0x4A69);
    gfx->draw16bitRGBBitmap(covX, covY, coverBuf, covSize, covSize);
  } else {
    // 拟物黑胶唱片
    const int discR = 86;
    const int discY = covY + covSize / 2;
    aaDisc(CX, discY, discR, 0x10A2);
    gfx->drawCircle(CX, discY, 68, 0x2124);
    gfx->drawCircle(CX, discY, 48, 0x2124);
    aaDisc(CX, discY, 32, 0x249F);
    aaDisc(CX, discY, 8, C_BLACK);
    gfx->drawCircle(CX, discY, 8, 0xDEFB);
  }

  // 3. 嵌入专辑封面的播放/暂停按钮 (暂停时才显示大播放三角，播放时不显示)
  if (!musicPlaying) {
    int pcx = CX, pcy = covY + covSize / 2;
    aaDisc(pcx, pcy, 34, 0x18C3);
    gfx->fillTriangle(pcx - 10, pcy - 16, pcx - 10, pcy + 16, pcx + 16, pcy, C_WHITE);
  }

  // 4. 嵌入专辑封面右下角的 Like 按钮 (点击或双击封面触发)
  const int likeX = covX + covSize - 22; // 301
  const int likeY = covY + covSize - 22; // 214
  aaDisc(likeX, likeY, 18, musicLiked ? 0x4800 : 0x18C3);
  drawHeartIcon(likeX, likeY, musicLiked ? 0xF800 : 0xCE79, musicLiked);

  // 5. 歌曲进度条与大号时间数字 (位置下移至 Y: 252 ~ 284)
  if (musicHave && musicDuration > 0) {
    uint32_t curSec = musicPos;
    if (musicPlaying && linked) {
      uint32_t elapsed = (millis() - musicPosAt) / 1000;
      curSec = min(musicDuration, musicPos + elapsed);
    }
    const int barW = 250;
    const int barH = 8;
    const int barX = CX - barW / 2; // 108
    const int barY = 252;

    // 进度条背景
    gfx->fillRoundRect(barX, barY, barW, barH, 4, 0x2124);

    // 进度指示
    float progress = constrain((float)curSec / (float)musicDuration, 0.0f, 1.0f);
    int fillW = max(6, (int)(barW * progress));
    gfx->fillRoundRect(barX, barY, fillW, barH, 4, 0x07FF); // C_CYAN
    gfx->fillCircle(barX + fillW, barY + barH / 2, 6, C_WHITE);

    // 大号时间文字 (当前时间 / 总时长)
    char timeCur[16], timeTotal[16];
    formatMusicTime(curSec, timeCur, sizeof(timeCur));
    formatMusicTime(musicDuration, timeTotal, sizeof(timeTotal));
    gfx->setTextColor(0xCE79);
    gfx->setTextSize(2);
    gfx->setCursor(barX, barY + barH + 6);
    gfx->print(timeCur);
    gfx->setCursor(barX + barW - 60, barY + barH + 6);
    gfx->print(timeTotal);
  }

  // 6. 歌名与歌手 (位置下移至 Y: 302, 336)
  if (hidMusicMode && linked && !appleMedia.available) {
    printCentered("PHONE MEDIA KEYS", CX, 304, 2, C_WHITE);
    printCentered("OPEN MUSIC APP ON PHONE", CX, 336, 2, C_GREY);
  } else if (linked || (musicHave && imgs[5].showW > 0)) {
    drawTextImg(5, 302, 0xFF);
    drawTextImg(6, 336, 0xBB);
  } else {
    printCentered("PHONE OFFLINE", CX, 316, 2, C_GREY);
  }

  // 7. 循环模式微型按钮 (Y: 382，不遮挡主界面，轻触可换循环模式)
  aaDisc(CX, 382, 22, 0x19A7);
  drawRepeatIcon(CX, 382, musicRepeatMode);

  // 8. 底部页面指示点
  drawPageDots();
  needFlush = true;
}
