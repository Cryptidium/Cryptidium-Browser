#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

int main(int argc, char* argv[]) {
    gtk_init(&argc, &argv);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Cryptidium");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);

    GtkWidget *webview = webkit_web_view_new();
    gtk_container_add(GTK_CONTAINER(window), webview);

    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    const char *url = (argc > 1) ? argv[1] : "https://example.com";
    webkit_web_view_load_uri(WEBKIT_WEB_VIEW(webview), url);

    gtk_widget_show_all(window);
    gtk_main();
    return 0;
}
