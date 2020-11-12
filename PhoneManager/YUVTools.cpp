#include "stdafx.h"
#include "YUVTools.h"


void yuv420pToRGB32(const BYTE *yBuf, const BYTE *uBuf, const BYTE *vBuf, const int width, const int height, int lineSize, BYTE *rgbBuf) {    
	int dstIndex = 0;
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            BYTE y = yBuf[i * width + j];
            BYTE u = uBuf[(i / 2) * (width / 2) + j / 2];
            BYTE v = vBuf[(i / 2) * (width / 2) + j / 2];
 
			dstIndex = (width*i+j) * 4;
            int data = (int)(y + 1.772 * (u - 128));
            rgbBuf[dstIndex] = ((data < 0) ? 0 : (data > 255 ? 255 : data));
 
            data = (int)(y - 0.34414 * (u - 128) - 0.71414 * (v - 128));
            rgbBuf[dstIndex + 1] = ((data < 0) ? 0 : (data > 255 ? 255 : data));
 
            data = (int)(y + 1.402 * (v - 128));
            rgbBuf[dstIndex + 2] = ((data < 0) ? 0 : (data > 255 ? 255 : data));
        }
    }
}

void NV12ToRGB32(const BYTE *yBuf, const BYTE *uvBuf, const int width, const int height, int lineSize, BYTE *rgbBuf) {    
	int dstIndex = 0;
	int uvIndex = 0;
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            BYTE y = yBuf[i * width + j];
			uvIndex = i/2*width+j-j%2;
			BYTE u = uvBuf[uvIndex];
			BYTE v = uvBuf[uvIndex+1];
		 
			dstIndex = (width*i+j) * 4;
            int data = (int)(y + 1.772 * (u - 128));
            rgbBuf[dstIndex] = ((data < 0) ? 0 : (data > 255 ? 255 : data));
 
            data = (int)(y - 0.34414 * (u - 128) - 0.71414 * (v - 128));
            rgbBuf[dstIndex + 1] = ((data < 0) ? 0 : (data > 255 ? 255 : data));
 
            data = (int)(y + 1.402 * (v - 128));
            rgbBuf[dstIndex + 2] = ((data < 0) ? 0 : (data > 255 ? 255 : data));
        }
    }
}
