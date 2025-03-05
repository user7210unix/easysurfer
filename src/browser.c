#include "browser.h"
#include <webkit2/webkit2.h>
#include <gtk/gtk.h>
#include <string.h>
#include <ctype.h>

static WebKitWebView* get_current_web_view(BrowserData *data) {
    gint current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(data->notebook));
    if (current_page == -1) return NULL;
    GtkWidget *scrolled_window = gtk_notebook_get_nth_page(GTK_NOTEBOOK(data->notebook), current_page);
    if (!scrolled_window) return NULL;
    GtkWidget *child = gtk_bin_get_child(GTK_BIN(scrolled_window));
    if (!WEBKIT_IS_WEB_VIEW(child)) {
        GList *children = gtk_container_get_children(GTK_CONTAINER(child));
        if (children) {
            child = children->data;
            g_list_free(children);
        }
    }
    return WEBKIT_IS_WEB_VIEW(child) ? WEBKIT_WEB_VIEW(child) : NULL;
}


static void apply_css(BrowserData *data) {
    GtkCssProvider *provider = gtk_css_provider_new();
    const char *css = data->dark_mode ?
        "box { background: linear-gradient(to bottom, #1e1e1e, #252525); padding: 8px; backdrop-filter: blur(10px); }"
        "notebook header { background: rgba(30, 30, 30, 0.95); border-bottom: 0px solid rgba(255, 255, 255, 0.08); padding: 0; }"
        "entry { border: none; border-radius: 0px; padding: 8px 12px; background: rgba(40, 40, 40, 0.9); color: #e0e0e0; box-shadow: inset 0 1px 2px rgba(0, 0, 0, 0.2); }"
        "button { background: rgba(40, 40, 40, 0.9); border: none; border-radius: 0px; padding: 6px 8px; color: #e0e0e0; transition: background 0.2s, color 0.2s; box-shadow: none; }"
        "button:hover { background: #0078d4; color: #ffffff; }"
        "notebook tab { background: transparent; color: #a0a0a0; padding: 8px 16px; margin: 4px; border-radius: 0px; border: none; transition: all 0.2s; font-size: 14px; }"
        "notebook tab:checked { background: linear-gradient(to bottom, #2a2a2a, #252525); color: #ffffff; box-shadow: 0 1px 3px rgba(0, 0, 0, 0.3); font-size: 14px; }" :
        "box { background: rgba(240, 242, 245, 0.9); padding: 8px; backdrop-filter: blur(10px); }"
        "notebook header { background: rgba(245, 245, 245, 0.95); border-bottom: 1px solid rgba(0, 0, 0, 0.1); padding: 0; }"
        "entry { border: none; border-radius: 0px; padding: 8px 12px; background: rgba(255, 255, 255, 0.9); color: #333; }"
        "button { background: rgba(229, 231, 235, 0.9); border: none; border-radius: 8px; padding: 6px 8px; color: #333; transition: background 0.2s, color 0.2s; box-shadow: none; }"
        "button:hover { background: #0078d4; color: #fff; }"
        "notebook tab { background: transparent; color: #666; padding: 8px 16px; margin: 4px; border-radius: 0px; border: none; transition: background 0.2s; font-size: 12px; }"
        "notebook tab:checked { background: rgba(255, 255, 255, 0.95); color: #000; box-shadow: 0 1px 2px rgba(0, 0, 0, 0.1); font-size: 12px; }";
    gtk_css_provider_load_from_data(provider, css, -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
                                              GTK_STYLE_PROVIDER(provider),
                                              GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
}

static GtkWidget* create_icon_button(const char *icon_name) {
    GtkWidget *button = gtk_button_new();
    GtkWidget *image = gtk_image_new_from_icon_name(icon_name, GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_button_set_image(GTK_BUTTON(button), image);
    return button;
}

static gboolean is_url(const char *input) {
    if (!input) return FALSE;
    return (strchr(input, '.') != NULL && strchr(input, ' ') == NULL);
}

static void on_navigate(GtkWidget *widget, gpointer user_data) {
    BrowserData *data = (BrowserData *)user_data;
    const char *input = gtk_entry_get_text(GTK_ENTRY(data->entry));
    char *url;
    if (!input || strlen(input) == 0) {
        url = g_strdup(data->default_homepage);
    } else if (g_str_has_prefix(input, "http://") || g_str_has_prefix(input, "https://") || is_url(input)) {
        if (g_str_has_prefix(input, "http://") || g_str_has_prefix(input, "https://")) {
            url = g_strdup(input);
        } else {
            url = g_strdup_printf("https://%s", input);
        }
    } else {
        url = g_strdup_printf("%s/search?q=%s", data->default_search_engine, input);
    }
    WebKitWebView *web_view = get_current_web_view(data);
    if (web_view) {
        webkit_web_view_load_uri(web_view, url);
        gtk_entry_set_text(GTK_ENTRY(data->entry), url);
        data->history = g_list_prepend(data->history, g_strdup(url));
        g_print("Navigated to: %s\n", url);
    }
    g_free(url);
}

static void on_reload(GtkWidget *widget, gpointer user_data) {
    BrowserData *data = (BrowserData *)user_data;
    WebKitWebView *web_view = get_current_web_view(data);
    if (web_view) {
        webkit_web_view_reload(web_view);
        g_print("Reload triggered\n");
    }
}

static void on_back(GtkWidget *widget, gpointer user_data) {
    BrowserData *data = (BrowserData *)user_data;
    WebKitWebView *web_view = get_current_web_view(data);
    if (web_view && webkit_web_view_can_go_back(web_view)) {
        webkit_web_view_go_back(web_view);
        g_print("Back triggered\n");
    }
}

static void on_forward(GtkWidget *widget, gpointer user_data) {
    BrowserData *data = (BrowserData *)user_data;
    WebKitWebView *web_view = get_current_web_view(data);
    if (web_view && webkit_web_view_can_go_forward(web_view)) {
        webkit_web_view_go_forward(web_view);
        g_print("Forward triggered\n");
    }
}

static void on_close_tab(GtkWidget *widget, gpointer user_data) {
    BrowserData *data = (BrowserData *)user_data;
    gint current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(data->notebook));
    if (current_page != -1) {
        gtk_notebook_remove_page(GTK_NOTEBOOK(data->notebook), current_page);
        g_print("Closed tab %d\n", current_page);
    }
}

static void on_load_changed(WebKitWebView *web_view, WebKitLoadEvent load_event, gpointer user_data) {
    GtkWidget *tab_box = (GtkWidget *)user_data;

    if (load_event == WEBKIT_LOAD_FINISHED) {
        // Get the label (first child of tab_box)
        GList *children = gtk_container_get_children(GTK_CONTAINER(tab_box));
        GtkWidget *label = NULL;
        if (children) {
            label = GTK_WIDGET(children->data);  // First child should be the label
            g_list_free(children);
        }

        if (GTK_IS_LABEL(label)) {
            const char *title = webkit_web_view_get_title(web_view);
            gtk_label_set_text(GTK_LABEL(label), title ? title : "New Tab");
        } else {
            g_print("Error: First child of tab_box is not a label\n");
        }

        // Remove existing favicon if present (to avoid duplicates)
        children = gtk_container_get_children(GTK_CONTAINER(tab_box));
        GList *iter = children;
        while (iter) {
            GtkWidget *child = GTK_WIDGET(iter->data);
            if (GTK_IS_IMAGE(child)) {
                gtk_container_remove(GTK_CONTAINER(tab_box), child);
                break;
            }
            iter = iter->next;
        }
        g_list_free(children);

        // Add favicon
        GdkPixbuf *favicon = webkit_web_view_get_favicon(web_view);
        if (favicon) {
            GdkPixbuf *scaled_favicon = gdk_pixbuf_scale_simple(favicon, 16, 16, GDK_INTERP_BILINEAR);
            GtkWidget *image = gtk_image_new_from_pixbuf(scaled_favicon);
            gtk_box_pack_start(GTK_BOX(tab_box), image, FALSE, FALSE, 4);
            gtk_box_reorder_child(GTK_BOX(tab_box), image, 0);  // Place favicon first
            g_object_unref(scaled_favicon);
        }

        gtk_widget_show_all(tab_box);
    }
}

static void on_bookmark_activate(GtkMenuItem *item, BrowserData *data) {
    const char *url = g_object_get_data(G_OBJECT(item), "url");
    gtk_entry_set_text(GTK_ENTRY(data->entry), url);
    on_navigate(NULL, data);
}

static WebKitWebView* create_web_view(BrowserData *data) {
    WebKitWebContext *context = data->private_mode ? 
        webkit_web_context_new_ephemeral() : webkit_web_context_get_default();
    WebKitWebView *web_view = WEBKIT_WEB_VIEW(webkit_web_view_new_with_context(context));
    WebKitSettings *settings = webkit_web_view_get_settings(web_view);
    webkit_settings_set_enable_javascript(settings, TRUE);
    webkit_settings_set_enable_html5_local_storage(settings, TRUE);
    webkit_settings_set_enable_html5_database(settings, TRUE);
    webkit_settings_set_enable_media(settings, TRUE);
    webkit_settings_set_enable_mediasource(settings, TRUE);
    webkit_settings_set_enable_webgl(settings, TRUE);
    webkit_settings_set_media_playback_requires_user_gesture(settings, FALSE);
    webkit_web_view_set_zoom_level(web_view, data->zoom_level / 100.0);

    WebKitUserContentManager *manager = webkit_web_view_get_user_content_manager(web_view);
    const char *ublock_css = "div[class*='ad'], iframe[src*='ads'], script[src*='ads'], "
                             "[id*='advert'], [class*='banner'], [id*='sponsor'] { display: none !important; }";
    WebKitUserStyleSheet *stylesheet = webkit_user_style_sheet_new(ublock_css,
                                                                  WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
                                                                  WEBKIT_USER_STYLE_LEVEL_USER,
                                                                  NULL, NULL);
    webkit_user_content_manager_add_style_sheet(manager, stylesheet);
    return web_view;
}

static void create_new_tab(BrowserData *data) {
    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    WebKitWebView *web_view = create_web_view(data);
    gtk_container_add(GTK_CONTAINER(scrolled_window), GTK_WIDGET(web_view));

    GtkWidget *label = gtk_label_new("New Tab");
    gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
    gtk_widget_set_size_request(label, 120, -1);

    GtkWidget *close_button = create_icon_button("window-close-symbolic");
    gtk_widget_set_margin_start(close_button, 4);

    GtkWidget *tab_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_box_pack_start(GTK_BOX(tab_box), label, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(tab_box), close_button, FALSE, FALSE, 0);

    int page_num = gtk_notebook_append_page(GTK_NOTEBOOK(data->notebook), scrolled_window, tab_box);
    if (page_num == -1) {
        g_print("Failed to create new tab\n");
        return;
    }

    gtk_notebook_set_tab_reorderable(GTK_NOTEBOOK(data->notebook), scrolled_window, TRUE);
    gtk_notebook_set_current_page(GTK_NOTEBOOK(data->notebook), page_num);

    g_signal_connect(web_view, "load-changed", G_CALLBACK(on_load_changed), tab_box);
    g_signal_connect(close_button, "clicked", G_CALLBACK(on_close_tab), data);

    webkit_web_view_load_uri(web_view, data->default_homepage);
    g_print("New tab created: %d\n", page_num);
}

static void on_new_tab(GtkWidget *widget, gpointer user_data) {
    BrowserData *data = (BrowserData *)user_data;
    create_new_tab(data);
}

static void on_history_activate(GtkMenuItem *item, BrowserData *data) {
    const char *url = g_object_get_data(G_OBJECT(item), "url");
    gtk_entry_set_text(GTK_ENTRY(data->entry), url);
    on_navigate(NULL, data);
}

static void populate_bookmark_menu(BrowserData *data, GtkWidget *menu) {
    gtk_container_foreach(GTK_CONTAINER(menu), (GtkCallback)gtk_widget_destroy, NULL);
    GtkWidget *item1 = gtk_menu_item_new_with_label("Google");
    g_object_set_data(G_OBJECT(item1), "url", (gpointer)"https://www.google.com");
    GtkWidget *item2 = gtk_menu_item_new_with_label("YouTube");
    g_object_set_data(G_OBJECT(item2), "url", (gpointer)"https://www.youtube.com");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item1);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item2);
    g_signal_connect(item1, "activate", G_CALLBACK(on_bookmark_activate), data);
    g_signal_connect(item2, "activate", G_CALLBACK(on_bookmark_activate), data);
}

