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
extern int coverSlideDx;

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

// 15x13 精致心形点阵掩码 (彻底杜绝圆圈交叉杂线，空心剔透、实心饱满)
static const uint16_t heartFilledMask[13] = {
  0b0111000000111000, // row 0:  ***     ***
  0b1111100001111100, // row 1: *****   *****
  0b1111110011111100, // row 2: ****** ******
  0b1111111111111100, // row 3: **************
  0b1111111111111100, // row 4: **************
  0b0111111111111000, // row 5:  ************
  0b0111111111111000, // row 6:  ************
  0b0011111111110000, // row 7:   **********
  0b0001111111100000, // row 8:    ********
  0b0000111111000000, // row 9:     ******
  0b0000011110000000, // row 10:     ****
  0b0000001100000000, // row 11:      **
  0b0000000000000000  // row 12
};

static const uint16_t heartOutlineMask[13] = {
  0b0111000000111000, // row 0:  ***     ***
  0b1000100001000100, // row 1: *   *   *   *
  0b1000010010000100, // row 2: *    * *    *
  0b1000001010000010, // row 3: *     *     *
  0b0100000000000100, // row 4:  *         *
  0b0100000000000100, // row 5:  *         *
  0b0010000000001000, // row 6:   *       *
  0b0010000000001000, // row 7:   *       *
  0b0001000000010000, // row 8:    *     *
  0b0000100000100000, // row 9:     *   *
  0b0000010001000000, // row 10:     * *
  0b0000001110000000, // row 11:      *
  0b0000000000000000
};

