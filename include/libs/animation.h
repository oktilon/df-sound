#pragma once

#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#define DURATION_INFINITE ((guint) 0xffffffff)


typedef enum AnimationTypeEnum {
    ANIMATION_TYPE_SPRING,
    ANIMATION_TYPE_TIMING
} AnimationType;

typedef enum AnimationStateEnum {
    ANIMATION_IDLE,
    ANIMATION_PAUSED,
    ANIMATION_PLAYING,
    ANIMATION_FINISHED,
} AnimationState;

typedef struct AnimationStruct {
    AnimationType   type;
    GtkWidget      *widget;
    double          value;
    AnimationState  state;
    glong           startTime;
    guint           tickCbId;
    guint           tick;
    guint           startTick;
    gpointer        userData;
    // Internal handlers
    double        (*calculate_value)   (struct AnimationStruct *, guint);
    guint         (*estimate_duration) (struct AnimationStruct *);
    void          (*deinit)            (struct AnimationStruct *);
    // User handlers
    void          (*valueHandler)      (double, struct AnimationStruct *, gpointer);
    void          (*doneHandler)       (gpointer);
} Animation;

typedef void (*AnimationDoneCallback) (gpointer);
typedef void (*AnimationValueCallback) (double, Animation*, gpointer);

void animation_init (Animation *self, GtkWidget *w, AnimationType t, AnimationValueCallback cb, gpointer userData);
void animation_deinit (Animation *self);
void animation_start (Animation *self);
void animation_set_done_handler (Animation *self, AnimationDoneCallback cb);
double animation_lerp (double a, double b, double t);
void animation_skip (Animation *self);