static void on_settings_changed(GtkWidget *widget, BrowserData *data) {
    if (GTK_IS_COMBO_BOX(widget) && strcmp(gtk_widget_get_name(widget), "zoom") == 0) {
        gint active = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
        switch (active) {
            case 0: data->zoom_level = 50; break;
            case 1: data->zoom_level = 75; break;
            case 2: data->zoom_level = 100; break;
            case 3: data->zoom_level = 125; break;
            case 4: data->zoom_level = 150; break;
            default: data->zoom_level = 100; break;
        }
        WebKitWebView *web_view = get_current_web_view(data);
        if (web_view) webkit_web_view_set_zoom_level(web_view, data->zoom_level / 100.0);
    } else if (GTK_IS_COMBO_BOX(widget)) {
        gint active = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
        g_free(data->default_search_engine);
        switch (active) {
            case 0: data->default_search_engine = g_strdup("https://www.google.com"); break;
            case 1: data->default_search_engine = g_strdup("https://duckduckgo.com"); break;
            case 2: data->default_search_engine = g_strdup("https://www.startpage.com"); break;
            case 3: data->default_search_engine = g_strdup("https://searxng.example.com"); break;
            default: data->default_search_engine = g_strdup("https://www.google.com"); break;
        }
    } else if (GTK_IS_ENTRY(widget)) {
        const char *text = gtk_entry_get_text(GTK_ENTRY(widget));
        g_free(data->default_homepage);
        data->default_homepage = g_strdup(text && strlen(text) > 0 ? text : "https://www.google.com");
    } else if (GTK_IS_CHECK_BUTTON(widget) && strcmp(gtk_widget_get_name(widget), "dark") == 0) {
        data->dark_mode = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
        apply_css(data);
    } else if (GTK_IS_CHECK_BUTTON(widget)) {
        data->private_mode = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
    }
}

