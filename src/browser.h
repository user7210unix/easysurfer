#ifndef BROWSER_H
#define BROWSER_H

#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

typedef struct {
    GtkWidget *notebook;
    GtkWidget *entry;
    GtkWidget *back_button;
    GtkWidget *forward_button;
    GtkWidget *reload_button;
    GtkWidget *go_button;
    GtkWidget *bookmark_button;
    GtkWidget *new_tab_button;
    GtkWidget *settings_button;
    char *default_search_engine;
    char *default_homepage;
    gboolean dark_mode;
    gboolean private_mode;
    int zoom_level;
    GList *history;
} BrowserData;

GtkWidget* create_browser_window();
void update_tab_title(WebKitWebView *web_view, GtkWidget *label);

#endif
