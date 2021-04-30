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

    //
    // basic controls
    //
    if (auto w = si::window("si::demo - basic controls", show_basic_controls))
    {
        static int click_cnt = 0;
        si::text("button click count: {}", click_cnt);
        if (si::button("button"))
            ++click_cnt;
        if (si::button("button (disabled)", si::disabled))
            ++click_cnt;

        static bool cb_var[] = {false, true, false, true};
        si::checkbox("checkbox (unchecked)", cb_var[0]);
        si::checkbox("checkbox (checked)", cb_var[1]);
        si::checkbox("checkbox (disabled, unchecked)", cb_var[2], si::disabled);
        si::checkbox("checkbox (disabled, checked)", cb_var[3], si::disabled);

        static bool tg_var[] = {false, true, false, true};
        si::toggle("toggle (unchecked)", tg_var[0]);
        si::toggle("toggle (checked)", tg_var[1]);
        si::toggle("toggle (disabled, unchecked)", tg_var[2], si::disabled);
        si::toggle("toggle (disabled, checked)", tg_var[3], si::disabled);

        static int sli_var[] = {0, 100};
        si::slider("slider (int)", sli_var[0], 0, 100);
        si::slider("slider (int, disabled)", sli_var[1], 0, 100).disable();
        // TODO: switch to si::disabled, but needs external property support

        static float slf_var[] = {0.f, 0.5f};
        si::slider("slider (float)", slf_var[0], -1, 1);
        si::slider("slider (float, disabled)", slf_var[1], -1, 1).disable();

        static int rd_var[] = {0, 0};
        if (si::radio_button("radio button 0", rd_var[0] == 0)) // explicit version
            rd_var[0] = 0;
        si::radio_button("radio button 1", rd_var[0], 1); // implicit version
        si::radio_button("radio button 0 (disabled)", rd_var[1], 0, si::disabled);
        si::radio_button("radio button 1 (disabled)", rd_var[1], 1, si::disabled);

        static int dd_var[] = {0, 0};
        si::dropdown("dropdown", dd_var[0], {0, 1, 2, 3});
        si::dropdown("dropdown (named)", dd_var[1], {0, 1, 2, 3}, {"zero", "one", "two", "three"});
    }
}