static void on_settings_clicked(GtkWidget *widget, gpointer user_data) {
    BrowserData *data = (BrowserData *)user_data;
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Settings", GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(data->settings_button))),
                                                    GTK_DIALOG_MODAL, "Close", GTK_RESPONSE_CLOSE, NULL);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 600, 400);
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_container_add(GTK_CONTAINER(content_area), vbox);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 15);

    GtkWidget *general_label = gtk_label_new("<b>General</b>");
    gtk_label_set_use_markup(GTK_LABEL(general_label), TRUE);
    gtk_box_pack_start(GTK_BOX(vbox), general_label, FALSE, FALSE, 0);

    GtkWidget *search_label = gtk_label_new("Default Search Engine:");
    gtk_box_pack_start(GTK_BOX(vbox), search_label, FALSE, FALSE, 0);
    GtkWidget *search_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(search_combo), "Google");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(search_combo), "DuckDuckGo");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(search_combo), "Startpage");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(search_combo), "SearxNG");
    gtk_combo_box_set_active(GTK_COMBO_BOX(search_combo), 0);
    gtk_box_pack_start(GTK_BOX(vbox), search_combo, FALSE, FALSE, 0);

    GtkWidget *home_label = gtk_label_new("Default Homepage:");
    gtk_box_pack_start(GTK_BOX(vbox), home_label, FALSE, FALSE, 0);
    GtkWidget *home_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(home_entry), data->default_homepage);
    gtk_box_pack_start(GTK_BOX(vbox), home_entry, FALSE, FALSE, 0);

    GtkWidget *appearance_label = gtk_label_new("<b>Appearance</b>");
    gtk_label_set_use_markup(GTK_LABEL(appearance_label), TRUE);
    gtk_box_pack_start(GTK_BOX(vbox), appearance_label, FALSE, FALSE, 0);

    GtkWidget *dark_check = gtk_check_button_new_with_label("Enable Dark Mode");
    gtk_widget_set_name(dark_check, "dark");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dark_check), data->dark_mode);
    gtk_box_pack_start(GTK_BOX(vbox), dark_check, FALSE, FALSE, 0);

    GtkWidget *zoom_label = gtk_label_new("Zoom Level:");
    gtk_box_pack_start(GTK_BOX(vbox), zoom_label, FALSE, FALSE, 0);
    GtkWidget *zoom_combo = gtk_combo_box_text_new();
    gtk_widget_set_name(zoom_combo, "zoom");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(zoom_combo), "50%");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(zoom_combo), "75%");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(zoom_combo), "100%");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(zoom_combo), "125%");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(zoom_combo), "150%");
    gtk_combo_box_set_active(GTK_COMBO_BOX(zoom_combo), 2);
    gtk_box_pack_start(GTK_BOX(vbox), zoom_combo, FALSE, FALSE, 0);

    GtkWidget *privacy_label = gtk_label_new("<b>Privacy & Security</b>");
    gtk_label_set_use_markup(GTK_LABEL(privacy_label), TRUE);
    gtk_box_pack_start(GTK_BOX(vbox), privacy_label, FALSE, FALSE, 0);

    GtkWidget *private_check = gtk_check_button_new_with_label("Private Browsing Mode");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(private_check), data->private_mode);
    gtk_box_pack_start(GTK_BOX(vbox), private_check, FALSE, FALSE, 0);

    g_signal_connect(search_combo, "changed", G_CALLBACK(on_settings_changed), data);
    g_signal_connect(home_entry, "changed", G_CALLBACK(on_settings_changed), data);
    g_signal_connect(dark_check, "toggled", G_CALLBACK(on_settings_changed), data);
    g_signal_connect(zoom_combo, "changed", G_CALLBACK(on_settings_changed), data);
    g_signal_connect(private_check, "toggled", G_CALLBACK(on_settings_changed), data);

    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static void on_bookmark_menu_popup(GtkMenuButton *button, BrowserData *data) {
    GtkMenu *menu = GTK_MENU(gtk_menu_button_get_popup(GTK_MENU_BUTTON(data->bookmark_button)));
    populate_bookmark_menu(data, GTK_WIDGET(menu));
    gtk_widget_show_all(GTK_WIDGET(menu));
}