static inline void drawHeartIcon(int cx, int cy, uint16_t color, bool filled) {
  const uint16_t *mask = filled ? heartFilledMask : heartOutlineMask;
  int startX = cx - 7;
  int startY = cy - 6;
  for (int r = 0; r < 13; r++) {
    uint16_t rowBits = mask[r];
    for (int c = 0; c < 15; c++) {
      if ((rowBits >> (15 - c)) & 1) {
        gfx->drawPixel(startX + c, startY + r, color);
      }
    }
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

// VOLUME 字母放到圆屏 9 点钟位置，紧贴屏幕边缘
static void drawVolumeTextRotated(uint16_t color) {
  const float R_text = 211.0f; // 紧贴音量轨内侧，极大贴近圆屏外边缘
  // 9 点钟方向为 p = 0.75，字母依序在 9 点钟上下沿切线展开
  const float p_start = 0.785f; // V 在上方 (约 9 点 25 分)
  const float p_end   = 0.715f; // E 在下方 (约 8 点 35 分)
  const float scale = 1.6f;

  for (int i = 0; i < 6; i++) {
    float p = p_start - i * ((p_start - p_end) / 5.0f);
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

// 绘制 9 点钟位置、更贴近屏幕边缘的无锯齿音量条
static void drawVolumeArc(int vol, bool showActive) {
  // 1. 常驻在 9 点钟贴边切线呈现 "VOLUME" (浅灰半透明)
  drawVolumeTextRotated(showActive ? 0xFFFF : 0xCE79);

  if (!showActive) return;

  // 2. 激活状态：紧贴屏幕外壳边缘 (外径 230，屏幕半径 233)，居中于 9 点钟 (0.75)
  // 圆周比例：0.68 (8 点 10 分，音量 0%) 到 0.82 (9 点 50 分，音量 100%)
  const float p_min = 0.68f;
  const float p_max = 0.82f;
  const float r_in = 222.0f;
  const float r_out = 230.0f;
  const float r_mid = 226.0f;
  const float half_th = 4.0f;

  // 绘制底轨槽 (深灰抗锯齿)
  fillRingBand(p_min, p_max, r_in, r_out, 0x2124);
  float a_min = p_min * 2.0f * (float)M_PI;
  aaDisc(CX + r_mid * sinf(a_min), CX - r_mid * cosf(a_min), half_th, 0x2124);
  float a_max = p_max * 2.0f * (float)M_PI;
  aaDisc(CX + r_mid * sinf(a_max), CX - r_mid * cosf(a_max), half_th, 0x2124);

  // 绘制音量填充弧：从底端 (0%) 顺时针向上生长到顶端 (100%)
  float p_fill = p_min + (p_max - p_min) * (constrain(vol, 0, 100) / 100.0f);
  if (vol > 0) {
    fillRingBand(p_min, p_fill, r_in, r_out, 0x07FF); // 高亮青色
    aaDisc(CX + r_mid * sinf(a_min), CX - r_mid * cosf(a_min), half_th, 0x07FF);
    float a_fill = p_fill * 2.0f * (float)M_PI;
    aaDisc(CX + r_mid * sinf(a_fill), CX - r_mid * cosf(a_fill), half_th, 0x07FF);
    // 滑块高亮小白点
    aaDisc(CX + r_mid * sinf(a_fill), CX - r_mid * cosf(a_fill), 6.0f, C_WHITE);
  }

  // 弧线内侧显示实时百分比大数字 (居中于 9 点钟内侧)
  char vbuf[16];
  snprintf(vbuf, sizeof(vbuf), "%d%%", vol);
  gfx->setTextColor(0x07FF);
  gfx->setTextSize(2);
  gfx->setCursor(34, CX - 8);
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
  const int covBaseX = CX - covSize / 2; // 143
  const int covBaseY = 56;

  // 原位置常驻半透明底图矩形边框与唱盘托架（切歌划开封面时自然露出底座）
  gfx->fillRoundRect(covBaseX - 3, covBaseY - 3, covSize + 6, covSize + 6, 14, 0x18C3);
  gfx->drawRoundRect(covBaseX - 3, covBaseY - 3, covSize + 6, covSize + 6, 14, 0x4A69);
  gfx->drawRoundRect(covBaseX - 1, covBaseY - 1, covSize + 2, covSize + 2, 12, 0x2945);
  // 底座内部同心凹槽（类似留声机托盘）
  gfx->drawCircle(CX, covBaseY + covSize / 2, 54, 0x2945);
  gfx->drawCircle(CX, covBaseY + covSize / 2, 24, 0x2945);

  // 封面实际绘制坐标（随滑动/离开动画水平偏移）
  int drawCovX = covBaseX + coverSlideDx;

  if (coverValid && coverBuf != nullptr) {
    gfx->fillRoundRect(drawCovX - 2, covBaseY - 2, covSize + 4, covSize + 4, 12, 0x2124);
    gfx->draw16bitRGBBitmap(drawCovX, covBaseY, coverBuf, covSize, covSize);
    gfx->drawRoundRect(drawCovX - 2, covBaseY - 2, covSize + 4, covSize + 4, 12, 0x5AEB);
  } else {
    // 拟物黑胶唱片
    const int discR = 86;
    const int discX = drawCovX + covSize / 2;
    const int discY = covBaseY + covSize / 2;
    aaDisc(discX, discY, discR, 0x10A2);
    gfx->drawCircle(discX, discY, 68, 0x2124);
    gfx->drawCircle(discX, discY, 48, 0x2124);
    aaDisc(discX, discY, 32, 0x249F);
    aaDisc(discX, discY, 8, C_BLACK);
    gfx->drawCircle(discX, discY, 8, 0xDEFB);
  }

  // 3. 嵌入专辑封面的播放/暂停按钮 (暂停时才显示大播放三角，大位移滑动时淡出)
  if (!musicPlaying && abs(coverSlideDx) < 60) {
    int pcx = drawCovX + covSize / 2, pcy = covBaseY + covSize / 2;
    aaDisc(pcx, pcy, 34, 0x18C3);
    gfx->fillTriangle(pcx - 10, pcy - 16, pcx - 10, pcy + 16, pcx + 16, pcy, C_WHITE);
  }

  // 4. 嵌入专辑封面右下角的 Like 按钮 (点击或双击封面触发，温润草莓红，绝不突兀)
  const int likeX = drawCovX + covSize - 22;
  const int likeY = covBaseY + covSize - 22;
  aaDisc(likeX, likeY, 18, musicLiked ? 0x4102 : 0x18C3);
  drawHeartIcon(likeX, likeY, musicLiked ? 0xFACD : 0xCE79, musicLiked);

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
