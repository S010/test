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

#pragma comment(linker, "\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define _UNICODE
#define UNICODE
#define _CRT_SECURE_NO_WARNINGS

#include "ui.h"

#include <windows.h>
#include <Windowsx.h>
#include <Commctrl.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define APP_CLASS_NAME L"ui_app"

#define NELEMS(array) (sizeof(array)/sizeof((array)[0]))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) < (b) ? (b) : (a))

#define DLU_WIDTH_DIVIDER   4
#define DLU_HEIGHT_DIVIDER  8
/* these are in Dialog Template Units */
#define BUTTON_MIN_WIDTH    50
#define BUTTON_MIN_HEIGHT   14
#define LABEL_MIN_WIDTH     1
#define LABEL_MIN_HEIGHT    8
#define TEXTBOX_MIN_WIDTH   ((int) (BUTTON_MIN_WIDTH * 1.5))
#define TEXTBOX_MIN_HEIGHT  14

/* FIXME should be in DLUs */
#define WIDGET_SPACING      6

struct event_handler {
    ui_event_handler_t   callback; /* callback procedure */
    void                *userdata; /* user data */
};

struct ui_widget {
    int                      type;
    HWND                     hwnd;
    struct event_handler     on_mousedown;
    struct event_handler     on_mouseup;
    struct event_handler     on_mousemove;
    struct event_handler     on_close;
    struct event_handler     on_click;
    ui_widget_t              parent;
    ui_widget_t              child;
    ui_widget_t              next;

    int                      flex;


    union {
        struct {
            int              multiline;
        } textbox;
    } u;
};

static ui_widget_t  *widgets;
static int           n_widgets;

static int           quit_uilib = 0;
static const char   *error_msg = NULL;

#define INVALID_WIDGET_TYPE(type)   (type < 1 || type >= UI_WIDGET_TYPES_END)
#define INVALID_PROPERTY(prop)      (prop < UI_WIDGET_TYPES_END || prop >= UI_EVENT_TYPES_END)
#define INVALID_EVENT_TYPE(type)    (type < UI_PROPERTIES_END || type >= UI_EVENT_TYPES_END)

const char *ui_names[] = {
    /* widget types */
    "UI_BUTTON",
    "UI_HBOX",
    "UI_LABEL",
    "UI_TEXTBOX",
    "UI_VBOX",
    "UI_WINDOW",

    /* properties */
    "UI_DEBUG_FILENAME",
    "UI_DISABLED",
    "UI_FLEX",
    "UI_HEIGHT",
    "UI_HIDDEN",
    "UI_MULTILINE",
    "UI_TEXT",
    "UI_WIDTH",
    "UI_X",
    "UI_Y",

    /* properties/event types */
    "UI_ON_CLICK",
    "UI_ON_CLOSE",
    "UI_ON_MOUSEDOWN",
    "UI_ON_MOUSEMOVE",
    "UI_ON_MOUSEUP",
};

#define WIDGET_TYPE_NAME(i) (INVALID_WIDGET_TYPE(i) ? "(unknown widget type)" : ui_names[i-1])
#define PROPERTY_NAME(i)    (INVALID_PROPERTY(i)    ? "(unknown property)"    : ui_names[i-1])
#define EVENT_TYPE_NAME(i)  (INVALID_EVENT_TYPE(i)  ? "(unknown event type)"  : ui_names[i-1])

static HINSTANCE     hInstance;
static HINSTANCE     hPrevInstance;
static LPSTR         szCmdLine;
static int           iCmdShow;
static WNDCLASSEX    wndclass;
static int           appReturnCode;
static int           initialized;
static FILE         *debug_file;
static char         *debug_filename;
static HFONT         hCaptionFont;
static HFONT         hSmallCaptionFont;
static HFONT         hMenuFont;
static HFONT         hStatusFont;
static HFONT         hMessageFont;
static int           iCaptionWidth = BUTTON_MIN_WIDTH;
static int           iCaptionHeight = BUTTON_MIN_HEIGHT;

extern int                main(int, char **);

static void               dbg_printf(const char *, ...);
static void               set_error(const char *, ...);

static int                global_set(int, va_list);
static int                global_get(int, va_list);

static ui_widget_t        widgets_find(HWND hwnd);
static int                widgets_add(ui_widget_t);
static int                widgets_del(ui_widget_t);

static wchar_t           *utf16(const char *);
static char              *utf8(wchar_t *);
static char              *str_dup(const char *s);
                        
static ui_event_t         event_create(int, ...);
                        
static ui_widget_t        window_create(void);
static int                window_set(ui_widget_t, int, va_list);
static int                window_get(ui_widget_t, int, va_list);
static int                window_set_possize(ui_widget_t, int, int, int, int);
static int                window_get_possize(ui_widget_t, int *, int *, int *, int *);
static int                window_set_text(ui_widget_t, const char *);
static int                window_get_text(ui_widget_t, char **);
static int                window_get_required_size(ui_widget_t, int *, int *);

static int                arrange(ui_widget_t);
static int                arrange_vbox(ui_widget_t, int, int, int, int);
static int                arrange_hbox(ui_widget_t, int, int, int, int);

static int                widget_set(ui_widget_t, int, va_list);
static int                widget_get(ui_widget_t, int, va_list);
static int                widget_get_possize(ui_widget_t, int *, int *, int *, int *);
static int                widget_set_possize(ui_widget_t, int, int, int, int);
static int                widget_get_required_size(ui_widget_t, int *, int *);
static void               get_dialog_base_units(HDC, int *, int *);
static int                load_ui_defaults(void);
static int                widget_set_default_font(ui_widget_t w);
static int                load_controls(void);

static ui_widget_t        box_create(ui_widget_t, int);
static int                box_set(ui_widget_t, int, va_list);
static int                box_get(ui_widget_t, int, va_list);
static HWND               get_hwnd(ui_widget_t);
static ui_widget_t        get_parent_window(ui_widget_t);
static int                vbox_get_required_size(ui_widget_t, int *, int *);
static int                hbox_get_required_size(ui_widget_t, int *, int *);

static ui_widget_t        label_create(ui_widget_t);
static int                label_set(ui_widget_t, int, va_list);
static int                label_get(ui_widget_t, int, va_list);

static ui_widget_t        button_create(ui_widget_t);
static int                button_set(ui_widget_t, int, va_list);
static int                button_get(ui_widget_t, int, va_list);

static ui_widget_t        textbox_create(ui_widget_t parent, ui_widget_t w, int);
static int                textbox_set(ui_widget_t, int, va_list);
static int                textbox_get(ui_widget_t, int, va_list);

static LRESULT CALLBACK  WndProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI
WinMain(HINSTANCE hinst, HINSTANCE hprevinst, LPSTR cmdline, int cmdshow)
{
    hInstance = hinst;
    hPrevInstance = hprevinst;
    szCmdLine = cmdline;
    iCmdShow = cmdshow;

    main(0, NULL);

    return appReturnCode;
}

