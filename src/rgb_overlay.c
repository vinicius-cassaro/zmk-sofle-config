#define DT_DRV_COMPAT eyelash_led_strip_overlay

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/kernel.h>

#include "rgb_overlay.h"

#define OVERLAY_NODE DT_NODELABEL(led_overlay)
#define OVERLAY_LENGTH DT_PROP(OVERLAY_NODE, chain_length)

struct overlay_config {
    const struct device *target;
};

struct overlay_data {
    struct k_spinlock lock;
    struct led_rgb pixels[OVERLAY_LENGTH];
    bool enabled[OVERLAY_LENGTH];
};

static struct overlay_data overlay_data;

static int overlay_update_rgb(const struct device *dev, struct led_rgb *pixels,
                              size_t num_pixels) {
    struct overlay_data *data = dev->data;
    k_spinlock_key_t key;

    if (num_pixels > OVERLAY_LENGTH) {
        return -EINVAL;
    }

    key = k_spin_lock(&data->lock);
    for (size_t i = 0; i < num_pixels; i++) {
        if (data->enabled[i]) {
            pixels[i] = data->pixels[i];
        }
    }
    k_spin_unlock(&data->lock, key);

    return led_strip_update_rgb(((const struct overlay_config *)dev->config)->target, pixels,
                                num_pixels);
}

static int overlay_init(const struct device *dev) {
    ARG_UNUSED(dev);
    return 0;
}

static const struct led_strip_driver_api overlay_api = {
    .update_rgb = overlay_update_rgb,
};

static const struct overlay_config overlay_config = {
    .target = DEVICE_DT_GET(DT_PHANDLE(OVERLAY_NODE, target)),
};

DEVICE_DT_DEFINE(OVERLAY_NODE, overlay_init, NULL, &overlay_data, &overlay_config, POST_KERNEL,
                 CONFIG_LED_STRIP_INIT_PRIORITY, &overlay_api);

int eyelash_rgb_overlay_set(uint8_t index, struct led_rgb color) {
    k_spinlock_key_t key;

    if (index >= OVERLAY_LENGTH) {
        return -EINVAL;
    }

    key = k_spin_lock(&overlay_data.lock);
    overlay_data.pixels[index] = color;
    overlay_data.enabled[index] = true;
    k_spin_unlock(&overlay_data.lock, key);

    return 0;
}

int eyelash_rgb_overlay_clear(uint8_t index) {
    k_spinlock_key_t key;

    if (index >= OVERLAY_LENGTH) {
        return -EINVAL;
    }

    key = k_spin_lock(&overlay_data.lock);
    overlay_data.enabled[index] = false;
    k_spin_unlock(&overlay_data.lock, key);

    return 0;
}
