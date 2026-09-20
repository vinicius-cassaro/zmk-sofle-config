#pragma once

#include <stdint.h>

#include <zephyr/drivers/led_strip.h>

int eyelash_rgb_overlay_set(uint8_t index, struct led_rgb color);
int eyelash_rgb_overlay_clear(uint8_t index);