int
ui_init(void)
{
    if (initialized) {
        set_error("already initialized");
        return -1;
    }

    wndclass.cbSize         = sizeof(wndclass);
    wndclass.style          = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc    = WndProc;
    wndclass.cbClsExtra     = 0;
    wndclass.cbWndExtra     = 0;
    wndclass.hInstance      = hInstance;
    wndclass.hIcon          = LoadIcon(NULL, IDI_APPLICATION);
    wndclass.hIconSm        = LoadIcon(NULL, IDI_APPLICATION);
    wndclass.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground  = (HBRUSH) (COLOR_BTNFACE + 1);
    wndclass.lpszClassName  = APP_CLASS_NAME;
    wndclass.lpszMenuName   = NULL;
    if (RegisterClassEx(&wndclass) == 0) {
        set_error("RegisterClassEx failed");
        return -1;
    }

    load_ui_defaults();
    load_controls();

    initialized = 1;

    return 0;
}

void
ui_main(void)
{
    MSG msg;

    dbg_printf("ui_main()\n");

    if (!initialized) {
        set_error("uilib has not been initialized");
        return;
    }

    while (quit_uilib == 0) {
        if (GetMessage(&msg, NULL, 0, 0) > 0) {
            dbg_printf("ui_main(): GetMessage(): msg.message=%lu\n", (unsigned long) msg.message);
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    /*  Exit with status specified in WM_QUIT message  */
    appReturnCode = (int) msg.wParam;
}

void
ui_quit(void)
{
    dbg_printf("ui_quit()\n");

    quit_uilib = 1;
}

const char *
ui_get_error(void)
{
    dbg_printf("ui_get_error(): error_msg=%s\n", error_msg != NULL ? error_msg : "(null)");

    return error_msg;
}

ui_widget_t
ui_create(ui_widget_t parent, int type)
{
    ui_widget_t  w, child;

    w = child = NULL;

    dbg_printf("ui_create(parent=%p, type=%s)\n", parent, WIDGET_TYPE_NAME(type));

    if (INVALID_WIDGET_TYPE(type)) {
        set_error("invalid arguments");
        return NULL;
    }

    if (type != UI_WINDOW && (parent == NULL || (parent->type != UI_WINDOW && parent->type != UI_VBOX && parent->type != UI_HBOX))) {
        set_error("invalid arguments");
        return NULL;
    }

    switch (type) {
    case UI_WINDOW:
        w = window_create();
        break;
    case UI_LABEL:
        w = label_create(parent);
        break;
    case UI_BUTTON:
        w = button_create(parent);
        break;
    case UI_TEXTBOX:
        w = textbox_create(parent, NULL, 0);
        break;
    case UI_HBOX:
    case UI_VBOX:
        w = box_create(parent, type);
        break;
    default:
        set_error("invalid arguments");
        w = NULL;
        break;
    }

    if (w == NULL)
        return NULL;

    if (type != UI_WINDOW) {
        if (parent->child == NULL)
            parent->child = w;
        else {
            for (child = parent->child; child->next != NULL; child = child->next)
                /* empty */;
            child->next = w;
        }
        w->parent = parent;
    }
    widgets_add(w); /* add to global lookup table */

    dbg_printf("  w=%p, hwnd=%p\n", w, (void *) w->hwnd);

    return w;
}

int
ui_destroy(ui_widget_t w)
{
    ui_widget_t  prev, child;

    dbg_printf("ui_destroy(w=%p): w->type=%s\n", w, WIDGET_TYPE_NAME(w->type));

    if (w == NULL || INVALID_WIDGET_TYPE(w->type)) {
        set_error("invalid arguments");
        return -1;
    }

    if (w->next != NULL)
        ui_destroy(w->next);

    if (w->child != NULL)
        ui_destroy(w->child);

    if (w->hwnd != NULL)
        DestroyWindow(w->hwnd);

    if (w->parent != NULL) {
        prev = NULL;
        for (child = w->parent->child; child != w && child != NULL; child = child->next)
            prev = child;
        if (child != NULL) {
            if (prev != NULL)
                prev->next = child->next;
            else
                w->parent->child = child->next;
        }
    }
    widgets_del(w); /* delete from global lookup table */
    free(w);

    return 0;
}

static int
arrange(ui_widget_t window)
{
    int width, height;

    dbg_printf("arrange(window=%p)\n", window);

    assert(window != NULL);
    assert(window->type == UI_WINDOW);

    if (window_get_possize(window, NULL, NULL, &width, &height) == -1)
        return -1;

    dbg_printf("  avail_width=%d, avail_height=%d\n", width, height);

    width -= WIDGET_SPACING * 2;
    if (width < 0)
        width = 0;
    height -= WIDGET_SPACING * 2;
    if (height < 0)
        height = 0;

    return arrange_vbox(window, WIDGET_SPACING, WIDGET_SPACING, width, height);
}

static int
arrange_vbox(ui_widget_t box, int start_x, int start_y, int avail_width, int avail_height)
{
    ui_widget_t  child;
    int          n_children;
    int          x, y;
    int          width, height, req_width, req_height;
    int          sum_flex;
    int          child_height;

    dbg_printf("arrange_vbox(box=%p, start_x=%d, start_y=%d, avail_width=%d, avail_height=%d)\n", box, start_x, start_y, avail_width, avail_height);

    width = avail_width;
    height = avail_height;
    sum_flex = 0;
    n_children = 0;
    for (child = box->child; child != NULL; child = child->next) {
        ++n_children;
        if (child->flex > 0) {
            sum_flex += child->flex;
        } else {
            widget_get_required_size(child, &req_width, &req_height);
            height -= req_height;
        }
    }
    if (n_children > 0)
        height -= WIDGET_SPACING * (n_children - 1);

    x = start_x;
    y = start_y;
    for (child = box->child; child != NULL; child = child->next) {
        widget_get_required_size(child, &req_width, &child_height);
        if (child->flex > 0)
            child_height = (int) ((double) height * ((double) child->flex / (double) sum_flex));

        dbg_printf("  placing child at { %d, %d, %d, %d }\n", x, y, req_width, child_height);

        switch (child->type) {
        case UI_VBOX:
            arrange_vbox(child, x, y, avail_width, child_height);
            break;
        case UI_HBOX:
            arrange_hbox(child, x, y, avail_width, child_height);
            break;
        default:
            widget_set_possize(child, x, y, req_width, child_height);
            break;
        }

        y += child_height;
        y += WIDGET_SPACING;
    }

    return 0;
}

static int
arrange_hbox(ui_widget_t box, int start_x, int start_y, int avail_width, int avail_height)
{
    ui_widget_t  child;
    int          n_children;
    int          x, y;
    int          width, height, req_width, req_height;
    int          sum_flex;
    int          child_width;

    dbg_printf("arrange_hbox(box=%p, start_x=%d, start_y=%d, avail_width=%d, avail_height=%d)\n", box, start_x, start_y, avail_width, avail_height);

    width = avail_width;
    height = avail_height;
    sum_flex = 0;
    n_children = 0;
    for (child = box->child; child != NULL; child = child->next) {
        ++n_children;
        if (child->flex > 0) {
            sum_flex += child->flex;
        } else {
            widget_get_required_size(child, &req_width, &req_height);
            width -= req_width;
        }
    }
    if (n_children > 0)
        width -= WIDGET_SPACING * (n_children - 1);

    x = start_x;
    y = start_y;
    for (child = box->child; child != NULL; child = child->next) {
        widget_get_required_size(child, &child_width, &req_height);
        if (child->flex > 0)
            child_width = (int) ((double) width * ((double) child->flex / (double) sum_flex));

        dbg_printf("  placing child at { %d, %d, %d, %d }\n", x, y, child_width, req_height);

        switch (child->type) {
        case UI_VBOX:
            arrange_vbox(child, x, y, child_width, avail_height);
            break;
        case UI_HBOX:
            arrange_hbox(child, x, y, child_width, avail_height);
            break;
        default:
            widget_set_possize(child, x, y, child_width, req_height);
            break;
        }

        x += child_width;
        x += WIDGET_SPACING;
    }

    return 0;
}

int
ui_set(ui_widget_t w, int prop, ...)
{
    int                  rc;
    va_list              ap;

    dbg_printf("ui_set(w=%p, prop=%s, ...)\n", w, PROPERTY_NAME(prop));

    if (!initialized) {
        set_error("uilib has not been initialized");
        return -1;
    }

    if (INVALID_PROPERTY(prop)) {
        set_error("invalid arguments");
        return -1;
    }

    va_start(ap, prop);
    if (w == NULL) {
        rc = global_set(prop, ap);
    } else {
        switch (w->type) {
        case UI_WINDOW:
            rc = window_set(w, prop, ap);
            break;
        case UI_LABEL:
            rc = label_set(w, prop, ap);
            break;
        case UI_BUTTON:
            rc = button_set(w, prop, ap);
            break;
        case UI_TEXTBOX:
            rc = textbox_set(w, prop, ap);
            break;
        default:
            rc = -1;
            set_error("invalid arguments");
            break;
        }
    }
    va_end(ap);

    return rc;
}

static int
global_set(int prop, va_list ap)
{
    const char  *s_arg;

    dbg_printf("global_set(prop=%s, ...)\n", PROPERTY_NAME(prop));

    switch (prop) {
    case UI_DEBUG_FILENAME:
        s_arg = va_arg(ap, const char *);
        if (debug_file != NULL)
            fclose(debug_file);
        if (debug_filename != NULL) {
            free(debug_filename);
            debug_filename = NULL;
        }
        debug_file = fopen(s_arg, "w");
        if (debug_file == NULL) {
            set_error("%s: failed to open the file", s_arg);
            return -1;
        } else {
            debug_filename = str_dup(s_arg);
            dbg_printf("--------------------------------------------------------------------------------\n");
        }
        break;
    default:
        set_error("%s is not a globally settable property", PROPERTY_NAME(prop));
        return -1;
    }

    return 0;
}

int
ui_get(ui_widget_t w, int prop, ...)
{
    int rc;
    va_list ap;

    dbg_printf("ui_get(w=%p, prop=%s, ...)\n", w, PROPERTY_NAME(prop));

    if (!initialized) {
        set_error("uilib has not been initialized");
        return -1;
    }

    if ((w != NULL && INVALID_WIDGET_TYPE(w->type)) || INVALID_PROPERTY(prop)) {
        set_error("invalid arguments");
        return -1;
    }

    va_start(ap, prop);
    if (w == NULL) {
        rc = global_get(prop, ap);
    } else {
        switch (w->type) {
        case UI_WINDOW:
            rc = window_get(w, prop, ap);
            break;
        case UI_LABEL:
            rc = label_get(w, prop, ap);
            break;
        case UI_BUTTON:
            rc = button_get(w, prop, ap);
            break;
        case UI_TEXTBOX:
            rc = textbox_get(w, prop, ap);
            break;
        default:
            rc = -1;
            set_error("invalid arguments");
            break;
        }
    }
    va_end(ap);

    return rc;
}

int
global_get(int prop, va_list ap)
{
    const char *s;
    const char **sp;

    dbg_printf("global_get(prop=%s, ...)\n", PROPERTY_NAME(prop));

    switch (prop) {
    case UI_DEBUG_FILENAME:
        sp = va_arg(ap, const char **);
        if (sp == NULL) {
            set_error("invalid arguments");
            return -1;
        }
        if (debug_filename == NULL)
            *sp = NULL;
        else {
            s = str_dup(debug_filename);
            if (s == NULL) {
                set_error("memory allocation failed");
                return -1;
            }
            *sp = s;
        }
        break;
    default:
        set_error("%s is not a globally settable property", PROPERTY_NAME(prop));
        return -1;
    }

    return 0;
}

int
widget_get_required_size(ui_widget_t w, int *widthp, int *heightp)
{
    HFONT            hfont, old_hfont = NULL;
    HDC              hdc;
    wchar_t         *wbuf;
    int              len;
    DWORD            ext;
    int              width_unit, height_unit;
    DWORD            padding;

    dbg_printf("widget_get_required_size(w=%p, widthp=%p, heightp=%p)\n", w, widthp, heightp);

    assert(w != NULL && !INVALID_WIDGET_TYPE(w->type) && widthp != NULL && heightp != NULL && "Invalid arguments");

    hfont = (HFONT) SendMessage(w->hwnd, WM_GETFONT, 0, 0);

    switch (w->type) {
    case UI_LABEL:
    case UI_BUTTON:
        hdc = GetDC(w->hwnd);

        if (hfont != NULL)
            old_hfont = SelectObject(hdc, hfont);

        get_dialog_base_units(hdc, &width_unit, &height_unit);

        len = GetWindowTextLength(w->hwnd);

        wbuf = malloc(sizeof(*wbuf) * (len + 1));
        if (wbuf == NULL) {
            if (old_hfont != NULL)
                SelectObject(hdc, old_hfont);
            ReleaseDC(w->hwnd, hdc);
            set_error("memory allocation failed");
            return -1;
        }

        GetWindowText(w->hwnd, wbuf, len + 1);
        wbuf[len] = L'\0';
        if ((ext = GetTabbedTextExtent(hdc, wbuf, len, 0, NULL)) == 0) {
            if (old_hfont != NULL)
                SelectObject(hdc, old_hfont);
            ReleaseDC(w->hwnd, hdc);
            free(wbuf);
            set_error("GetTabbedTextExtent failed");
            return -1;
        }

        padding = GetTabbedTextExtent(hdc, L"M", 1, 0, NULL);
        
        if (old_hfont != NULL)
            SelectObject(hdc, old_hfont);
        free(wbuf);
        ReleaseDC(w->hwnd, hdc);

        *widthp  = MAX(LOWORD(ext) + LOWORD(padding) * 2, MulDiv(BUTTON_MIN_WIDTH,  width_unit,  DLU_WIDTH_DIVIDER ));
        *heightp = MAX(HIWORD(ext), MulDiv(BUTTON_MIN_HEIGHT, height_unit, DLU_HEIGHT_DIVIDER));
        break;
    case UI_TEXTBOX:
        hdc = GetDC(w->hwnd);

        if (hfont != NULL)
            SelectObject(hdc, hfont);

        get_dialog_base_units(hdc, &width_unit, &height_unit);

        ext = GetTabbedTextExtent(hdc, L"|", 1, 0, NULL);

        ReleaseDC(w->hwnd, hdc);

        *widthp = MulDiv(TEXTBOX_MIN_WIDTH, width_unit, DLU_WIDTH_DIVIDER);
        *heightp = MulDiv(TEXTBOX_MIN_HEIGHT, height_unit, DLU_HEIGHT_DIVIDER);
        break;
    case UI_VBOX:
        return vbox_get_required_size(w, widthp, heightp);
        break;
    case UI_HBOX:
        return hbox_get_required_size(w, widthp, heightp);
        break;
    default:
        *widthp = 0;
        *heightp = 0;
        break;
    }

    return 0;
}

static void
get_dialog_base_units(HDC hdc, int *width_unit, int *height_unit)
{
    LONG         dlu;
    TEXTMETRIC   tm;
    SIZE         size;
    const wchar_t alphabet[] = L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    const size_t  alphabet_len = NELEMS(alphabet) - 1;

    assert(hdc != NULL);
    assert(width_unit != NULL);
    assert(height_unit != NULL);

    /* Assign sensible values in case we fail */
    dlu = GetDialogBaseUnits(); /* Computes DLUs based on the ugly, bitmapped System font */
    *width_unit = LOWORD(dlu);
    *height_unit = HIWORD(dlu);

    if (GetTextMetrics(hdc, &tm) == 0) {
        dbg_printf("GetTextMetrics failed\n");
        return;
    }
    if (GetTextExtentPoint32(hdc, alphabet, (int) alphabet_len, &size) == 0) {
        dbg_printf("GetTextExtentPoint32 failed\n");
        return;
    }

    *width_unit  = size.cx / (int) alphabet_len;
    *height_unit = tm.tmHeight;

    return;
}

static void
dbg_printf(const char *fmt, ...)
{
    va_list ap;

    if (debug_file == NULL)
        return;

    va_start(ap, fmt);
    vfprintf(debug_file, fmt, ap);
    fflush(debug_file);
    va_end(ap);
}

static ui_widget_t
widgets_find(HWND hwnd)
{
    ui_widget_t *p;

    dbg_printf("widget_find(hwnd=%p)\n", (void *) hwnd);

    for (p = widgets; p != NULL && p < (widgets + n_widgets); ++p)
        if ((*p)->hwnd == hwnd)
            return *p;

    return NULL;
}


static int
widgets_add(ui_widget_t widget)
{
    ui_widget_t *p;

    dbg_printf("widgets_add(widget=%p)\n", widget);

    p = realloc(widgets, sizeof(ui_widget_t) * (n_widgets + 1));
    if (p == NULL)
        return -1;

    widgets = p;
    ++n_widgets;

    widgets[n_widgets - 1] = widget;

    return 0;
}

static int
widgets_del(ui_widget_t widget)
{
    int          i;
    ui_widget_t *p;
    ui_widget_t  tmp;

    dbg_printf("widgets_del(widget=%p)\n", widget);

    for (i = 0; i < n_widgets; ++i)
        if (widgets[i] == widget)
            goto remove;
    return -1;

 remove:
    tmp = widgets[n_widgets - 1];
    widgets[n_widgets - 1] = widgets[i];
    widgets[i] = tmp;

    p = realloc(widgets, sizeof(*widgets) * --n_widgets);
    if (p != NULL)
        widgets = p;

    return 0;
}

static void
set_error(const char *fmt, ...)
{
    static char      buf[4096];
    const size_t     buf_size = sizeof buf;
    va_list          ap;

    va_start(ap, fmt);
    vsnprintf(buf, buf_size, fmt, ap);
    va_end(ap);

    dbg_printf("set_error: %s\n", buf);

    error_msg = buf;
}

/* convert utf8 string to utf16.
   allocates a buffer -- it is user's responsibility to free() it
   */
static wchar_t *
utf16(const char *s)
{
    size_t   s_size;
    int      n_chars;
    WCHAR   *buf;

    dbg_printf("utf16(s=%s)\n", s == NULL ? "(null)" : s);

    s_size = strlen(s) + 1;

    n_chars = MultiByteToWideChar(CP_UTF8, 0, s, (int) s_size, NULL, 0);
    if (n_chars == 0) {
        set_error("MultiByteToWideChar failed");
        return NULL;
    }

    buf = malloc(sizeof(WCHAR) * n_chars);
    if (buf == NULL) {
        set_error("memory allocation failed");
        return NULL;
    }

    if (MultiByteToWideChar(CP_UTF8, 0, s, (int) s_size, buf, n_chars) == 0) {
        free(buf);
        set_error("MultiByteToWideChar failed");
        return NULL;
    }

    return buf;
}

static char *
utf8(wchar_t *ws)
{
    size_t   ws_size;
    int      n_bytes;
    char    *buf;

    dbg_printf("utf8(ws=%p)\n", ws);

    ws_size = wcslen(ws) + 1;

    n_bytes = WideCharToMultiByte(CP_UTF8, 0, ws, (int) ws_size, NULL, 0, NULL, NULL);
    if (n_bytes == 0) {
        set_error("WideCharToMultiByte failed");
        return NULL;
    }

    buf = malloc(n_bytes);
    if (buf == NULL) {
        set_error("memory allocation failed");
        return NULL;
    }

    if (WideCharToMultiByte(CP_UTF8, 0, ws, (int) ws_size, buf, n_bytes, NULL, NULL) == 0) {
        free(buf);
        set_error("WideCharToMultiByte failed");
        return NULL;
    }

    return buf;
}

static char *
str_dup(const char *s)
{
    char *buf;
    size_t s_size;

    if (s == NULL)
        return NULL;

    s_size = strlen(s) + 1;
    buf = malloc(s_size);
    if (buf == NULL)
        return NULL;
    memcpy(buf, s, s_size);
    return buf;
}

static ui_event_t
event_create(int type, ...)
{
    ui_event_t   event;
    va_list      ap;

    dbg_printf("event_create(type=%s, ...)\n", EVENT_TYPE_NAME(type));

    event = calloc(1, sizeof(*event));

    if (event == NULL)
        return NULL;

    va_start(ap, type);

    event->type = type;
    switch (type) {
    case UI_ON_MOUSEDOWN:
        event->mouseup.button = va_arg(ap, int);
        event->mouseup.x      = va_arg(ap, int);
        event->mouseup.y      = va_arg(ap, int);
        break;
    case UI_ON_MOUSEUP:
        event->mousedown.button = va_arg(ap, int);
        event->mousedown.x      = va_arg(ap, int);
        event->mousedown.y      = va_arg(ap, int);
        break;
    case UI_ON_MOUSEMOVE:
        event->mousemove.x = va_arg(ap, int);
        event->mousemove.y = va_arg(ap, int);
        break;
    case UI_ON_CLICK:
        break;
    default:
        free(event);
        set_error("unknown event type %d", type);
        va_end(ap);
        return NULL;
    }

    va_end(ap);

    return event;
}

static ui_widget_t
window_create(void)
{
    ui_widget_t  w;

    dbg_printf("window_create()\n");

    w = calloc(1, sizeof *w);
    if (w == NULL) {
        set_error("memory allocation failed");
        return NULL;
    }
    w->type = UI_WINDOW;
    w->hwnd = CreateWindowEx(0,
                             APP_CLASS_NAME,
                             APP_CLASS_NAME,
                             WS_OVERLAPPEDWINDOW,
                             CW_USEDEFAULT,
                             CW_USEDEFAULT,
                             CW_USEDEFAULT,
                             CW_USEDEFAULT,
                             NULL,
                             NULL,
                             hInstance,
                             NULL);
    if (w->hwnd == NULL) {
        free(w);
        set_error("CreateWindowEx failed");
        return NULL;
    }

    widget_set_default_font(w);

    return w;
}

static int
window_set(ui_widget_t w, int prop, va_list ap)
{
    int i;
    const char *s;
    int x, y, width, height;

    dbg_printf("window_set(w=%p, prop=%s, ...)\n", w, PROPERTY_NAME(prop));

    if (w == NULL || w->type != UI_WINDOW || INVALID_PROPERTY(prop)) {
        set_error("invalid arguments");
        return -1;
    }

    switch (prop) {
    case UI_X:
    case UI_Y:
    case UI_WIDTH:
    case UI_HEIGHT:
        i = va_arg(ap, int);
        if (window_get_possize(w, &x, &y, &width, &height) == -1)
            return -1;
        switch (prop) {
        case UI_X:
            x = i;
            break;
        case UI_Y:
            y = i;
            break;
        case UI_WIDTH:
            width = i;
            break;
        case UI_HEIGHT:
            height = i;
            break;
        }
        return window_set_possize(w, x, y, width, height);
    case UI_TEXT:
        s = va_arg(ap, const char *);
        return window_set_text(w, s);
    case UI_ON_MOUSEDOWN:
        w->on_mousedown.callback = va_arg(ap, void *);
        w->on_mousedown.userdata = va_arg(ap, void *);
        break;
    case UI_ON_MOUSEUP:
        w->on_mouseup.callback = va_arg(ap, void *);
        w->on_mousedown.userdata = va_arg(ap, void *);
        break;
    case UI_ON_MOUSEMOVE:
        w->on_mousemove.callback = va_arg(ap, void *);
        w->on_mousemove.userdata = va_arg(ap, void *);
        break;
    case UI_ON_CLOSE:
        w->on_close.callback = va_arg(ap, void *);
        w->on_close.userdata = va_arg(ap, void *);
        break;
    case UI_HIDDEN:
        i = va_arg(ap, int);
        if (i == 0) {
            ShowWindow(w->hwnd, SW_SHOW);
            UpdateWindow(w->hwnd);
        } else {
            ShowWindow(w->hwnd, SW_HIDE);
        }
        break;
    default:
        set_error("%s is not supported by %s", PROPERTY_NAME(prop), WIDGET_TYPE_NAME(w->type));
        return -1;
    }

    return 0;
}

static int
window_get(ui_widget_t w, int prop, va_list ap)
{
    int                     *ip;
    char                    **sp;
    void                    **cbp, **udp;
    struct event_handler     handler;

    dbg_printf("window_get(%p, %s, ...)\n", w, PROPERTY_NAME(w->type));

    if (w == NULL || w->type != UI_WINDOW || INVALID_PROPERTY(prop)) {
        set_error("invalid arguments");
        return -1;
    }

    switch (prop) {
    case UI_X:
    case UI_Y:
    case UI_WIDTH:
    case UI_HEIGHT:
        ip = va_arg(ap, int *);
        if (ip == NULL) {
            set_error("invalid arguments");
            return -1;
        }
        switch (prop) {
        case UI_X:
            return window_get_possize(w, ip, NULL, NULL, NULL);
        case UI_Y:
            return window_get_possize(w, NULL, ip, NULL, NULL);
        case UI_WIDTH:
            return window_get_possize(w, NULL, NULL, ip, NULL);
        case UI_HEIGHT:
            return window_get_possize(w, NULL, NULL, NULL, ip);
        }
        break;
    case UI_TEXT:
        sp = va_arg(ap, char **);
        return window_get_text(w, sp);
        break;
    case UI_ON_MOUSEDOWN:
    case UI_ON_MOUSEUP:
    case UI_ON_MOUSEMOVE:
    case UI_ON_CLOSE:
        cbp = va_arg(ap, void *); /* callback */
        udp = va_arg(ap, void *); /* userdata */

        switch (prop) {
        case UI_ON_MOUSEDOWN:
            handler = w->on_mousedown;
            break;
        case UI_ON_MOUSEUP:
            handler = w->on_mouseup;
            break;
        case UI_ON_MOUSEMOVE:
            handler = w->on_mousemove;
            break;
        case UI_ON_CLOSE:
            handler = w->on_close;
            break;
        }

        if (cbp != NULL)
            *cbp = handler.callback;
        if (udp != NULL)
            *udp = handler.userdata;

        break;
    default:
        set_error("%s is not supported by %s", PROPERTY_NAME(prop), WIDGET_TYPE_NAME(w->type));
        return -1;
    }

    return 0;
}

static int
window_set_possize(ui_widget_t w, int x, int y, int width, int height)
{
    RECT     window_rect, client_rect;
    int      window_width, client_width, window_height, client_height;
    int      dx, dy;

    assert(w != NULL && w->type == UI_WINDOW);

    if (GetWindowRect(w->hwnd, &window_rect) == 0) {
        set_error("GetWindowRect failed");
        return -1;
    }
    if (GetClientRect(w->hwnd, &client_rect) == 0) {
        set_error("GetClientRect failed");
        return -1;
    }

    window_width  = window_rect.right - window_rect.left;
    window_height = window_rect.bottom - window_rect.top;
    client_width  = client_rect.right - client_rect.left;
    client_height = client_rect.bottom - client_rect.top;
    
    dx = window_width  - client_width;
    dy = window_height - client_height;

    if (MoveWindow(w->hwnd, x, y, width + dx, height + dy, TRUE) == 0) {
        set_error("MoveWindow failed");
        return -1;
    }

    return 0;
}

static int
window_set_text(ui_widget_t w, const char *s)
{
    wchar_t *ws;
    int      rc;

    dbg_printf("window_set_text(%p, %s)\n", w, s == NULL ? "(null)" : s);

    assert(w != NULL && !INVALID_WIDGET_TYPE(w->type) && s != NULL);

    if ((ws = utf16(s)) == NULL)
        return -1;

    if (SetWindowText(w->hwnd, ws) == 0) {
        set_error("SetWindowText failed");
        rc = -1;
    }

    free(ws);

    return rc;
}

static int
window_get_possize(ui_widget_t w, int *x, int *y, int *width, int *height)
{
    RECT window_rect, client_rect;

    dbg_printf("window_get_possize(w=%p ...)\n", w);

    assert(w != NULL && w->type == UI_WINDOW);

    if (GetWindowRect(w->hwnd, &window_rect) == 0) {
        set_error("GetWindowRect failed");
        return -1;
    }
    if (GetClientRect(w->hwnd, &client_rect) == 0) {
        set_error("GetClientRect failed");
        return -1;
    }

    if (x != NULL)
        *x = window_rect.left;
    if (y != NULL)
        *y = window_rect.top;
    if (width != NULL)
        *width = client_rect.right - client_rect.left;
    if (height != NULL)
        *height = client_rect.bottom - client_rect.top;

    return 0;
}

static int
window_get_text(ui_widget_t w, char **sp)
{
    wchar_t     *ws;
    char        *s;
    int          len;

    dbg_printf("window_get_text(w=%p, sp=%p)\n", w, sp);

    assert(w != NULL && !INVALID_WIDGET_TYPE(w->type) && w->hwnd != NULL && sp != NULL);

    len = GetWindowTextLength(w->hwnd);
    ws = malloc(sizeof(wchar_t) * (len + 1));
    if (ws == NULL) {
        set_error("memory allocation failed");
        return -1;
    }

    GetWindowText(w->hwnd, ws, len + 1);
    ws[len] = L'\0';

    s = utf8(ws);
    if (s == NULL) {
        free(ws);
        return -1;
    }

    *sp = s;

    return 0;
}

static int
widget_get_possize(ui_widget_t w, int *x, int *y, int *width, int *height)
{
    RECT     rect;
    POINT    topleft, bottomright;

    dbg_printf("widget_get_possize(w=%p ...)\n", w);

    assert(w != NULL);
    assert(w->type != UI_WINDOW);
    assert(w->parent != NULL);

    if (GetWindowRect(w->hwnd, &rect) == 0)
        return -1;
    topleft.x = rect.left;
    topleft.y = rect.top;
    bottomright.x = rect.right;
    bottomright.y = rect.bottom;
    if (ScreenToClient(w->parent->hwnd, &topleft) == 0) {
        set_error("ScreenToClient failed");
        return -1;
    }
    if (ScreenToClient(w->parent->hwnd, &bottomright) == 0) {
        set_error("ScreenToClient failed");
        return -1;
    }

    if (x != NULL)
        *x = topleft.x;
    if (y != NULL)
        *y = topleft.y;
    if (width != NULL)
        *width = bottomright.x - topleft.x;
    if (height != NULL)
        *height = bottomright.y - topleft.y;

    return 0;
}

static int
widget_set_possize(ui_widget_t w, int x, int y, int width, int height)
{
    assert(w != NULL);
    assert(w->hwnd != NULL);
    assert(w->type != UI_WINDOW);
    assert(!INVALID_WIDGET_TYPE(w->type));

    if (MoveWindow(w->hwnd, x, y, width, height, TRUE) == 0) {
        set_error("MoveWindow failed");
        return -1;
    }

    return 0;
}

static int
widget_set(ui_widget_t w, int prop, va_list ap)
{
    const char  *s_arg;
    int          i_arg;
    int          x, y;
    int          req_width, req_height, width, height;
    ui_widget_t  window;

    dbg_printf("widget_set(w=%p, prop=%s, ...)\n", w, PROPERTY_NAME(prop));

    assert(w != NULL && !INVALID_PROPERTY(prop));

    switch (prop) {
    case UI_X:
    case UI_Y:
    case UI_WIDTH:
    case UI_HEIGHT:
        widget_get_possize(w, &x, &y, &width, &height);
        i_arg = va_arg(ap, int);
        switch (prop) {
        case UI_X:
            x = i_arg;
            break;
        case UI_Y:
            y = i_arg;
            break;
        case UI_WIDTH:
            width = i_arg;
            break;
        case UI_HEIGHT:
            height = i_arg;
            break;
        }
        if (MoveWindow(w->hwnd, x, y, width, height, TRUE) == 0) {
            set_error("MoveWindow failed");
            return -1;
        }
        if (prop == UI_WIDTH || prop == UI_HEIGHT) {
            return arrange(get_parent_window(w));
        }
        break;
    case UI_TEXT:
        s_arg = va_arg(ap, const char *);
        if (s_arg == NULL) {
            set_error("invalid arguments");
            return -1;
        }
        window_set_text(w, s_arg);
        widget_get_possize(w, &x, &y, &width, &height);
        widget_get_required_size(w, &req_width, &req_height);
        dbg_printf("  width=%d, height=%d, req_width=%d, req_height=%d\n", width, height, req_width, req_height);
        if (req_width > width || req_height > height) {
            if (req_width > width)
                width = req_width;
            if (req_height > height)
                height = req_height;
            dbg_printf("  moving widget to %d, %d, %d, %d\n", x, y, width, height);
            if (MoveWindow(w->hwnd, x, y, width, height, TRUE) == 0) {
                set_error("MoveWindow failed");
                return -1;
            }
            return arrange(get_parent_window(w));
        }
        break;
    case UI_DISABLED:
        i_arg = va_arg(ap, int);
        EnableWindow(w->hwnd, i_arg ? FALSE : TRUE);
        return 0;
    case UI_FLEX:
        i_arg = va_arg(ap, int);
        w->flex = i_arg;
        window = get_parent_window(w);
        if (window != NULL)
            arrange(window);
        break;
    default:
        set_error("%s is not supported by %s", PROPERTY_NAME(prop), WIDGET_TYPE_NAME(w->type));
        return -1;
    }

    return 0;
}

static int
widget_get(ui_widget_t w, int prop, va_list ap)
{
    char    **sp;
    int     *ip;

    dbg_printf("widget_get(w=%p, prop=%s, ...)\n", w, PROPERTY_NAME(prop));

    if (w == NULL || w->type == UI_WINDOW || INVALID_PROPERTY(prop)) {
        set_error("invalid arguments");
        return -1;
    }

    switch (prop) {
    case UI_X:
        ip = va_arg(ap, int *);
        if (ip == NULL) {
            set_error("invalid arguments");
            return -1;
        }
        widget_get_possize(w, ip, NULL, NULL, NULL);
        break;
    case UI_Y:
        ip = va_arg(ap, int *);
        if (ip == NULL) {
            set_error("invalid arguments");
            return -1;
        }
        widget_get_possize(w, NULL, ip, NULL, NULL);
        break;
    case UI_WIDTH:
        ip = va_arg(ap, int *);
        if (ip == NULL) {
            set_error("invalid arguments");
            return -1;
        }
        widget_get_possize(w, NULL, NULL, ip, NULL);
        break;
    case UI_HEIGHT:
        ip = va_arg(ap, int *);
        if (ip == NULL) {
            set_error("invalid arguments");
            return -1;
        }
        widget_get_possize(w, NULL, NULL, NULL, ip);
        break;
    case UI_TEXT:
        sp = va_arg(ap, char **);
        if (sp == NULL) {
            set_error("invalid arguments");
            return -1;
        }
        return window_get_text(w, sp);
        break;
    case UI_DISABLED:
        ip = va_arg(ap, int *);
        if (ip == NULL) {
            set_error("invalid arguments");
            return -1;
        }
        *ip = IsWindowEnabled(w->hwnd) ? 0 : 1;
        break;
    case UI_FLEX:
        ip = va_arg(ap, int *);
        if (ip == NULL) {
            set_error("invalid arguments");
            return -1;
        }
        *ip = w->flex;
        break;
    default:
        set_error("%s is not supported by %s", PROPERTY_NAME(prop), WIDGET_TYPE_NAME(w->type));
        return -1;
    }

    return 0;
}

static int
load_ui_defaults(void)
{
    NONCLIENTMETRICS     met;

    dbg_printf("load_ui_defaults()\n");

    DeleteObject(hCaptionFont);
    DeleteObject(hSmallCaptionFont);
    DeleteObject(hMenuFont);
    DeleteObject(hStatusFont);
    DeleteObject(hMessageFont);

    hCaptionFont      = NULL;
    hSmallCaptionFont = NULL;
    hMenuFont         = NULL;
    hStatusFont       = NULL;
    hMessageFont      = NULL;

    met.cbSize = sizeof(met);
    if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(met), &met, 0) == FALSE) {
        dbg_printf("SystemParametersInfo failed\n");
        return -1;
    }

    hCaptionFont      = CreateFontIndirect(&met.lfCaptionFont);
    hSmallCaptionFont = CreateFontIndirect(&met.lfSmCaptionFont);
    hMenuFont         = CreateFontIndirect(&met.lfMenuFont);
    hStatusFont       = CreateFontIndirect(&met.lfStatusFont);
    hMessageFont      = CreateFontIndirect(&met.lfMessageFont);
    iCaptionWidth     = met.iCaptionWidth;
    iCaptionHeight    = met.iCaptionHeight;

    dbg_printf(" iCaptionWidth=%d\n iCaptionHeight=%d\n", iCaptionWidth, iCaptionHeight);

    return 0;
}

