#include "App.hpp"

int main() {
    if (App app; app.init()) {
        app.run();
    }
}
