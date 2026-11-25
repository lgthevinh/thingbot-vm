#include "native.h"
#include "esp_log.h"
#include "thingbot.h"

static const char *TAG = "native";

static bool check_act(vm_t *vm, esp_err_t err) {
    if (err == ESP_OK) {
        return true;
    }

    ESP_LOGE(TAG, "act failed: %s, ip %d", esp_err_to_name(err), vm->ip - 2);
    vm->running = false;
    return false;
}

static void native_motor(vm_t *vm) {
    thingbot_t *bot = vm->ctx;
    int speed = vm_pop(vm);
    int id = vm_pop(vm);
    if (!check_act(vm, act_set_motor_speed(&bot->act, id, speed))) {
        return;
    }

    ESP_LOGD(TAG, "motor %d speed %d, ip %d, sp %d", id, speed, vm->ip - 2, vm->sp);
}

static void native_servo(vm_t *vm) {
    thingbot_t *bot = vm->ctx;
    int angle = vm_pop(vm);
    int id = vm_pop(vm);
    if (!check_act(vm, act_set_servo_angle(&bot->act, id, angle))) {
        return;
    }

    ESP_LOGD(TAG, "servo %d angle %d, ip %d, sp %d", id, angle, vm->ip - 2, vm->sp);
}

static void native_spin(vm_t *vm) {
    thingbot_t *bot = vm->ctx;
    int speed = vm_pop(vm);
    int id = vm_pop(vm);
    if (!check_act(vm, act_set_servo_speed(&bot->act, id, speed))) {
        return;
    }

    ESP_LOGD(TAG, "spin %d speed %d, ip %d, sp %d", id, speed, vm->ip - 2, vm->sp);
}

static void native_buzz(vm_t *vm) {
    thingbot_t *bot = vm->ctx;
    int duty = vm_pop(vm);
    if (!check_act(vm, act_set_buzzer(&bot->act, duty))) {
        return;
    }

    ESP_LOGD(TAG, "buzz duty %d, ip %d, sp %d", duty, vm->ip - 2, vm->sp);
}

static void native_led(vm_t *vm) {
    thingbot_t *bot = vm->ctx;
    int brightness = vm_pop(vm);
    int id = vm_pop(vm);
    if (!check_act(vm, act_set_led(&bot->act, id, brightness))) {
        return;
    }

    ESP_LOGD(TAG, "led %d brightness %d, ip %d, sp %d", id, brightness, vm->ip - 2, vm->sp);
}

const NativeFn thingbot_natives[NATIVE_COUNT] = {
    [MOTOR] = native_motor,
    [SERVO] = native_servo,
    [SPIN] = native_spin,
    [BUZZ] = native_buzz,
    [LED] = native_led,
};
