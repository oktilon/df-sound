#include <math.h>

#include "libs/animation.h"

#define DELTA 0.001
#define MAX_ITERATIONS 20000

static void set_value (Animation *self, guint t) {
    self->value = self->calculate_value (self, t);
    if (self->valueHandler) {
        self->valueHandler (self->value, self, self->userData);
    }
}

static void stop_animation (Animation *self) {
    g_return_if_fail(self);
    if (self->tickCbId) {
        gtk_widget_remove_tick_callback (self->widget, self->tickCbId);
        self->tickCbId = 0;
    }
}

static gboolean tick_cb (GtkWidget *widget, GdkFrameClock *frameClock, Animation *self) {
    g_return_val_if_fail(self, G_SOURCE_REMOVE);
    g_return_val_if_fail(frameClock, G_SOURCE_REMOVE);

    gulong frameTime = gdk_frame_clock_get_frame_time (frameClock) / 1000; /* ms */
    guint duration = self->estimate_duration (self);
    guint t = (guint) (frameTime - self->startTime);

    self->tick = t;

    if (t >= duration && duration != DURATION_INFINITE) {
        animation_skip (self);

        return G_SOURCE_REMOVE;
    }

    set_value (self, t);

    return G_SOURCE_CONTINUE;
}

/**
 * From adw_lerp:
 * @a: the start
 * @b: the end
 * @t: the interpolation rate
 *
 * Computes the linear interpolation between @a and @b for @t.
 *
 * Returns: the computed value
 */
double animation_lerp (double a, double b, double t) {
    return a * (1.0 - t) + b * t;
}


void animation_skip (Animation *self) {

    if (self->state == ANIMATION_FINISHED)
        return;

    self->state = ANIMATION_FINISHED;

    stop_animation (self);

    set_value (self, self->estimate_duration (self));

    self->startTime = 0;

    if (self->doneHandler) {
        self->doneHandler (self->userData);
    }
}

void animation_deinit (Animation *self) {
    self->deinit (self);
    g_free (self);
}

void animation_init (Animation *self, GtkWidget *w, AnimationType t, AnimationValueCallback cb, gpointer userData) {
    self->widget = w;
    self->type = t;
    self->state = ANIMATION_IDLE;
    self->startTime = 0;
    self->tickCbId = 0;
    self->doneHandler = NULL;
    self->valueHandler = cb;
    self->userData = userData;
    self->tick = 0;
    self->startTick = 0;
}

void animation_start (Animation *self) {
    self->state = ANIMATION_PLAYING;

    self->startTime = gdk_frame_clock_get_frame_time (gtk_widget_get_frame_clock (self->widget)) / 1000;
    self->startTime -= self->startTick;
    self->tickCbId = gtk_widget_add_tick_callback (self->widget, (GtkTickCallback) tick_cb, self, NULL);

}

void animation_set_done_handler (Animation *self, AnimationDoneCallback cb) {
    self->doneHandler = cb;
}
