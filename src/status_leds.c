#define DT_DRV_COMPAT zmk_behavior_eyelash_status_led

#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <drivers/behavior.h>
#include <drivers/ext_power.h>

#include <zmk/battery.h>
#include <zmk/behavior.h>
#include <zmk/ble.h>
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/hid_indicators_changed.h>
#include <zmk/endpoints.h>

#include "rgb_overlay.h"

#define STATUS_LED_NODE DT_NODELABEL(status_leds)
#define STRIP_LENGTH DT_PROP(DT_NODELABEL(led_overlay), chain_length)

#define STATUS_LED_PROFILE 0U
#define STATUS_LED_BATTERY 1U
#define STATUS_LED_NONE UINT8_MAX
#define STATUS_LED_SHOW_MS 3000U

#define ZERO_LED_INDEX DT_PROP(STATUS_LED_NODE, zero_led_index)
#define ONE_LED_INDEX DT_PROP(STATUS_LED_NODE, one_led_index)
#define TWO_LED_INDEX DT_PROP(STATUS_LED_NODE, two_led_index)
#define THREE_LED_INDEX DT_PROP(STATUS_LED_NODE, three_led_index)
#define FOUR_LED_INDEX DT_PROP(STATUS_LED_NODE, four_led_index)
#define FIVE_LED_INDEX DT_PROP(STATUS_LED_NODE, five_led_index)
#define SIX_LED_INDEX DT_PROP(STATUS_LED_NODE, six_led_index)

BUILD_ASSERT(ZERO_LED_INDEX < STRIP_LENGTH, "zero_led_index exceeds the strip length");
BUILD_ASSERT(ONE_LED_INDEX < STRIP_LENGTH, "one_led_index exceeds the strip length");
BUILD_ASSERT(TWO_LED_INDEX < STRIP_LENGTH, "two_led_index exceeds the strip length");
BUILD_ASSERT(THREE_LED_INDEX < STRIP_LENGTH, "three_led_index exceeds the strip length");
BUILD_ASSERT(FOUR_LED_INDEX < STRIP_LENGTH, "four_led_index exceeds the strip length");
BUILD_ASSERT(FIVE_LED_INDEX < STRIP_LENGTH, "five_led_index exceeds the strip length");
BUILD_ASSERT(SIX_LED_INDEX < STRIP_LENGTH, "six_led_index exceeds the strip length");

struct status_led_state {
    bool caps_lock;
    bool usb_powered;
    bool battery_charging;
    uint8_t battery_level;
    uint8_t display;
    int64_t display_started_at;
    int64_t display_until;
};

static struct status_led_state state = {.display = STATUS_LED_NONE};

static const struct led_rgb led_blue = {.r = 0, .g = 0, .b = 255};
static const struct led_rgb led_green = {.r = 0, .g = 255, .b = 0};
static const struct led_rgb led_yellow = {.r = 255, .g = 180, .b = 0};
static const struct led_rgb led_red = {.r = 255, .g = 0, .b = 0};
static const struct led_rgb led_white = {.r = 255, .g = 255, .b = 255};
static const struct led_rgb led_black = {.r = 0, .g = 0, .b = 0};

static void render_status_leds(struct k_work *work);
K_WORK_DELAYABLE_DEFINE(status_led_work, render_status_leds);

static void request_render(k_timeout_t delay) {
    k_work_reschedule(&status_led_work, delay);
}

static void clear_transient_overlays(void) {
    eyelash_rgb_overlay_clear(ZERO_LED_INDEX);
    eyelash_rgb_overlay_clear(ONE_LED_INDEX);
    eyelash_rgb_overlay_clear(TWO_LED_INDEX);
    eyelash_rgb_overlay_clear(THREE_LED_INDEX);
    eyelash_rgb_overlay_clear(FOUR_LED_INDEX);
    eyelash_rgb_overlay_clear(FIVE_LED_INDEX);
    eyelash_rgb_overlay_clear(SIX_LED_INDEX);
}

// static void render_caps_lock(void) {
//     if (state.caps_lock) {
//         eyelash_rgb_overlay_set(SIX_LED_INDEX, led_red);
//         zmk_rgb_set(rgb_colors, 29);
//     } else {
//         eyelash_rgb_overlay_clear(SIX_LED_INDEX);
//     }
// }

static struct led_rgb pulse_color(struct led_rgb color, int64_t elapsed_ms) {
    const uint32_t phase = elapsed_ms % 2000U;
    const uint8_t brightness =
        64U + ((phase <= 1000U ? phase : 2000U - phase) * 191U / 1000U);

    color.r = color.r * brightness / 255U;
    color.g = color.g * brightness / 255U;
    color.b = color.b * brightness / 255U;
    return color;
}

