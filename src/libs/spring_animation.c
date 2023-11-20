#include <math.h>

#include "libs/spring_animation.h"

#define DELTA 0.001
#define MAX_ITERATIONS 20000

static double oscillate (SpringAnimation *self, guint time, double *velocity) {
    double b = self->springParams->damping;
    double m = self->springParams->mass;
    double k = self->springParams->stiffness;
    double v0 = self->initialVelocity;

    double t = time / 1000.0;

    double beta = b / (2 * m);
    double omega0 = sqrt (k / m);

    double x0 = self->valueFrom - self->valueTo;

    double envelope = exp (-beta * t);

    /*
    * Solutions of the form C1*e^(lambda1*x) + C2*e^(lambda2*x)
    * for the differential equation m*ẍ+b*ẋ+kx = 0
    */

    /* Critically damped */
    /* DBL_EPSILON is too small for this specific comparison, so we use
    * FLT_EPSILON even though it's doubles */
    if (G_APPROX_VALUE (beta, omega0, FLT_EPSILON)) {
        if (velocity)
            *velocity = envelope * (-beta * t * v0 - beta * beta * t * x0 + v0);

        return self->valueTo + envelope * (x0 + (beta * x0 + v0) * t);
    }

    /* Underdamped */
    if (beta < omega0) {
        double omega1 = sqrt ((omega0 * omega0) - (beta * beta));

        if (velocity)
            *velocity = envelope * (v0 * cos (omega1 * t) - (x0 * omega1 + (beta * beta * x0 + beta * v0) / (omega1)) * sin (omega1 * t));

        return self->valueTo + envelope * (x0 * cos (omega1 * t) + ((beta * x0 + v0) / omega1) * sin (omega1 * t));
    }

    /* Overdamped */
    if (beta > omega0) {
        double omega2 = sqrt ((beta * beta) - (omega0 * omega0));

        if (velocity)
            *velocity = envelope * (v0 * coshl (omega2 * t) + (omega2 * x0 - (beta * beta * x0 + beta * v0) / omega2) * sinhl (omega2 * t));

        return self->valueTo + envelope * (x0 * coshl (omega2 * t) + ((beta * x0 + v0) / omega2) * sinhl (omega2 * t));
    }

    g_assert_not_reached ();
}

static guint get_start_tick (SpringAnimation *self, double startValue) {
    if (startValue < self->valueTo && startValue < self->valueFrom) return 0;
    if (startValue > self->valueTo && startValue > self->valueFrom) return 0;
    /* The first frame is not that important and we avoid finding the trivial 0
    * for in-place animations. */
    guint i = 1;
    double y = oscillate (self, i, NULL);

    while ((self->valueTo - self->valueFrom > DBL_EPSILON && startValue - y > self->epsilon) ||
            (self->valueFrom - self->valueTo > DBL_EPSILON && y - startValue > self->epsilon)) {
        if (i > MAX_ITERATIONS)
        return 0;

        y = oscillate (self, ++i, NULL);
    }

    return i;
}

static guint calculate_duration (SpringAnimation *self) {
    double beta = self->springParams->damping / (2 * self->springParams->mass);
    double omega0;
    double x0, y0;
    double x1, y1;
    double m;

    int i = 0;

    if (G_APPROX_VALUE (beta, 0, DBL_EPSILON) || beta < 0)
        return DURATION_INFINITE;

    omega0 = sqrt (self->springParams->stiffness / self->springParams->mass);

    /*
    * As first ansatz for the overdamped solution,
    * and general estimation for the oscillating ones
    * we take the value of the envelope when it's < epsilon
    */
    x0 = -log (self->epsilon) / beta;

    /* DBL_EPSILON is too small for this specific comparison, so we use
    * FLT_EPSILON even though it's doubles */
    if (G_APPROX_VALUE (beta, omega0, FLT_EPSILON) || beta < omega0)
        return x0 * 1000;

    /*
    * Since the overdamped solution decays way slower than the envelope
    * we need to use the value of the oscillation itself.
    * Newton's root finding method is a good candidate in this particular case:
    * https://en.wikipedia.org/wiki/Newton%27s_method
    */
    y0 = oscillate (self, x0 * 1000, NULL);
    m = (oscillate (self, (x0 + DELTA) * 1000, NULL) - y0) / DELTA;

    x1 = (self->valueTo - y0 + m * x0) / m;
    y1 = oscillate (self, x1 * 1000, NULL);

    while (ABS (self->valueTo - y1) > self->epsilon) {
        if (i>1000)
            return 0;

        x0 = x1;
        y0 = y1;

        m = (oscillate (self, (x0 + DELTA) * 1000, NULL) - y0) / DELTA;

        x1 = (self->valueTo - y0 + m * x0) / m;
        y1 = oscillate (self, x1 * 1000, NULL);
        i++;
    }

    return x1 * 1000;
}

static guint estimate_duration (Animation *base) {
    SpringAnimation *self = SPRING_ANIMATION(base);
    return self->estimatedDuration;
}

static double calculate_value (Animation *base, guint time) {
    SpringAnimation *self = SPRING_ANIMATION(base);
    return oscillate (self, time, &(self->velocity));
};

static void update_params (SpringAnimation *self) {
    self->base.value = calculate_value ((Animation *)self, 0);
    self->estimatedDuration = calculate_duration (self);
}

static void deinit (Animation *base) {
    SpringAnimation *self = SPRING_ANIMATION(base);
    g_free (self->springParams);
}

SpringParams *spring_animation_params_new_full (double damping, double mass, double stiffness) {
    SpringParams *self = g_new0 (SpringParams, 1);

    self->damping = damping;
    self->mass = mass;
    self->stiffness = stiffness;

    return self;
}

SpringParams *spring_animation_params_new (double damping_ratio, double mass, double stiffness) {
    double critical_damping, damping;

    critical_damping = 2 * sqrt (mass * stiffness);
    damping = damping_ratio * critical_damping;

    return spring_animation_params_new_full (damping, mass, stiffness);
}

Animation *spring_animation_new (GtkWidget *w, double beg, double end, SpringParams *par, AnimationValueCallback cb, gpointer userData) {
    SpringAnimation *self;
    Animation *base;

    self = g_new0 (SpringAnimation, 1);
    base = (Animation *)self;
    // Init base
    animation_init (base, w, ANIMATION_TYPE_SPRING, cb, userData);
    base->calculate_value = calculate_value;
    base->estimate_duration = estimate_duration;
    base->deinit = deinit;

    // Init Spring
    self->springParams = par;
    self->epsilon = 0.001;
    self->valueFrom = beg;
    self->valueTo = end;
    self->velocity = 0.0;
    self->initialVelocity = 0.0;

    update_params (self);

    return base;
}

void spring_animation_set_epsilon (SpringAnimation *self, double epsilon) {
    self->epsilon = epsilon;
    update_params (self);
}

void spring_animation_set_initial_velocity (SpringAnimation *self, double v0) {
    self->initialVelocity = v0;
    update_params (self);
}

void spring_animation_set_offset (SpringAnimation *self, double startValue) {
    self->base.startTick = get_start_tick (self, startValue);
}

guint spring_animation_get_estimated_duration (SpringAnimation *self) {
    return self->estimatedDuration;
}
