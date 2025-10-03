#pragma once

#include "context.h"
#include "config.h"

void system_fmt(char *format, ...);

void display_set_config(const duet_config_t *cfg);

void setLayout(duet_context_t *status);

void setMirror();
void setSingleMonitor();
void setLandscape();
void setPortrait90();
void setPortrait270();
