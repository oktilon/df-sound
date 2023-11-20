#include <glib.h>
#include <stdlib.h>
#include <freetype/freetype.h>
#include <cairo-ft.h>
#include <pango/pango.h>
#include <math.h>

// Timeval
#include <locale.h>
// #include <stdio.h>
#include <sys/time.h>


#include "app.h"
#include "common.h"
#include "view_manager.h"
#include "views/view_pin.h"

#define UI_FILE             "pin.glade"
#define PIN_BUF_SZ          8
#define PIN_SIZE            4
#define PIN_MAX             6
#define LABEL_MAIN_TEXT     "To open the door\nplease enter the pin code"
#define LABEL_ERROR_TEXT    "Wrong password\nPlease try again"

static void pin_view_on_close_button_clicked (GtkButton *btn, gpointer data);
static void pin_view_on_digit_button_clicked (GtkButton *btn, gpointer data);
static void pin_view_on_ok_button_clicked (GtkButton *btn, gpointer data);
static void pin_view_on_cross_button_clicked (GtkButton *btn, gpointer data);
static void pin_view_reset ();
static void pin_view_update_pin ();

static GtkWidget            *window;
static GtkWidget            *btnOk;
static GtkWidget            *lblText;
static GtkWidget            *lblDigit[PIN_MAX];
static gchar                 pin[PIN_BUF_SZ];
static gboolean              isError;

void pin_view_init (GtkApplication *app, View *view) {
    selfLogWrn ("Create pin view");
    // Gtk builder
    gchar name[8] = {0};
    int i;
    GtkBuilder  *builder = gtk_builder_new_from_resource (UI_RESOURCE (UI_FILE));
    returnIfFailErr (builder, "View builder error: %m");

    selfLogWrn ("Get pin view window");
    // Get window object
    window = GTK_WIDGET (gtk_builder_get_object (builder, "pinWindow"));
    returnIfFailErr (window, "Get view window error: %m");

    // Set window to app
    gtk_window_set_application (GTK_WINDOW (window), app);
    // Set fullscreen
    ui_fullscreen (window);
    // Connect signals (glade file)
    gtk_builder_add_callback_symbol (builder, "btnDigitClicked", G_CALLBACK (pin_view_on_digit_button_clicked));
    gtk_builder_add_callback_symbol (builder, "btnOkClicked", G_CALLBACK (pin_view_on_ok_button_clicked));
    gtk_builder_add_callback_symbol (builder, "btnCrossClicked", G_CALLBACK (pin_view_on_cross_button_clicked));
    gtk_builder_add_callback_symbol (builder, "btnCloseClicked", G_CALLBACK (pin_view_on_close_button_clicked));
    gtk_builder_connect_signals (builder, NULL);

    // Get objects
    btnOk = GTK_WIDGET (gtk_builder_get_object (builder, "btnOk"));
    lblText = GTK_WIDGET (gtk_builder_get_object (builder, "lblText"));
    for (i = 0; i < PIN_MAX; i++) {
        sprintf (name, "lblD%u", i);
        lblDigit[i] = GTK_WIDGET (gtk_builder_get_object (builder, name));
    }

    memset (pin, 0, PIN_BUF_SZ);
    isError = FALSE;

    // Init View struct
    view->window = window;
    view->valid = TRUE;
    view->type = VIEW_TYPE_REGULAR;
    view->show = pin_view_show;
    view->hide = pin_view_hide;
}

AppState pin_view_show (AppState old, gpointer data) {
    if (!window) return old;
    isError = FALSE;
    pin_view_reset ();
    gtk_label_set_label (GTK_LABEL (lblText), _(LABEL_MAIN_TEXT));
    gtk_widget_show (window);
    gtk_window_present (GTK_WINDOW (window));
    return UI_CardPin;
}

void pin_view_hide () {
    if (!window) return;
    gtk_widget_hide (window);
}

void pin_view_error () {
    isError = TRUE;
    gtk_label_set_label (GTK_LABEL (lblText), _(LABEL_ERROR_TEXT));
    pin_view_reset ();
}

void pin_view_reset_countdown () {
    // Restart internal timer
    // Do nothing ?
}

static void pin_view_on_close_button_clicked (GtkButton *btn, gpointer data) {
    selfLogTrc ("Close button clicked");
    view_manager_return ();
}

static void pin_view_on_digit_button_clicked (GtkButton *btn, gpointer data) {
    const gchar *name = gtk_widget_get_name (GTK_WIDGET (btn));
    gchar key = name[3] - '0';
    if (key >= 0 && key <= 9) {
        strcat (pin, name + 3);
    }

    if (isError) {
        gtk_label_set_label (GTK_LABEL (lblText), _(LABEL_MAIN_TEXT));
        isError = FALSE;
    }

    pin_view_update_pin ();
    if (strlen (pin) >= PIN_MAX) {
        pin_view_on_ok_button_clicked (NULL, NULL);
    }
}

static void pin_view_on_ok_button_clicked (GtkButton *btn, gpointer data) {
    if (strlen(pin) < PIN_SIZE) return;
    view_manager_pin_entered (pin);
}

static void pin_view_on_cross_button_clicked (GtkButton *btn, gpointer data) {
    pin_view_reset ();
}

static void pin_view_reset () {
    memset (pin, 0, PIN_BUF_SZ);
    pin_view_update_pin ();
}

static void pin_view_update_pin () {
    guint i;
    const gchar *digit;
    GtkStyleContext *styleContext;
    GList *classes;

    for (i = 0; i < PIN_MAX; i++) {
        digit = pin[i] == 0 ? " " : "*"; // "•";
        // digit[0] = pin[i] == 0 ? ' ' : pin[i];
        gtk_label_set_label (GTK_LABEL (lblDigit[i]), digit);
    }
    gtk_widget_set_sensitive (GTK_WIDGET (btnOk), strlen (pin) >= PIN_SIZE);

    styleContext = gtk_widget_get_style_context (lblText);
    classes = gtk_style_context_list_classes (styleContext);
    g_list_foreach (classes, (GFunc)ui_remove_class, styleContext);
    gtk_style_context_add_class (styleContext, isError ? "pin_error" : "pin_text");
}