#ifndef DISPLAY_H
#define DISPLAY_H

#define MODE_AUTO 0
#define MODE_MIRROR 1
#define MODE_LANDSCAPE 2
#define MODE_PORTRAIT_90 3
#define MODE_PORTRAIT_270 4

#define ROTATION_LANDSCAPE 0
#define ROTATION_PORTRAIT_90 1
#define ROTATION_PORTRAIT_270 2

void system_fmt(char* format, ...);

void setLayout(int keyboardConnected, int rotation, int mode);

void setMirror();
void setSingleMonitor();
void setLandscape();
void setPortrait90();
void setPortrait270();

#endif