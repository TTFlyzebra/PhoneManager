#pragma once

void yuv420pToRGB32(const BYTE *yBuf, const BYTE *uBuf, const BYTE *vBuf, const int width, const int height, int lineSize, BYTE *rgbBuf);

