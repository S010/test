/*
 * Copyright (c) 2011 Sviatoslav Chagaev <sviatoslav.chagaev@gmail.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef UI_H
#define UI_H

#ifdef __cplusplus
extern "C" {
#endif

struct ui_widget;
typedef struct ui_widget * ui_widget_t;

enum ui_widget_types {
    UI_BUTTON = 1,
    UI_HBOX,
    UI_LABEL,
    UI_TEXTBOX,
    UI_VBOX,
    UI_WINDOW,

    UI_WIDGET_TYPES_END
};

enum ui_properties {
    UI_DEBUG_FILENAME = UI_WIDGET_TYPES_END,
    UI_DISABLED,
    UI_FLEX,
    UI_HEIGHT,
    UI_HIDDEN,
    UI_MULTILINE,
    UI_TEXT,
    UI_WIDTH,
    UI_X,
    UI_Y,

    UI_PROPERTIES_END
};

enum ui_event_types {
    UI_ON_CLICK = UI_PROPERTIES_END,
    UI_ON_CLOSE,
    UI_ON_MOUSEDOWN,
    UI_ON_MOUSEMOVE,
    UI_ON_MOUSEUP,

    UI_EVENT_TYPES_END
};

typedef union ui_event * ui_event_t;
union ui_event {
    int type;
    struct {
        int type;
        int x;
        int y;
        int button;
    } mousedown;
    struct {
        int type;
        int x;
        int y;
        int button;
    } mouseup;
    struct {
        int type;
        int x;
        int y;
    } mousemove;
};
typedef void (*ui_event_handler_t)(ui_widget_t, ui_event_t, void *);

int          ui_init(void);
void         ui_main(void);
void         ui_quit(void);
const char  *ui_get_error(void);

ui_widget_t  ui_create(ui_widget_t parent, int widget_type);
int          ui_destroy(ui_widget_t widget);

int          ui_set(ui_widget_t widget, enum ui_properties property, ...);
int          ui_get(ui_widget_t widget, enum ui_properties property, ...);

#ifdef __cplusplus
};
#endif

#endif
