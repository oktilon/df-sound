#include <stdlib.h>
#include "app.h"
#include "keypad.h"
#include "views/view_main.h"

#define KEY_BACKSPACE               '\b'
#define KEY_BUF_SIZE                7

/**
 * @brief Hide keyboard timeout (sec)
 * to hide keyboard on user inactive
 */
#define TIMEOUT_HIDE_KEYBOARD       60

/**
 * @brief Reset search timeout (sec)
 * to reset search on user inactive
 */
#define TIMEOUT_RESET_SEARCH        120

/**
 * @brief Keys row structure
 * for keyboard layout
 */
typedef struct KeyRowStruct {
    gint left;
    const gchar *keys;
} KeyRow;

/**
 * @brief Key data (for GtkButton click event)
 */
typedef struct KeyDataStruct {
    gunichar key;
} KeyData;

// Keyboard layout dimensions
static gint keyWidth = 64;
static gint keyHeight = 64;
static gint keyHSpace = 11;
static gint keypadTop = 33;

/********************************************
*         L A Y O U T S                     *
********************************************/
// English
static const KeyRow         rowsEn[] = {
    {  35, "1234567890" },
    {  35, "qwertyuiop" },
    {  68, "asdfghjkl" },
    { 102, "zxcvbnm\b" },
    { 177, " ß" },
    {  -1, NULL }
};

// Global variables
static gchar                singleChar[KEY_BUF_SIZE]    = {0};
static GtkWidget           *keypad;
static GtkWidget           *boxKeypad;
static GtkWidget           *lblSearch;
static guint                timerReset = 0;
static guint                timerHide = 0;

/**
 * @brief Reset timer handler
 *
 * @param _d unused
 * @return gboolean G_SOURCE_REMOVE to stop timer
 */
static gboolean keypad_timeout_reset (gpointer _d) {
    timerReset = 0;
    selfLogTrc ("keypad: Reset search timeout");
    // Reset search
    main_view_stop_search (TRUE);
    return G_SOURCE_REMOVE;
}

/**
 * @brief Stops reset timer if exists
 */
static void keypad_stop_reset_timer () {
    if (timerReset) {
        g_source_remove (timerReset);
        timerReset = 0;
    }
}

/**
 * @brief Starts new reset timer
 */
static void keypad_start_reset_timer () {
    keypad_stop_reset_timer ();
    timerReset = g_timeout_add (TIMEOUT_RESET_SEARCH * 1000, keypad_timeout_reset, NULL);
}

/**
 * @brief Hide timer handler
 *
 * @param _d unused
 * @return gboolean G_SOURCE_REMOVE to stop timer
 */
static gboolean keypad_timeout_hide (gpointer _d) {
    timerHide = 0;
    selfLogTrc ("keypad: Hide keyboard timeout");
    main_view_stop_search (FALSE);
    return G_SOURCE_REMOVE;
}

/**
 * @brief Stops hide timer if exists
 */
static void keypad_stop_hide_timer () {
    if (timerHide) {
        g_source_remove (timerHide);
        timerHide = 0;
    }
}

/**
 * @brief Starts new hide timer
 */
static void keypad_start_hide_timer () {
    keypad_stop_hide_timer ();
    timerHide = g_timeout_add (TIMEOUT_HIDE_KEYBOARD * 1000, keypad_timeout_hide, NULL);
}

/**
 * @brief Delete widget from container
 * for gtk_container_foreach
 * @param w widget
 * @param c container
 */
static void keypad_delete_container_item (GtkWidget *w, gpointer c) {
    gtk_container_remove (GTK_CONTAINER (c), w);
}

/**
 * @brief Key click handler
 *
 * @param _b unused
 * @param keyData Key data (with unicode char)
 */
static void on_keypad_button_clicked (GtkWidget *_b, KeyData *keyData) {
    // Variables
    int i;
    gchar *val, *end;
    gunichar ch;

    // Clear converter buffer
    memset (singleChar, 0, KEY_BUF_SIZE);
    // Get current text
    GString *text = g_string_new (gtk_label_get_label (GTK_LABEL (lblSearch)));
    if (keyData->key == KEY_BACKSPACE) { // backspace key
        // Get text end pointer
        end = text->str + text->len;
        // Find previous utf8 char
        end = g_utf8_find_prev_char (text->str, end);
        if (end) { // If there is a char
            // Convert this utf8 char to unicode
            ch = g_utf8_get_char (end);
            // Convert single unicode to utf8 string back, and get it size
            i = g_unichar_to_utf8 (ch, singleChar);
            // Truncate string on specific size
            g_string_truncate (text, text->len - i);
            selfLogTrc ("Delete char [%s] sz=%d", singleChar, i);
        }
    } else { // Regular key
        // Convert key unicode char to utf8 string
        (void)g_unichar_to_utf8 (keyData->key, singleChar);
        // Append it to text
        g_string_append (text, singleChar);
        selfLogTrc ("Button [%s]", singleChar);
    }
    // Free GString struct
    val = g_string_free (text, FALSE);
    // Update label
    gtk_label_set_label (GTK_LABEL (lblSearch), val);
    // Free text buffer
    g_free (val);
    // Update Main View
    main_view_on_search_update ();
    // Restart user inactivity timers
    keypad_start_hide_timer ();
    keypad_start_reset_timer ();
}

/**
 * @brief Gets current search text
 *
 * @return const char* search text
 */
const char *keypad_get_text () {
    const char *ret = NULL;
    if (lblSearch) {
        ret = gtk_label_get_label (GTK_LABEL (lblSearch));
    }
    return ret;
}

/**
 * @brief Gets current search text width
 *
 * @return double text width
 */
