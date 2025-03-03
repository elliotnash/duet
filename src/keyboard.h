#pragma once

#include "context.h"

#define IN_BUFF_SIZE 16384

extern const char *keyboardVendorId;
extern const char *keyboardProductId;

void keyboard_watch(duet_context_t *status);

void keyboard_cleanup();
