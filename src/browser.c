#include "browser.h"
#include <libsoup/soup.h>
#include <gtk/gtk.h>

static void on_navigate(GtkButton *button, GtkWidget *entry) {
    GtkTextView *text_view = GTK_TEXT_VIEW(g_object_get_data(G_OBJECT(button), "text_view"));
    const char *url = gtk_entry_get_text(GTK_ENTRY(entry));

    // Create a libsoup session
    SoupSession *session = soup_session_new();
    SoupMessage *msg = soup_message_new("GET", url);

    if (!msg) {
        GtkTextBuffer *buffer = gtk_text_view_get_buffer(text_view);
        gtk_text_buffer_set_text(buffer, "Invalid URL.", -1);
        return;
    }

    // Send the request
    soup_session_send_message(session, msg);

    // Check if the request was successful
    if (SOUP_STATUS_IS_SUCCESSFUL(msg->status_code)) {
        const char *response = msg->response_body->data;
        GtkTextBuffer *buffer = gtk_text_view_get_buffer(text_view);
        gtk_text_buffer_set_text(buffer, response, -1);
    } else {
        GtkTextBuffer *buffer = gtk_text_view_get_buffer(text_view);
        gtk_text_buffer_set_text(buffer, "Failed to load page.", -1);
    }

    g_object_unref(msg);
    g_object_unref(session);
}

GtkWidget* create_browser_window() {
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "EasySurfer");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    // URL Entry and Navigate Button
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *entry = gtk_entry_new();
    GtkWidget *button = gtk_button_new_with_label("Go");
    gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    // Text View for Rendering Content
    GtkWidget *text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_box_pack_start(GTK_BOX(vbox), text_view, TRUE, TRUE, 0);

    // Store the text_view in the button's data
    g_object_set_data(G_OBJECT(button), "text_view", text_view);

    // Connect the "Go" button to the navigation function
    g_signal_connect(button, "clicked", G_CALLBACK(on_navigate), entry);

    return window;
}