static void on_history_menu_popup(GtkWidget *widget, BrowserData *data) {
    GtkWidget *menu = gtk_menu_new();
    for (GList *l = data->history; l != NULL; l = l->next) {
        const char *url = (const char *)l->data;
        GtkWidget *item = gtk_menu_item_new_with_label(url);
        g_object_set_data(G_OBJECT(item), "url", (gpointer)url);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        g_signal_connect(item, "activate", G_CALLBACK(on_history_activate), data);
    }
    gtk_menu_popup_at_widget(GTK_MENU(menu), widget, GDK_GRAVITY_SOUTH, GDK_GRAVITY_NORTH, NULL);
}

static void on_window_destroy(GtkWidget *widget, BrowserData *data) {
    g_list_free_full(data->history, g_free);
    g_free(data->default_search_engine);
    g_free(data->default_homepage);
    g_free(data);
}

static gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, BrowserData *data) {
    if (event->state & GDK_CONTROL_MASK) {
        switch (event->keyval) {
            case GDK_KEY_t:
                on_new_tab(NULL, data);
                return TRUE;
            case GDK_KEY_r:
                on_reload(NULL, data);
                return TRUE;
            case GDK_KEY_w:
                on_close_tab(NULL, data);
                return TRUE;
            case GDK_KEY_h:
                on_history_menu_popup(widget, data);
                return TRUE;
        }
    }
    return FALSE;
}