static int
widget_set_default_font(ui_widget_t w)
{
    assert(w != NULL);
    assert(!INVALID_WIDGET_TYPE(w->type));
    assert(w->hwnd != NULL);

    switch (w->type) {
    case UI_TEXTBOX:
        SendMessage(w->hwnd, WM_SETFONT, (WPARAM) hMessageFont, (LPARAM) TRUE);
        break;
    default:
        SendMessage(w->hwnd, WM_SETFONT, (WPARAM) hCaptionFont, (LPARAM) TRUE);
        break;
    }

    return 0;
}

static int
load_controls(void)
{
    INITCOMMONCONTROLSEX     initctrls;

    memset(&initctrls, 0, sizeof(initctrls));
    initctrls.dwSize = sizeof(initctrls);
    initctrls.dwICC = ICC_STANDARD_CLASSES;
    if (InitCommonControlsEx(&initctrls) == FALSE)
        dbg_printf("InitCommonControlsEx failed\n");

    return 0;
}

static ui_widget_t
box_create(ui_widget_t parent, int type)
{
    ui_widget_t w;

    assert(parent != NULL);
    assert(parent->hwnd != NULL);
    assert(type == UI_VBOX || type == UI_HBOX);

    w = calloc(1, sizeof(*w));
    if (w == NULL) {
        set_error("memory allocation failed");
        return NULL;
    }

    w->type = type;
    w->parent = parent;
    w->hwnd = parent->hwnd;

    return w;
}

