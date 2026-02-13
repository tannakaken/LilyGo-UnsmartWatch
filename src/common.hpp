#ifndef COMMON_HPP_
#define COMMON_HPP_
#include <cstdint>

// なぜか画面の大きさちょうどを目指すとと画面の下の方が黒くなって、エラーになって、画像描画ができないので、余裕を持たせる。
// 画面の幅が240x240
static const uint8_t CANVAS_WIDTH = 200;
static const uint8_t CANVAS_HEIGHT = 200;
static const uint8_t HALF_WIDTH = CANVAS_WIDTH / 2;
static const uint8_t HALF_HEIGHT = CANVAS_HEIGHT / 2;
static int32_t center_x = HALF_WIDTH;
static int32_t center_y = HALF_HEIGHT;

#endif // COMMON_HPP_