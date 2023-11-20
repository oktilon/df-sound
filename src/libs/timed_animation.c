#include <math.h>

#include "libs/timed_animation.h"


static guint estimate_duration (Animation *animation) {
    TimedAnimation *self = TIMED_ANIMATION (animation);

    if (self->repeatCount == 0)
        return DURATION_INFINITE;

    return self->duration * self->repeatCount;
}

static double calculate_value (Animation *animation, guint t) {
    TimedAnimation *self = TIMED_ANIMATION (animation);

    double value;
    double iteration, progress;
    gboolean reverse = FALSE;

    if (self->duration == 0)
        return self->valueTo;

    progress = modf (((double) t / self->duration), &iteration);

    if (self->alternate)
        reverse = ((int) iteration % 2);

    if (self->reverse)
        reverse = !reverse;

    /* When the animation ends, return the exact final value, which depends on the
        direction the animation is going at that moment, having into account that at the
        time of this check we're already on the next iteration. */
    if (t >= estimate_duration (animation))
        return self->alternate == reverse ? self->valueTo : self->valueFrom;

    progress = reverse ? (1 - progress) : progress;

    value = easing_ease (self->easing, progress);

    return animation_lerp (self->valueFrom, self->valueTo, value);
}

static void deinit (Animation *_base) {
    // Do nothing
}

Animation *timed_animation_new (GtkWidget *w, double beg, double end, guint duration, AnimationValueCallback cb, gpointer userData) {
    TimedAnimation *self;
    Animation *base;

    self = g_new0 (TimedAnimation, 1);
    base = (Animation *)self;
    // Init base
    animation_init (base, w, ANIMATION_TYPE_TIMING, cb, userData);
    base->calculate_value = calculate_value;
    base->estimate_duration = estimate_duration;
    base->deinit = deinit;

    // Init Spring
    self->duration      = duration;
    self->valueFrom     = beg;
    self->valueTo       = end;
    self->easing        = EASING_EASE_OUT_CUBIC;
    self->repeatCount  = 1;
    self->reverse       = FALSE;
    self->alternate     = FALSE;

    return base;
}

Easing timed_animation_get_easing (TimedAnimation *self) {
    return self->easing;
}
void timed_animation_set_easing (TimedAnimation *self, Easing easing) {
    self->easing = easing;
}

guint timed_animation_get_repeatCount (TimedAnimation *self) {
    return self->repeatCount;
}
void timed_animation_set_repeatCount (TimedAnimation *self, guint repeatCount) {
    self->repeatCount = repeatCount;
}

gboolean timed_animation_get_reverse (TimedAnimation *self) {
    return self->reverse;
}
void timed_animation_set_reverse (TimedAnimation *self, gboolean reverse) {
    self->reverse = reverse;
}

gboolean timed_animation_get_alternate (TimedAnimation *self) {
    return self->alternate;
}
void timed_animation_set_alternate (TimedAnimation *self, gboolean alternate) {
    self->alternate = alternate;
}