static int
box_set(ui_widget_t w, int prop, va_list ap)
{
    assert(w != NULL);
    assert(w->type == UI_VBOX || w->type == UI_HBOX);

    return widget_set(w, prop, ap);
}

static int
box_get(ui_widget_t w, int prop, va_list ap)
{
    assert(w != NULL);
    assert(w->type == UI_VBOX || w->type == UI_HBOX);

    return widget_get(w, prop, ap);
}

static HWND
get_hwnd(ui_widget_t w)
{
    while (w != NULL) {
        if (w->hwnd != NULL)
            return w->hwnd;
        w = w->parent;
    }
    return NULL;
}

static ui_widget_t
get_parent_window(ui_widget_t w)
{
    assert(w != NULL);

    while (w->type != UI_WINDOW && w != NULL)
        w = w->parent;
    return w;
}

static int
vbox_get_required_size(ui_widget_t box, int *widthp, int *heightp)
{
    int width, height, max_width, sum_height;
    ui_widget_t child;

    assert(box != NULL);
    assert(box->type == UI_VBOX);

    max_width = 0;
    sum_height = 0;
    for (child = box->child; child != NULL; child = child->next) {
        widget_get_required_size(child, &width, &height);
        if (width > max_width)
            max_width = width;
        sum_height += height;
    }

    if (widthp != NULL)
        *widthp = max_width;
    if (heightp != NULL)
        *heightp = sum_height;

    return 0;
}

