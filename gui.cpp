#include <iostream>
#include <string>
#include <vector>
#include <initializer_list>

#include <gtk/gtk.h>

namespace gui {
    class exception : public std::exception {
    };

    class illegal_op : public exception {
    };

    enum properties {
        WIDTH,
        HEIGHT,
        HOMOGENOUS,
        SPACING,
        ON_CLICK,
        ON_CLOSE,
    };

    class property_value {
        public:
            enum types {
                STRING,
                INTEGER,
                POINTER,
            };

            property_value(const std::string &s) :
                m_type(STRING),
                m_s(s)
            {
            }

            property_value(int i) :
                m_type(INTEGER),
                m_i(i)
            {
            }

            property_value(void *p) :
                m_type(POINTER),
                m_p(p)
            {
            }

            const std::string &sval() const {
                if (m_type != STRING)
                    throw illegal_op();
                return m_s;
            }

            int ival() const {
                if (m_type != INTEGER)
                    throw illegal_op();
                return m_i;
            }

            void *pval() const {
                if (m_type != POINTER)
                    throw illegal_op();
                return m_p;
            }

        private:
            enum types         m_type;
            const std::string  m_s;
            int                m_i;
            void              *m_p;
    };

    class widget {
        public:
            GtkWidget *gtk_widget() const {
                return m_widget;
            }
            
            void show() {
                gtk_widget_show_all(m_widget);
            }

        protected:
            GtkWidget *m_widget;
    };

    class window : public widget {
        public:
            window() :
                m_width(200),
                m_height(200)
            {
                m_widget = gtk_window_new(GTK_WINDOW_TOPLEVEL);
                gtk_window_set_default_size(GTK_WINDOW(m_widget), m_width, m_height);
            }

            template<typename ... Types>
            window(Types ... args) :
                window()
            {
                vctor(args...);
            }

            void push_back(const widget &w) {
                gtk_container_add(GTK_CONTAINER(m_widget), w.gtk_widget());
            }
        private:
            template<typename ... Types>
            void vctor(enum properties property, const property_value &value, Types ... args) {
                std::clog << "vctor(enum properties property, const property_value &value, Types ... args)" << std::endl;
                switch (property) {
                case WIDTH:
                    m_width = value.ival();
                    gtk_window_set_default_size(GTK_WINDOW(m_widget), m_width, m_height);
                    break;
                case HEIGHT:
                    m_height = value.ival();
                    gtk_window_set_default_size(GTK_WINDOW(m_widget), m_width, m_height);
                    break;
                }
                vctor(args...);
            }

            template<typename ... Types>
            void vctor(const widget &w, Types ... args) {
                push_back(w);
                vctor(args...);
            }

            void vctor() {
            }

            int m_width, m_height;
    };

    class vbox : public widget {
        public:
            vbox() {
                m_widget = gtk_vbox_new(FALSE, 0);
            }

            template<typename ... Types>
            vbox(Types ... args) :
                vbox()
            {
                push_back(args...);
            }

            void push_back(const widget &w) {
                gtk_box_pack_start(GTK_BOX(m_widget), w.gtk_widget(), FALSE, FALSE, 0);
            }

            template<typename ... Types>
            void push_back(const widget &w, Types ... args) {
                push_back(w);
                push_back(args...);
            }

        private:
            template<typename ... Types> void vctor(enum properties property, const property_value &value, Types ... args) {
            }

            template<typename ... Types> void vctor(const widget &w, Types ... args) {
            }

            void vctor() {
            }

    };

    class button : public widget {
        public:
            button(const std::string &label) {
                m_widget = gtk_button_new_with_label(label.c_str());
            }
    };

    class label : public widget {
        public:
            label(const std::string &value) {
                m_widget = gtk_label_new(value.c_str());
            }
    };
};

static GtkWidget *window, *vbox, *label, *button;

int main(int argc, char **argv) {

    gtk_init(&argc, &argv);

    gui::window window {
        gui::WIDTH, 400,
        gui::HEIGHT, 300,
        gui::vbox {
            gui::label { "foobar" },
            gui::button { "Push me" },
        },
    };

    window.show();

    gtk_main();

    return 0;
}
