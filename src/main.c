#include <gtk/gtk.h>
#include "app.h"

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    app_start();
    gtk_main();
    return 0;
}
