#pragma once

#include <glib.h>

typedef enum {
    EASING_LINEAR,
    EASING_EASE_IN_QUAD,
    EASING_EASE_OUT_QUAD,
    EASING_EASE_IN_OUT_QUAD,
    EASING_EASE_IN_CUBIC,
    EASING_EASE_OUT_CUBIC,
    EASING_EASE_IN_OUT_CUBIC,
    EASING_EASE_IN_QUART,
    EASING_EASE_OUT_QUART,
    EASING_EASE_IN_OUT_QUART,
    EASING_EASE_IN_QUINT,
    EASING_EASE_OUT_QUINT,
    EASING_EASE_IN_OUT_QUINT,
    EASING_EASE_IN_SINE,
    EASING_EASE_OUT_SINE,
    EASING_EASE_IN_OUT_SINE,
    EASING_EASE_IN_EXPO,
    EASING_EASE_OUT_EXPO,
    EASING_EASE_IN_OUT_EXPO,
    EASING_EASE_IN_CIRC,
    EASING_EASE_OUT_CIRC,
    EASING_EASE_IN_OUT_CIRC,
    EASING_EASE_IN_ELASTIC,
    EASING_EASE_OUT_ELASTIC,
    EASING_EASE_IN_OUT_ELASTIC,
    EASING_EASE_IN_BACK,
    EASING_EASE_OUT_BACK,
    EASING_EASE_IN_OUT_BACK,
    EASING_EASE_IN_BOUNCE,
    EASING_EASE_OUT_BOUNCE,
    EASING_EASE_IN_OUT_BOUNCE,
} Easing;

double easing_ease (Easing self, double value);
