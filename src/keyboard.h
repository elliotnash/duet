#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

#include "context.h"

#define IN_BUFF_SIZE 16384

extern const uint16_t keyboardVendorId;
extern const uint16_t keyboardProductId;

int get_keyboard_status();

void keyboard_watch(int fd, duet_context_t *status);

void keyboard_cleanup();

#endif