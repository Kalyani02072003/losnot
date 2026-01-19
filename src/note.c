#include <gtk/gtk.h>
#include <time.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "note.h"
#include <glib.h>
#include <glib/gstdio.h>

#define NOTES_DIR "./notes"
#define CONFIG_DIR g_build_filename(g_get_user_config_dir(), "losnot", NULL)
#define CONFIG_FILE g_build_filename(g_get_user_config_dir(), "losnot", "config.ini", NULL)

static int note_count = 0;
static GKeyFile *config = NULL;


/* config addition*/
static void load_config() {
    config = g_key_file_new();

    g_mkdir_with_parents(CONFIG_DIR, 0700);

    if (g_file_test(CONFIG_FILE, G_FILE_TEST_EXISTS)) {
        g_key_file_load_from_file(config, CONFIG_FILE,
                                  G_KEY_FILE_NONE, NULL);
    }
}

static void save_config() {
    gsize len;
    gchar *data = g_key_file_to_data(config, &len, NULL);
    g_file_set_contents(CONFIG_FILE, data, len, NULL);
    g_free(data);
}

static gboolean on_configure(GtkWidget *window,
                             GdkEvent *event,
                             gpointer data) {
    gint x, y, w, h;

    gtk_window_get_position(GTK_WINDOW(window), &x, &y);
    gtk_window_get_size(GTK_WINDOW(window), &w, &h);

    g_key_file_set_integer(config, "window", "x", x);
    g_key_file_set_integer(config, "window", "y", y);
    g_key_file_set_integer(config, "window", "width", w);
    g_key_file_set_integer(config, "window", "height", h);

    save_config();
    return FALSE;
}


/* autostart function*/

static void enable_autostart() {
    const char *dir = g_build_filename(
        g_get_user_config_dir(), "autostart", NULL);
    g_mkdir_with_parents(dir, 0700);

    const char *path = g_build_filename(
        g_get_user_config_dir(), "autostart", "losnot.desktop", NULL);

    const char *content =
        "[Desktop Entry]\n"
        "Type=Application\n"
        "Name=Losnot\n"
        "Exec=losnot\n"
        "Hidden=false\n"
        "NoDisplay=false\n"
        "X-GNOME-Autostart-enabled=true\n";

    g_file_set_contents(path, content, -1, NULL);

    g_key_file_set_boolean(config, "app", "autostart", TRUE);
    save_config();
}

static void disable_autostart() {
    const char *path = g_build_filename(
        g_get_user_config_dir(), "autostart", "losnot.desktop", NULL);

    g_remove(path);

    g_key_file_set_boolean(config, "app", "autostart", FALSE);
    save_config();
}


/* ================== NOTE CONTEXT ================== */

typedef struct {
    char *filepath;
    GtkWidget *title;
} NoteCtx;


// styles 

static void load_dark_theme() {
    GtkCssProvider *provider = gtk_css_provider_new();

    const char *css =
        "* {"
        "  font-family: JetBrains Mono, Fira Code, monospace;"
        "  font-size: 12px;"
        "}"
        "window {"
        "  background-color: #0f1115;"
        "}"
        "#header {"
        "  background-color: #151820;"
        "  border-bottom: 1px solid #222630;"
        "}"
        "textview {"
        "  background-color: #0f1115;"
        "  color: #d6d6d6;"
        "  padding: 8px;"
        "}"
        "button {"
        "  background: transparent;"
        "  color: #9aa4b2;"
        "  border: none;"
        "  box-shadow: none;"
        "}"
        "button:hover {"
        "  color: #ffffff;"
        "}"
        "label {"
        "  color: #cdd6f4;"
        "}"
        "popover {"
        "  background-color: #1a1d26;"
        "  border-radius: 6px;"
        "  padding: 6px;"
        "}";

    gtk_css_provider_load_from_data(provider, css, -1, NULL);

    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );
}


/* ================== UTILS ================== */