static void render_bluetooth(int64_t now) {
    const uint8_t flashes = zmk_ble_active_profile_index() + 1U;
    const int64_t elapsed = now - state.display_started_at;
    const int64_t flash_window = flashes * 500U;

    if (elapsed < flash_window) {
        if ((elapsed % 500U) < 250U) {
            eyelash_rgb_overlay_set(ZERO_LED_INDEX, led_blue);
        }
        return;
    }

    const struct zmk_endpoint_instance endpoint = zmk_endpoints_select();

    if (endpoint == ZMK_ENDPOINT_BLE) {
        if (zmk_ble_active_profile_is_connected()) {

            switch (zmk_ble_active_profile_index()) {
            case 0:
                eyelash_rgb_overlay_set(FIVE_LED_INDEX, led_blue);
                break;
            case 1:
                eyelash_rgb_overlay_set(FOUR_LED_INDEX, led_blue);
                break;
            case 2:
                eyelash_rgb_overlay_set(THREE_LED_INDEX, led_blue);
                break;
            case 3:
                eyelash_rgb_overlay_set(TWO_LED_INDEX, led_blue);
                break;
            default:
                eyelash_rgb_overlay_set(ONE_LED_INDEX, led_blue);
                break;
            }
        } else if (zmk_ble_active_profile_is_open()) {
            eyelash_rgb_overlay_set(ZERO_LED_INDEX, pulse_color(led_blue, elapsed - flash_window));
        } else if (((elapsed - flash_window) % 1000U) < 200U) {
            eyelash_rgb_overlay_set(ZERO_LED_INDEX, led_blue);
        }
    } else if (endpoint == ZMK_ENDPOINT_USB) {
        eyelash_rgb_overlay_set(ZERO_LED_INDEX, led_white);
    }
}

static void render_battery(void) {
    if (state.battery_charging) {
        eyelash_rgb_overlay_set(
            ZERO_LED_INDEX, pulse_color(led_white, k_uptime_get() - state.display_started_at));
    } else if (state.usb_powered) {
        eyelash_rgb_overlay_set(ZERO_LED_INDEX, led_white);
    } else if (state.battery_level > 60U) {
        eyelash_rgb_overlay_set(ZERO_LED_INDEX, led_green);
    } else if (state.battery_level >= 30U) {
        eyelash_rgb_overlay_set(ZERO_LED_INDEX, led_yellow);
    } else if (state.battery_level >= 15U) {
        eyelash_rgb_overlay_set(ZERO_LED_INDEX, led_red);
    } else {
        eyelash_rgb_overlay_set(ZERO_LED_INDEX, pulse_color(led_blue, k_uptime_get() - state.display_started_at));
    }
}

static void render_status_leds(struct k_work *work) {
    const int64_t now = k_uptime_get();

    ARG_UNUSED(work);
    clear_transient_overlays();
    // render_caps_lock();

    if (state.display == STATUS_LED_NONE) {
        return;
    }

    if (now >= state.display_until) {
        state.display = STATUS_LED_NONE;
        return;
    }

    if (state.display == STATUS_LED_PROFILE) {
        render_bluetooth(now);
        request_render(K_MSEC(CONFIG_EYELASH_SOFLE_STATUS_LED_PERIOD_MS));
    } else {
        render_battery();
        request_render(K_TIMEOUT_ABS_MS(state.display_until));
    }
}

static void show_status(uint8_t display) {
    const int64_t now = k_uptime_get();
    int64_t visible_for = STATUS_LED_SHOW_MS;

    state.display = display;
    state.display_started_at = now;
    state.display_until = now + visible_for;
    request_render(K_NO_WAIT);
}

static int status_led_behavior_pressed(struct zmk_behavior_binding *binding,
                                       struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);

    ARG_UNUSED(event);
    show_status(*(const uint8_t *)dev->config);
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api status_led_behavior_api = {
    .binding_pressed = status_led_behavior_pressed,
    .locality = BEHAVIOR_LOCALITY_EVENT_SOURCE,
};

#define STATUS_LED_BEHAVIOR(index)                                                               \
    static const uint8_t status_led_behavior_config_##index = DT_INST_PROP(index, action);      \
    BEHAVIOR_DT_INST_DEFINE(index, NULL, NULL, NULL,                                             \
                            &status_led_behavior_config_##index, POST_KERNEL,                    \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &status_led_behavior_api);

DT_INST_FOREACH_STATUS_OKAY(STATUS_LED_BEHAVIOR)

static int status_led_event_listener(const zmk_event_t *eh) {
    const struct zmk_hid_indicators_changed *hid = as_zmk_hid_indicators_changed(eh);

    if (hid != NULL) {
        state.caps_lock = (hid->indicators & BIT(1)) != 0;
        request_render(K_NO_WAIT);
        return ZMK_EV_EVENT_BUBBLE;
    }

    const struct zmk_battery_state_changed *battery = as_zmk_battery_state_changed(eh);

    if (battery != NULL) {
        state.battery_level = battery->state_of_charge;
        if (state.display == STATUS_LED_BATTERY) {
            request_render(K_NO_WAIT);
        }
        return ZMK_EV_EVENT_BUBBLE;
    }

    if (as_zmk_ble_active_profile_changed(eh) != NULL &&
        state.display == STATUS_LED_PROFILE) {
        show_status(STATUS_LED_PROFILE);
    }

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(eyelash_status_leds, status_led_event_listener);
ZMK_SUBSCRIPTION(eyelash_status_leds, zmk_hid_indicators_changed);
ZMK_SUBSCRIPTION(eyelash_status_leds, zmk_battery_state_changed);
ZMK_SUBSCRIPTION(eyelash_status_leds, zmk_ble_active_profile_changed);

static int status_led_init(void) {
    state.battery_level = zmk_battery_state_of_charge();
    request_render(K_NO_WAIT);
    return 0;
}

SYS_INIT(status_led_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
