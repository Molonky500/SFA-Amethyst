#pragma once
extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <gccore.h>

void appFontInit();
void fontSetPos(int x, int y);
void fontSetSize(int size);
void fontSetColor(Color4b color);
int fontDrawString(const char *text);
void fontMeasureString(const char *text, int *outX, int *outY);

}; //extern "C"
