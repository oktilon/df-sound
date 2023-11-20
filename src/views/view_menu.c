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
#include "views/view_menu.h"
#include "download_manager.h"

#define UI_FILE             "menu.glade"

static void menu_view_on_settings_button_clicked (GtkButton *btn, gpointer data);
static void menu_view_on_apartments_button_clicked (GtkButton *btn, gpointer data);
static void menu_view_on_businesses_button_clicked (GtkButton *btn, gpointer data);

static GtkWidget            *window;
static GtkWidget            *lblName;
static GtkWidget            *imgGear;
static mdlDoorbellConfig     doorbellConfig;

void menu_view_init (GtkApplication *app, View *view) {
    // Gtk builder
    GtkBuilder  *builder = gtk_builder_new_from_resource (UI_RESOURCE (UI_FILE));
    returnIfFailErr (builder, "View builder error: %m");

    // Get window object
    window = GTK_WIDGET (gtk_builder_get_object (builder, "menuWindow"));
    returnIfFailErr (window, "Get view window error: %m");

    // Set window to app
    gtk_window_set_application (GTK_WINDOW (window), app);
    // Set fullscreen
    ui_fullscreen (window);
    // Connect signals (glade file)
    gtk_builder_add_callback_symbol (builder, "btnApartmentsClicked", G_CALLBACK (menu_view_on_apartments_button_clicked));
    gtk_builder_add_callback_symbol (builder, "btnBusinessesClicked", G_CALLBACK (menu_view_on_businesses_button_clicked));
    gtk_builder_add_callback_symbol (builder, "btnSettingsClicked", G_CALLBACK (menu_view_on_settings_button_clicked));
    gtk_builder_connect_signals (builder, NULL);

    // Get objects
    lblName = GTK_WIDGET (gtk_builder_get_object (builder, "lblName"));
    imgGear = GTK_WIDGET (gtk_builder_get_object (builder, "imgGear"));

    // Init View struct
    view->window = window;
    view->valid = TRUE;
    view->type = VIEW_TYPE_REGULAR;
    view->show = menu_view_show;
    view->hide = menu_view_hide;
}

void menu_view_new_data (MdlDoorbell *mdl) {
    doorbellConfig.config = doorbell_get_config (mdl);

    gtk_label_set_label (GTK_LABEL (lblName), doorbell_get_name (mdl));

    if (doorbellConfig.installationMode) {
        gtk_image_set_from_icon_name (GTK_IMAGE (imgGear), "defigo-gear", GTK_ICON_SIZE_DIALOG);
    } else {
        gtk_image_set_from_icon_name (GTK_IMAGE (imgGear), "defigo-blank", GTK_ICON_SIZE_DIALOG);
    }
}

AppState menu_view_show (AppState old, gpointer data) {
    if (!window) return old;
    gtk_widget_show (window);
    gtk_window_present (GTK_WINDOW (window));
    return UI_MainMenu;
}

void menu_view_hide () {
    if (!window) return;
    gtk_widget_hide (window);
}

static void menu_view_on_settings_button_clicked (GtkButton *btn, gpointer data) {
    selfLogTrc ("Setting button clicked");
    if (doorbellConfig.installationMode) {
        view_manager_settings ();
    }
}

static void menu_view_on_apartments_button_clicked (GtkButton *btn, gpointer data) {
    view_manager_apartments ();
}

static void menu_view_on_businesses_button_clicked (GtkButton *btn, gpointer data) {
    view_manager_businesses ();
}