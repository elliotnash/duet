#pragma once

#include "context.h"

void system_fmt(char *format, ...);

void setLayout(duet_context_t *status);

void setMirror();
void setSingleMonitor();
void setLandscape();
void setPortrait90();
void setPortrait270();