static int
hbox_get_required_size(ui_widget_t box, int *widthp, int *heightp)
{
    int          width, height, sum_width, max_height;
    ui_widget_t  child;

    assert(box != NULL);
    assert(box->type == UI_HBOX);

    sum_width = 0;
    max_height = 0;
    for (child = box->child; child != NULL; child = child->next) {
        widget_get_required_size(child, &width, &height);
        if (height > max_height)
            max_height = height;
        sum_width += width;
    }

    if (widthp != NULL)
        *widthp = sum_width;
    if (heightp != NULL)
        *heightp = max_height;

    return 0;
}

static ui_widget_t 
label_create(ui_widget_t parent)
{
    ui_widget_t w;

    dbg_printf("label_create(parent=%p)\n", parent);

    assert(parent != NULL && (parent->type == UI_WINDOW || parent->type == UI_VBOX || parent->type == UI_HBOX) && parent->hwnd != NULL);

    w = calloc(1, sizeof *w);
    if (w == NULL) {
        set_error("memory allocation failed");
        return NULL;
    }

    w->type = UI_LABEL;
    w->hwnd = CreateWindow(L"STATIC",
                           L"",
                           WS_CHILD | WS_VISIBLE | WS_VISIBLE,
                           0,
                           0,
                           LABEL_MIN_WIDTH,
                           LABEL_MIN_HEIGHT,
                           parent->hwnd,
                           NULL,
                           hInstance,
                           NULL);
    if (w->hwnd == NULL) {
        free(w);
        set_error("CreateWindow failed");
        return NULL;
    }

    widget_set_default_font(w);

    return w;
}

