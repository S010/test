/* encoding: utf8 */

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "ui.h"

void
write_log(const char *fmt, ...)
{
    static FILE *fp;
    va_list ap;

    if (fp == NULL)
        if ((fp = fopen("log.txt", "a")) == NULL)
            return;
        else
            fprintf(fp, "------- log start -------\n");

    va_start(ap, fmt);
    vfprintf(fp, fmt, ap);
    fflush(fp);
    va_end(ap);
}

void
on_window_close(ui_widget_t w, ui_event_t event, void *unused)
{
    ui_quit();
}

void
on_exit_click(ui_widget_t w, ui_event_t event, void *unused)
{
    ui_quit();
}

void
on_toggle_disabled_click(ui_widget_t w, ui_event_t event, void *data)
{
    ui_widget_t  txt = data;
    int          value;

    ui_get(txt, UI_DISABLED, &value);
    ui_set(txt, UI_DISABLED, !value);
}

int
main(int argc, char **argv)
{
    ui_widget_t window, but1, but2, hbox, txt1, txt2;

    write_log("log start\n");

    if (ui_init() != 0)
        write_log("ui_init() failed\n");

    ui_set(NULL, UI_DEBUG_FILENAME, "uilib_debug.log");

    window = ui_create(NULL, UI_WINDOW);
    but1   = ui_create(window, UI_BUTTON);
    but2   = ui_create(window, UI_BUTTON);
    hbox   = ui_create(window, UI_HBOX);
    txt1   = ui_create(hbox, UI_TEXTBOX);
    txt2   = ui_create(hbox, UI_TEXTBOX);

    ui_set(but1, UI_TEXT, "Exit");
    ui_set(but1, UI_ON_CLICK, on_exit_click, NULL);
    ui_set(but1, UI_FLEX, 1);

    ui_set(but2, UI_TEXT, "Toggle Disabled");
    ui_set(but2, UI_ON_CLICK, on_toggle_disabled_click, txt1);

    ui_set(txt1, UI_FLEX, 1);
    ui_set(txt1, UI_TEXT, "flex=1");

    ui_set(window, UI_ON_CLOSE, on_window_close, NULL);
    ui_set(window, UI_WIDTH, 400);
    ui_set(window, UI_HEIGHT, 300);
    if (ui_set(window, UI_HIDDEN, 0) != 0)
        write_log("ui_set(window, UI_HIDDEN, 0) failed: %s\n", ui_get_error());

    ui_main();

    return 0;
}

