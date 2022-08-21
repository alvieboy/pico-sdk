/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "hardware/gpio.h"
#include <stdlib.h>
struct gpio_listener_entry
{
    void (*gpio_changed_callback)(void *user, uint gpio, uint value);
    void *user;
    struct gpio_listener_entry *next;
};

#define GPIO_NO_PULL 0
#define GPIO_PULL_UP 1
#define GPIO_PULL_DOWN 2

#define GPIO_INPUT 0
#define GPIO_OUTPUT 1

struct gpio_entry
{
    struct gpio_listener_entry *listeners;
    uint value:1;
    uint external_value:1;
    uint external_drive:1;
    uint pull:2;
    uint drive:1;
    uint direction:1;
};

struct gpio_entry gpios[32] = {0};

void *gpio_add_listener(uint gpionum, void (*callback)(void*,uint,uint),
                        void *user)
{
    struct gpio_listener_entry *e = malloc(sizeof(struct gpio_listener_entry));
    e->user = user;
    e->gpio_changed_callback = callback;
    e->next = gpios[gpionum].listeners;
    gpios[gpionum].listeners = e;
    return e;
}

static int evaluate_gpio_value(uint gpio)
{
    if (gpios[gpio].direction == GPIO_OUTPUT)
        return gpios[gpio].value;
    // Input
    if (gpios[gpio].external_drive)
        return gpios[gpio].external_value;
    return gpios[gpio].pull == GPIO_PULL_UP;
}

static void propagate_if_needed(uint gpio, int oldvalue)
{
    int newvalue = evaluate_gpio_value(gpio);
    if (newvalue!=oldvalue) {
        struct gpio_listener_entry *e;
        for (e=gpios[gpio].listeners; e; e=e->next) {
            e->gpio_changed_callback(e->user, gpio, newvalue);
        }
    }
}

// todo weak or replace? probably weak
void gpio_set_function(uint gpio, enum gpio_function fn) {

}

void gpio_pull_up(uint gpio) {
    int oldvalue = evaluate_gpio_value(gpio);
    gpios[gpio].pull = GPIO_PULL_UP;
    propagate_if_needed(gpio, oldvalue);
}

void gpio_pull_down(uint gpio) {
    int oldvalue = evaluate_gpio_value(gpio);
    gpios[gpio].pull = GPIO_PULL_DOWN;
    propagate_if_needed(gpio, oldvalue);

}

void gpio_disable_pulls(uint gpio) {
    int oldvalue = evaluate_gpio_value(gpio);
    gpios[gpio].pull = GPIO_NO_PULL;
    propagate_if_needed(gpio, oldvalue);
}

void gpio_set_pulls(uint gpio, bool up, bool down) {
    int oldvalue = evaluate_gpio_value(gpio);
    gpios[gpio].pull = up ?GPIO_PULL_UP : down? GPIO_PULL_DOWN: 0;
    propagate_if_needed(gpio, oldvalue);
}

void gpio_set_irqover(uint gpio, uint value) {

}

void gpio_set_outover(uint gpio, uint value) {

}

void gpio_set_inover(uint gpio, uint value) {

}

void gpio_set_oeover(uint gpio, uint value) {

}

void gpio_set_input_hysteresis_enabled(uint gpio, bool enabled){

}

bool gpio_is_input_hysteresis_enabled(uint gpio){
    return true;
}

void gpio_set_slew_rate(uint gpio, enum gpio_slew_rate slew){

}

enum gpio_slew_rate gpio_get_slew_rate(uint gpio){
    return GPIO_SLEW_RATE_FAST;
}

void gpio_set_drive_strength(uint gpio, enum gpio_drive_strength drive){

}

enum gpio_drive_strength gpio_get_drive_strength(uint gpio){
    return GPIO_DRIVE_STRENGTH_4MA;
}


void gpio_set_irq_enabled(uint gpio, uint32_t events, bool enable) {

}

void gpio_acknowledge_irq(uint gpio, uint32_t events) {

}

void gpio_init(uint gpio) {

}

PICO_WEAK_FUNCTION_DEF(gpio_get)

bool PICO_WEAK_FUNCTION_IMPL_NAME(gpio_get)(uint gpio) {

    return evaluate_gpio_value(gpio) != 0;
}

uint32_t gpio_get_all() {
    return 0;
}

void gpio_set_mask(uint32_t mask) {

}

void gpio_clr_mask(uint32_t mask) {

}

void gpio_xor_mask(uint32_t mask) {

}

void gpio_put_masked(uint32_t mask, uint32_t value) {

}

void gpio_put_all(uint32_t value) {

}

void gpio_put(uint gpio, int value) {
    int oldvalue = evaluate_gpio_value(gpio);
    gpios[gpio].value = value;
    propagate_if_needed(gpio, oldvalue);
}

void gpio_set_dir_out_masked(uint32_t mask) {

}

void gpio_set_dir_in_masked(uint32_t mask) {

}

void gpio_set_dir_masked(uint32_t mask, uint32_t value) {

}

void gpio_set_dir_all_bits(uint32_t value) {

}

void gpio_set_dir(uint gpio, bool out) {
    int oldvalue = evaluate_gpio_value(gpio);
    gpios[gpio].direction = out?GPIO_OUTPUT:GPIO_INPUT;
    propagate_if_needed(gpio, oldvalue);
}

void gpio_debug_pins_init() {

}

void gpio_set_input_enabled(uint gpio, bool enable) {

}

void gpio_init_mask(uint gpio_mask) {

}

void gpio_set_external_drive(uint gpio, gpio_drive_t drive)
{
    switch (drive)
    {
    case GPIO_EXTERNAL_FLOAT:
        gpios[gpio].external_drive = false;
        break;
    case GPIO_EXTERNAL_PULLUP:
        gpios[gpio].external_drive = true;
        gpios[gpio].external_value = 1;
        break;
    case GPIO_EXTERNAL_PULLDOWN:
        gpios[gpio].external_drive = true;
        gpios[gpio].external_value = 0;
        break;
    case GPIO_EXTERNAL_DRIVE_LOW:
        gpios[gpio].external_drive = true;
        gpios[gpio].external_value = 0;
        break;
    case GPIO_EXTERNAL_DRIVE_HIGH:
        gpios[gpio].external_drive = true;
        gpios[gpio].external_value = 1;
        break;
    }
}