static int
label_set(ui_widget_t w, int prop, va_list ap)
{
    dbg_printf("label_set(w=%p, prop=%s, ...)\n", w, PROPERTY_NAME(prop));

    return widget_set(w, prop, ap);
}

static int
label_get(ui_widget_t w, int prop, va_list ap)
{
    dbg_printf("label_get(w=%p, prop=%s, ...)\n", w, PROPERTY_NAME(prop));

    return widget_get(w, prop, ap);
}

static ui_widget_t
button_create(ui_widget_t parent)
{
    ui_widget_t w;

    dbg_printf("button_create(parent=%p)\n", parent);

    assert(parent != NULL && (parent->type == UI_WINDOW || parent->type == UI_VBOX || parent->type == UI_HBOX) && parent->hwnd != NULL);

    w = calloc(1, sizeof(*w));
    if (w == NULL) {
        set_error("memory allocation failed");
        return NULL;
    }

    w->type = UI_BUTTON;
    w->hwnd = CreateWindow(L"BUTTON",
                           L"",
                           WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                           0,
                           0,
                           iCaptionWidth,
                           iCaptionHeight,
                           parent->hwnd,
                           NULL,
                           hInstance,
                           NULL);
    if (w->hwnd == NULL) {
        free(w);
        set_error("CreateWindow failed");
        return NULL;
    }

    widget_set_default_font(w);

    return w;
}