static void ensure_notes_dir() {
    struct stat st = {0};
    if (stat(NOTES_DIR, &st) == -1) {
        mkdir(NOTES_DIR, 0700);
    }
}

static char* generate_filename() {
    char *path = malloc(256);
    time_t t = time(NULL);
    strftime(
        path,
        256,
        NOTES_DIR "/Untitled-%Y-%m-%d-%H%M%S.txt",
        localtime(&t)
    );
    return path;
}

/* ================== DRAG (GESTURE-BASED) ================== */

static void on_drag_begin(GtkGestureDrag *gesture,
                          double x, double y,
                          gpointer window) {
    GdkEvent *event = gtk_get_current_event();
    if (!event)
        return;

    GdkEventButton *bevent = (GdkEventButton *)event;

    gtk_window_begin_move_drag(
        GTK_WINDOW(window),
        bevent->button,
        bevent->x_root,
        bevent->y_root,
        bevent->time
    );

    gdk_event_free(event);
}



/* ================== AUTOSAVE ================== */

static void autosave(GtkTextBuffer *buffer, gpointer data) {
    NoteCtx *ctx = (NoteCtx*)data;

    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    char *text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

    FILE *f = fopen(ctx->filepath, "w");
    if (f) {
        fputs(text, f);
        fclose(f);
    }

    g_free(text);
}

/* ================== ACTIONS ================== */

static void show_settings(GtkWidget *btn, gpointer popover) {
    gtk_popover_popup(GTK_POPOVER(popover));
}

static void new_note(GtkWidget *btn, gpointer data) {
    create_note();
}

static void close_note(GtkWidget *btn, gpointer window) {
    if (note_count <= 1) return;
    note_count--;
    gtk_widget_destroy(GTK_WIDGET(window));
}

static void disable_session(GtkWidget *btn, gpointer window) {
    gtk_widget_destroy(GTK_WIDGET(window));
}

static void disable_permanent(GtkWidget *btn, gpointer window) {
    disable_autostart();
    gtk_widget_destroy(GTK_WIDGET(window));
}


static void open_notes_dir(GtkWidget *btn, gpointer data) {
    system("xdg-open " NOTES_DIR " &");
}

/* ================== RENAME ================== */

static void rename_note(GtkWidget *btn, gpointer data) {
    NoteCtx *ctx = (NoteCtx*)data;

    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Rename Note",
        NULL,
        GTK_DIALOG_MODAL,
        "OK", GTK_RESPONSE_OK,
        "Cancel", GTK_RESPONSE_CANCEL,
        NULL
    );

    GtkWidget *entry = gtk_entry_new();
    gtk_container_add(
        GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
        entry
    );

    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        const char *new_name = gtk_entry_get_text(GTK_ENTRY(entry));
        if (new_name && *new_name) {
            char new_path[256];
            snprintf(new_path, sizeof(new_path),
                     NOTES_DIR "/%s.txt", new_name);

            if (rename(ctx->filepath, new_path) == 0) {
                free(ctx->filepath);
                ctx->filepath = strdup(new_path);
                gtk_label_set_text(GTK_LABEL(ctx->title), new_name);
            }
        }
    }

    gtk_widget_destroy(dialog);
}

/* ================== CREATE NOTE ================== */