GtkWidget* create_browser_window() {
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "EasySurfer");
    gtk_window_set_default_size(GTK_WINDOW(window), 1024, 768);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    GtkWidget *toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);

    GtkWidget *back_button = create_icon_button("go-previous-symbolic");
    GtkWidget *forward_button = create_icon_button("go-next-symbolic");
    GtkWidget *reload_button = create_icon_button("view-refresh-symbolic");
    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Search or enter address");
    GtkWidget *go_button = create_icon_button("go-jump-symbolic");
    GtkWidget *bookmark_button = create_icon_button("starred-symbolic");
    gtk_button_set_relief(GTK_BUTTON(bookmark_button), GTK_RELIEF_NORMAL);
    GtkWidget *new_tab_button = create_icon_button("tab-new-symbolic");
    GtkWidget *settings_button = create_icon_button("emblem-system-symbolic");

    gtk_box_pack_start(GTK_BOX(toolbar), back_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), forward_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), reload_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), entry, TRUE, TRUE, 4);
    gtk_box_pack_start(GTK_BOX(toolbar), go_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), bookmark_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), new_tab_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), settings_button, FALSE, FALSE, 0);

    GtkWidget *notebook = gtk_notebook_new();
    gtk_notebook_set_show_border(GTK_NOTEBOOK(notebook), FALSE);
    gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);

    BrowserData *data = g_new(BrowserData, 1);
    data->notebook = notebook;
    data->entry = entry;
    data->back_button = back_button;
    data->forward_button = forward_button;
    data->reload_button = reload_button;
    data->go_button = go_button;
    data->bookmark_button = bookmark_button;
    data->new_tab_button = new_tab_button;
    data->settings_button = settings_button;
    data->default_search_engine = g_strdup("https://www.google.com");
    data->default_homepage = g_strdup("https://www.google.com");
    data->dark_mode = FALSE;
    data->private_mode = FALSE;
    data->zoom_level = 100;
    data->history = NULL;

    apply_css(data);

    GtkWidget *bookmark_menu = gtk_menu_new();
    gtk_menu_button_set_popup(GTK_MENU_BUTTON(bookmark_button), bookmark_menu);
    populate_bookmark_menu(data, bookmark_menu);

    create_new_tab(data);

    g_signal_connect(G_OBJECT(data->go_button), "clicked", G_CALLBACK(on_navigate), data);
    g_signal_connect(G_OBJECT(data->reload_button), "clicked", G_CALLBACK(on_reload), data);
    g_signal_connect(G_OBJECT(data->back_button), "clicked", G_CALLBACK(on_back), data);
    g_signal_connect(G_OBJECT(data->forward_button), "clicked", G_CALLBACK(on_forward), data);
    g_signal_connect(G_OBJECT(data->new_tab_button), "clicked", G_CALLBACK(on_new_tab), data);
    g_signal_connect(G_OBJECT(data->settings_button), "clicked", G_CALLBACK(on_settings_clicked), data);
    g_signal_connect_swapped(G_OBJECT(data->bookmark_button), "clicked", G_CALLBACK(on_bookmark_menu_popup), data);
    g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(on_window_destroy), data);
    g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(G_OBJECT(data->entry), "activate", G_CALLBACK(on_navigate), data);
    g_signal_connect(G_OBJECT(window), "key-press-event", G_CALLBACK(on_key_press), data);

    gtk_widget_show_all(window);
    return window;
}