static int
button_set(ui_widget_t w, int prop, va_list ap)
{
    void *p;

    dbg_printf("button_set(w=%p, prop=%s, ...)\n", w, PROPERTY_NAME(prop));

    switch (prop) {
    case UI_ON_CLICK:
        p = va_arg(ap, void *);
        if (p == NULL) {
            set_error("invalid arguments");
            return -1;
        }
        w->on_click.callback = p;
        w->on_click.userdata = va_arg(ap, void *);
        return 0;
    };

    return widget_set(w, prop, ap);
}

static int
button_get(ui_widget_t w, int prop, va_list ap)
{
    dbg_printf("button_get(w=%p, prop=%s, ...)\n", w, PROPERTY_NAME(prop));

    return widget_get(w, prop, ap);
}

static ui_widget_t
textbox_create(ui_widget_t parent, ui_widget_t w, int multiline)
{
    int          x, y, width, height;
    HWND         hwnd, parent_hwnd;
    char        *buf = NULL;

    dbg_printf("textbox_create(parent=%p, w=%p, multiline=%d)\n", parent, w, multiline);

    assert((parent != NULL) || (w != NULL && w->parent != NULL));

    parent_hwnd = parent == NULL ? w->parent->hwnd : parent->hwnd;
    hwnd = CreateWindowEx(WS_EX_CLIENTEDGE,
                          L"EDIT",
                          L"",
                          WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_LEFT | (multiline ? (ES_MULTILINE | ES_AUTOVSCROLL) : 0),
                          0,
                          0,
                          TEXTBOX_MIN_WIDTH,
                          TEXTBOX_MIN_HEIGHT,
                          parent_hwnd,
                          NULL,
                          hInstance,
                          NULL);
    if (hwnd == NULL) {
        set_error("CreateWindow failed\n");
        return NULL;
    }

    if (parent == NULL) {
        /* we're changing the UI_MULTILINE property on an existing widget */
        if (window_get_text(w, &buf) == -1) {
            DestroyWindow(hwnd);
            return NULL;
        }
        widget_get_possize(w, &x, &y, &width, &height);
        MoveWindow(hwnd, x, y, width, height, TRUE);
        DestroyWindow(w->hwnd);
        w->hwnd = hwnd;
        widget_set_default_font(w);
        if (window_set_text(w, buf) == -1) {
            free(buf);
            return NULL;
        }
        free(buf);
    } else {
        /* we're creating a brand new widget */
        w = calloc(1, sizeof(*w));
        if (w == NULL) {
            set_error("memory allocation failed");
            DestroyWindow(hwnd);
            return NULL;
        }
        w->type = UI_TEXTBOX;
        w->hwnd = hwnd;

        widget_set_default_font(w);

        if (widget_get_required_size(w, &width, &height) != -1)
            MoveWindow(w->hwnd, 0, 0, width, height, TRUE);
    }

    if (multiline)
        w->u.textbox.multiline = 1;
    else
        w->u.textbox.multiline = 0;

    return w;
}