void create_note() {
    load_dark_theme();

    note_count++;

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_name(window, "losnot-window");


    load_config();

    if (g_key_file_has_group(config, "window")) {
        gint x = g_key_file_get_integer(config, "window", "x", NULL);
        gint y = g_key_file_get_integer(config, "window", "y", NULL);
        gint w = g_key_file_get_integer(config, "window", "width", NULL);
        gint h = g_key_file_get_integer(config, "window", "height", NULL);

        gtk_window_move(GTK_WINDOW(window), x, y);
        gtk_window_resize(GTK_WINDOW(window), w, h);
    }


    gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
    gtk_window_set_keep_above(GTK_WINDOW(window), TRUE);
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(window), TRUE);
    gtk_window_stick(GTK_WINDOW(window));
    gtk_window_set_default_size(GTK_WINDOW(window), 320, 220);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    /* ---------- HEADER ---------- */

    GtkWidget *header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_widget_set_name(header, "header");

    gtk_widget_set_size_request(header, -1, 28);

    GtkGesture *drag = gtk_gesture_drag_new(header);
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(drag), 1);
    g_signal_connect(drag, "drag-begin",
                     G_CALLBACK(on_drag_begin), window);

    GtkWidget *btn_add   = gtk_button_new_with_label("+");
    GtkWidget *btn_more  = gtk_button_new_with_label("⋮");
    GtkWidget *btn_close = gtk_button_new_with_label("x");
    GtkWidget *title     = gtk_label_new("Untitled");

    gtk_box_pack_start(GTK_BOX(header), btn_add, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(header), title, TRUE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(header), btn_close, FALSE, FALSE, 4);
    gtk_box_pack_end(GTK_BOX(header), btn_more, FALSE, FALSE, 4);

    /* ---------- MENU ---------- */

    GtkWidget *popover = gtk_popover_new(btn_more);
    gtk_popover_set_relative_to(GTK_POPOVER(popover), btn_more);
    gtk_popover_set_position(GTK_POPOVER(popover), GTK_POS_BOTTOM);

    GtkWidget *menu = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);

    GtkWidget *btn_sess   = gtk_button_new_with_label("Disable for session");
    GtkWidget *btn_perm   = gtk_button_new_with_label("Disable permanently");
    GtkWidget *btn_open   = gtk_button_new_with_label("Open notes directory");
    GtkWidget *btn_rename = gtk_button_new_with_label("Rename note");
    GtkWidget *btn_autostart = gtk_button_new_with_label("Enable autostart");

    gtk_box_pack_start(GTK_BOX(menu), btn_sess,   FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(menu), btn_perm,   FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(menu), btn_open,   FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(menu), btn_rename, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(menu), btn_autostart, FALSE, FALSE, 0);


    gtk_container_add(GTK_CONTAINER(popover), menu);
    gtk_widget_show_all(menu);

    /* ---------- TEXT ---------- */

    GtkWidget *text = gtk_text_view_new();
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(text), TRUE);
    gtk_text_view_set_pixels_above_lines(GTK_TEXT_VIEW(text), 2);
    gtk_text_view_set_pixels_below_lines(GTK_TEXT_VIEW(text), 2);

    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text), GTK_WRAP_WORD_CHAR);
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text));

    ensure_notes_dir();

    NoteCtx *ctx = malloc(sizeof(NoteCtx));
    ctx->filepath = generate_filename();
    ctx->title = title;

    /* ---------- SIGNALS ---------- */

    g_signal_connect(buffer, "changed", G_CALLBACK(autosave), ctx);
    g_signal_connect(btn_add,   "clicked", G_CALLBACK(new_note), NULL);
    g_signal_connect(btn_close, "clicked", G_CALLBACK(close_note), window);
    g_signal_connect(btn_more,  "clicked", G_CALLBACK(show_settings), popover);

    g_signal_connect(btn_sess,  "clicked", G_CALLBACK(disable_session), window);
    g_signal_connect(btn_perm,  "clicked", G_CALLBACK(disable_permanent), window);
    g_signal_connect(btn_open,  "clicked", G_CALLBACK(open_notes_dir), NULL);
    g_signal_connect(btn_rename,"clicked", G_CALLBACK(rename_note), ctx);
    g_signal_connect(window, "configure-event", G_CALLBACK(on_configure), NULL);
    g_signal_connect(btn_autostart, "clicked",G_CALLBACK(enable_autostart), NULL);

    
    /* ---------- LAYOUT ---------- */

    gtk_box_pack_start(GTK_BOX(vbox), header, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), text, TRUE, TRUE, 0);

    gtk_container_add(GTK_CONTAINER(window), vbox);
    gtk_widget_show_all(window);
}
