#include "demo.hh"

#include <structured-interface/si.hh>

void si::show_demo_window()
{
    static bool show_basic_controls = false;

    if (auto w = si::window("si::demo"))
    {
        if (si::button("basic controls"))
            show_basic_controls = true;
    }
}
