#include "native.h"
#include "esp_log.h"
#include "runtime.h"
#include <stdbool.h>

static const char *TAG = "native";

static bool check_act(engine_t *e, esp_err_t err) {
    if (err == ESP_OK) {
        return true;
    }

    ESP_LOGE(TAG, "act failed: %s, pc %d", esp_err_to_name(err), e->pc - 2);
    e->status = ENGINE_HALTED;
    return false;
}

static void native_motor(engine_t *e, runtime_t *rt) {
    int speed = engine_pop(e);
    int id = engine_pop(e);
    if (!check_act(e, act_set_motor_speed(&rt->act, id, speed))) {
        return;
    }

    ESP_LOGD(TAG, "motor %d speed %d, pc %d, sp %d", id, speed, e->pc - 2, e->sp);
}

static void native_servo(engine_t *e, runtime_t *rt) {
    int angle = engine_pop(e);
    int id = engine_pop(e);
    if (!check_act(e, act_set_servo_angle(&rt->act, id, angle))) {
        return;
    }

    ESP_LOGD(TAG, "servo %d angle %d, pc %d, sp %d", id, angle, e->pc - 2, e->sp);
}

static void native_spin(engine_t *e, runtime_t *rt) {
    int speed = engine_pop(e);
    int id = engine_pop(e);
    if (!check_act(e, act_set_servo_speed(&rt->act, id, speed))) {
        return;
    }

    ESP_LOGD(TAG, "spin %d speed %d, pc %d, sp %d", id, speed, e->pc - 2, e->sp);
}

static void native_buzz(engine_t *e, runtime_t *rt) {
    int duty = engine_pop(e);
    if (!check_act(e, act_set_buzzer(&rt->act, duty))) {
        return;
    }

    ESP_LOGD(TAG, "buzz duty %d, pc %d, sp %d", duty, e->pc - 2, e->sp);
}

static void native_led(engine_t *e, runtime_t *rt) {
    int brightness = engine_pop(e);
    int id = engine_pop(e);
    if (!check_act(e, act_set_led(&rt->act, id, brightness))) {
        return;
    }

    ESP_LOGD(TAG, "led %d brightness %d, pc %d, sp %d", id, brightness, e->pc - 2, e->sp);
}

const NativeFn thingbot_natives[NATIVE_COUNT] = {
    [MOTOR] = native_motor,
    [SERVO] = native_servo,
    [SPIN] = native_spin,
    [BUZZ] = native_buzz,
    [LED] = native_led,
};
