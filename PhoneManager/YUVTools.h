#pragma once

void yuv420pToRGB32(const BYTE *yBuf, const BYTE *uvBuf, const int width, const int height, int lineSize, BYTE *rgbBuf);
void NV12ToRGB32(const BYTE *yBuf, const BYTE *uvBuf, const int width, const int height, int lineSize, BYTE *rgbBuf);


