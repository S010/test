#include <iostream>
#include <string>
#include <vector>
#include <map>

//#include <gtk/gtk.h>

namespace gui {
    enum properties {
        LABEL,
        EXPAND,
        FILL,
        ON_CLICK,
    };

    class property_value {
        public:
            property_value(const char *s) {
            };

            property_value(bool b) {
            }
    };

    class widget {
        public:
            template<typename ... Types> widget(enum properties, property_value, Types ... args) {
            }

            template<typename ... Types> widget(widget w, Types ... args) {
            }

            widget() {
            }

    };
};

void on_hello_click(gui::widget &w) {
}

void test() {
    gui::widget window {
        gui::widget {
            gui::EXPAND, true,
            gui::FILL,   true,

            gui::widget {
                 gui::LABEL,    "Hello" ,
                 gui::ON_CLICK, on_hello_click ,
            },

            gui::widget {
                gui::widget {
                },

                gui::LABEL, "aaa",
            },
        }
    };
}

int main(int argc, char **argv) {
    test();

    return 0;
}
