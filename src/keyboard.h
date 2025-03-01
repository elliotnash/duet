#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

#include "context.h"

#define IN_BUFF_SIZE 16384

extern const char* keyboardVendorId;
extern const char* keyboardProductId;

void keyboard_watch(duet_context_t *status);

void keyboard_cleanup();

#endif