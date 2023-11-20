#pragma once

#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "libs/animation.h"
#include "libs/easing.h"


typedef struct TimedAnimationStruct {
    Animation               base;
    double                  valueFrom;
    double                  valueTo;
    guint                   duration; /*ms*/
    Easing                  easing;
    guint                   repeatCount;
    gboolean                reverse;
    gboolean                alternate;
} TimedAnimation;

static inline TimedAnimation *TIMED_ANIMATION(gpointer ptr) { return (TimedAnimation *)ptr; }

Animation *timed_animation_new (GtkWidget *w, double beg, double end, guint duration, AnimationValueCallback cb, gpointer userData);

Easing timed_animation_get_easing (TimedAnimation *self);
void   timed_animation_set_easing (TimedAnimation *self, Easing easing);

guint timed_animation_get_repeatCount (TimedAnimation *self);
void  timed_animation_set_repeatCount (TimedAnimation *self, guint repeatCount);

gboolean timed_animation_get_reverse (TimedAnimation *self);
void     timed_animation_set_reverse (TimedAnimation *self, gboolean reverse);

gboolean timed_animation_get_alternate (TimedAnimation *self);
void     timed_animation_set_alternate (TimedAnimation *self, gboolean alternate);