static int
textbox_set(ui_widget_t w, int prop, va_list ap)
{
    int      i_arg;
    void    *p;

    dbg_printf("textbox_set(w=%p, prop=%s, ...)\n", w, PROPERTY_NAME(prop));

    assert(w != NULL);
    assert(w->type == UI_TEXTBOX);
    assert(!INVALID_PROPERTY(prop));

    switch (prop) {
    // FIXME maybe it is possible to use SetWindowLong
    case UI_MULTILINE:
        i_arg = va_arg(ap, int);
        i_arg = i_arg ? 1 : 0;
        if (i_arg == w->u.textbox.multiline)
            return 0;
        if (i_arg)
            p = textbox_create(NULL, w, 1);
        else
            p = textbox_create(NULL, w, 0);
        if (p == NULL)
            return -1;
        break;
    default:
        return widget_set(w, prop, ap);
        break;
    }

    return 0;
}

static int
textbox_get(ui_widget_t w, int prop, va_list ap)
{
    int     *ip;

    dbg_printf("textbox_get(w=%p, prop=%s, ...)\n", w, PROPERTY_NAME(prop));

    assert(w != NULL);
    assert(w->type == UI_TEXTBOX);
    assert(!INVALID_PROPERTY(prop));

    switch (prop) {
    case UI_MULTILINE:
        ip = va_arg(ap, int *);
        if (ip == NULL) {
            set_error("invalid arguments");
            return -1;
        }
        *ip = w->u.textbox.multiline;
        break;
    default:
        return widget_get(w, prop, ap);
        break;
    }

    return 0;
}

static LRESULT CALLBACK
WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT          ps;
    HDC                  hdc;
    int                  event_type, button, x, y;
    ui_widget_t          widget;
    ui_event_handler_t   callback;
    void                *userdata;
    ui_event_t           event;
    
    switch (iMsg) {
    case WM_COMMAND:
        dbg_printf("WM_COMMAND\n");

        if (HIWORD(wParam) != BN_CLICKED)
            break;
        dbg_printf(" HIWORD(wParam) == BN_CLICKED\n");
        widget = widgets_find((HWND) lParam);
        if (widget != NULL && widget->on_click.callback != NULL) {
            event = event_create(UI_ON_CLICK);
            widget->on_click.callback(widget, event, widget->on_click.userdata);
            free(event);
        }
        return 0;

    case WM_PAINT:
        /*  We receive WM_PAINT every time window is updated  */
        hdc = BeginPaint(hwnd, &ps);
        EndPaint(hwnd, &ps);
        return 0;

    case WM_DESTROY:
        /*  Window has been destroyed, so exit cleanly  */
        PostQuitMessage(0);
        return 0;

    case WM_SIZE:
        widget = widgets_find(hwnd);
        if (widget != NULL && widget->type == UI_WINDOW)
            arrange(widget);
        break;

    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
        // dbg_printf("WndProc: WM_{L,M,R}BUTTON{UP,DOWN}: hwnd=%p, iMsg=%x, wParam=%x, lParam=%x\n", hwnd, iMsg, wParam, lParam);

        widget = widgets_find(hwnd);
        if (widget == NULL)
            break;

        if (widget->type != UI_WINDOW)
            break;

        switch (iMsg) {
        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
            event_type = UI_ON_MOUSEDOWN;
            callback = widget->on_mousedown.callback;
            userdata = widget->on_mousedown.userdata;
            break;
        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP:
            event_type = UI_ON_MOUSEUP;
            callback = widget->on_mouseup.callback;
            userdata = widget->on_mouseup.userdata;
            break;
        }

        if (callback == NULL)
            break;

        switch (iMsg) {
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
            button = 0;
            break;
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
            button = 1;
            break;
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
            button = 2;
            break;
        }

        x = GET_X_LPARAM(lParam);
        y = GET_Y_LPARAM(lParam);

        event = event_create(event_type, button, x, y);
        (*callback)(widget, event, userdata);
        free(event);

        return 0;

    case WM_MOUSEMOVE:
        // dbg_printf("WndProc: WM_MOUSEMOVE: hwnd=%p, iMsg=%x, wParam=%0x, lParam=%0x\n", hwnd, iMsg, wParam, lParam);
        x = GET_X_LPARAM(lParam);
        y = GET_Y_LPARAM(lParam);

        widget = widgets_find(hwnd);
        if (widget == NULL)
            break;

        if (widget->type != UI_WINDOW)
            break;

        callback = widget->on_mousemove.callback;
        userdata = widget->on_mousemove.userdata;

        if (callback == NULL)
            break;

        event = event_create(UI_ON_MOUSEMOVE, x, y);
        (*callback)(widget, event, userdata);
        free(event);

        return 0;

    case WM_CLOSE:
        widget = widgets_find(hwnd);
        if (widget == NULL)
            break;
        if (widget->type != UI_WINDOW)
            break;

        callback = widget->on_close.callback;
        userdata = widget->on_close.userdata;

        if (callback == NULL)
            break;

        event = calloc(1, sizeof(event));
        (*callback)(widget, event, userdata);
        free(event);

        return 0;
    }

    /*  Send any messages we don't handle to default window procedure  */
    return DefWindowProc(hwnd, iMsg, wParam, lParam);
}


