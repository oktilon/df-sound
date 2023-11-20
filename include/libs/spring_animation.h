#pragma once

#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "libs/animation.h"

typedef struct SpringParamsStruct {
    double damping;
    double mass;
    double stiffness;
} SpringParams;

typedef struct SpringAnimationStruct {
    Animation               base;
    double                  valueFrom;
    double                  valueTo;
    SpringParams           *springParams;

    double                  initialVelocity;
    double                  velocity;
    double                  epsilon;
    guint                   estimatedDuration; /*ms*/

} SpringAnimation;

static inline SpringAnimation *SPRING_ANIMATION(gpointer ptr) { return (SpringAnimation *)ptr; }

SpringParams *spring_animation_params_new_full (double damping, double mass, double stiffness);
SpringParams *spring_animation_params_new (double damping_ratio, double mass, double stiffness);
Animation *spring_animation_new (GtkWidget *w, double beg, double end, SpringParams *par, AnimationValueCallback cb, gpointer userData);
void spring_animation_set_epsilon (SpringAnimation *self, double epsilon);
void spring_animation_set_initial_velocity (SpringAnimation *self, double v0);
void spring_animation_set_offset (SpringAnimation *self, double offset);
guint spring_animation_get_estimated_duration (SpringAnimation *self);