int keypad_get_text_width () {
    int ret = 0;
    if (lblSearch) {
        // Get label width
        ret = gtk_widget_get_allocated_width (lblSearch);
    }
    return ret;
}

/**
 * @brief Show user search keyboard
 *
 * @return gboolean success
 */
gboolean keypad_show () {
    if (keypad) {
        selfLogTrc ("Show keypad");
        gtk_widget_set_visible (keypad, TRUE);
        // Start user inactivity timers
        keypad_start_hide_timer ();
        if (lblSearch)
            keypad_start_reset_timer ();
        return TRUE;
    }
    return FALSE;
}

/**
 * @brief Hide user search keyboard
 *
 * @param clear hide with reset?
 * @return gboolean filter has text
 */
gboolean keypad_hide (gboolean clear) {
    gboolean ret = FALSE;
    const char *text;
    if (keypad) {
        gtk_widget_set_visible (keypad, FALSE);
        keypad_stop_hide_timer ();
    }
    if (lblSearch) {
        if (clear) {
            gtk_label_set_label (GTK_LABEL (lblSearch), "");
            keypad_stop_reset_timer ();
        } else {
            text = keypad_get_text ();
            if (text && g_utf8_strlen (text, -1) > 0)
                ret = TRUE;
        }
    }
    return ret;
}

// static void keypad_child_revealed (GtkWidget *revealer, GParamSpec* pspec, gpointer _data) {
//     selfLogTrc ("child_revealed");
// }

void keypad_init (GtkBuilder *builder) {
    gint wd;
    KeyData *userData;
    GtkWidget *boxRow;
    GtkWidget *btnKey;
    GtkStyleContext *styleContext;
    const KeyRow *row;
    const gchar *keyClass;
    gchar *next = NULL, *end;
    gunichar key = 0;
    gboolean firstRow = TRUE;

    keypad = GTK_WIDGET (gtk_builder_get_object (builder, "keypad"));
    boxKeypad = GTK_WIDGET (gtk_builder_get_object (builder, "boxKeypad"));
    lblSearch = GTK_WIDGET (gtk_builder_get_object (builder, "lblSearch"));


    // Remove demo keys from keypad
    gtk_container_foreach (GTK_CONTAINER (boxKeypad), keypad_delete_container_item, boxKeypad);

    // Reset demo text
    gtk_label_set_label (GTK_LABEL (lblSearch), "");

    // TODO Select keyboard here
    row = rowsEn;

    // Keys top offset
    // y = keysTop;
    // For each key rows
    while (row->left >= 0) {
        // Key iterator
        next = (char *)(row->keys);
        // End pointer
        end = next + strlen(next);
        // Convert first UTF8 char to Unicode
        key = g_utf8_get_char (next);
        // Row box
        boxRow = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, keyHSpace);
        gtk_widget_set_visible (boxRow, TRUE);
        gtk_widget_set_halign (boxRow, GTK_ALIGN_CENTER);
        if (firstRow) {
            gtk_widget_set_margin_top (boxRow, keypadTop);
            firstRow = FALSE;
        }

        // Foreach key in row
        while (key) {
            // Create key data
            userData = g_new0 (KeyData, 1);
            userData->key = key;
            // Key CSS-style
            keyClass = "keypad_key";

            // Check if it's a special char
            switch(key) {
                case ' ': // Space key
                    btnKey = gtk_button_new_with_label (" ");
                    // Space key has wide size
                    wd = 6 * keyWidth + 5 * keyHSpace;
                    break;

                case KEY_BACKSPACE: // Backspace key
                    // Backspace key has own icon, width and CSS-style
                    btnKey = gtk_button_new_from_icon_name ("defigo-key-delete", GTK_ICON_SIZE_INVALID);
                    wd = 106;
                    keyClass = "keypad_key_del";
                    break;

                default: // Regular key
                    // Reset UTF8 buffer
                    memset (singleChar, 0, KEY_BUF_SIZE);
                    // Convert unicode to UTF8
                    (void)g_unichar_to_utf8 (key, singleChar);
                    // Create regular text button with regular size
                    btnKey = gtk_button_new_with_label (singleChar);
                    wd = keyWidth;
                    break;
            }

            // Set key button size
            gtk_widget_set_size_request (btnKey, wd, keyHeight);
            // Add CSS-style
            styleContext = gtk_widget_get_style_context (btnKey);
            gtk_style_context_add_class (styleContext, keyClass);
            // Remove Gtk button border
            gtk_button_set_relief (GTK_BUTTON (btnKey), GTK_RELIEF_NONE);
            gtk_widget_set_visible (btnKey, TRUE);
            // Connect click signal
            g_signal_connect (btnKey, "clicked", G_CALLBACK (on_keypad_button_clicked), userData);
            // Put key in keyboard layout
            // gtk_fixed_put (GTK_FIXED (keypadFixed), btnKey, x, y);
            gtk_box_pack_start (GTK_BOX (boxRow), btnKey, FALSE, FALSE, 0);


            // Calculate next key left offset
            // x += wd + keyHSpace;
            // Get next UTF8 char from keys row string
            next = g_utf8_find_next_char (next, end);
            if (next) { // Has next key
                // Convert first UTF8 char to unicode
                key = g_utf8_get_char (next);
            } else { // No more keys
                key = 0;
            }

        }

        gtk_box_pack_start (GTK_BOX (boxKeypad), boxRow, FALSE, FALSE, 0);

        // Calculate next row top offset
        // y += keyHeight + keyVSpace;
        // Increment row pointer
        row++;
    }

    gtk_widget_set_visible (keypad, FALSE);
}

void keypad_test () {
    //
}

void keypad_hide_revealer () {
    //
}

void keypad_show_revealer () {
    //
}