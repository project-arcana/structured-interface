#include "gui.hh"

#include <structured-interface/detail/recorder.hh>
#include <structured-interface/detail/ui_context.hh>
#include <structured-interface/element_tree.hh>
#include <structured-interface/input_state.hh>

#include <babel-serializer/file.hh>

si::recorded_ui si::gui::record(cc::function_ref<void()> do_record)
{
    // setup recording
    si::detail::current_ui_context().input = _input_state.get();
    si::detail::current_ui_context().prev_ui = _current_ui.get();
    si::detail::start_recording(*this);

    // call user UI recorder
    do_record();

    // for safety:
    si::detail::current_ui_context().input = nullptr;
    si::detail::current_ui_context().prev_ui = nullptr;

    // TODO: less copying
    cc::vector<std::byte> data;
    si::detail::end_recording(data);
    return recorded_ui(cc::move(data));
}

void si::gui::update(si::recorded_ui const& ui, cc::function_ref<si::element_tree(si::element_tree const&, si::element_tree&&, input_state&)> merger)
{
    // convert to element tree
    auto new_ui = element_tree::from_record(ui);

    // prepare next input
    _input_state->on_next_update();

    // perform merge
    *_current_ui = merger(*_current_ui, cc::move(new_ui), *_input_state);
}

bool si::gui::has(cc::string_view name) const
{
    // TODO: name property?
    for (auto const& e : _current_ui->all_elements())
        if (_current_ui->has_property(e, property::text))
            if (_current_ui->get_property(e, property::text) == name)
                return true;

    return false;
}

void si::gui::load_ui_state()
{
    if (!babel::file::exists(_ui_file))
        return;

    auto data = babel::file::read_all_bytes(_ui_file);
    *_current_ui = element_tree::from_binary_data(data);
}

void si::gui::save_ui_state()
{
    auto data = _current_ui->to_binary_data();
    babel::file::write(_ui_file, data);
}

si::gui::~gui()
{
    // save on shutdown
    save_ui_state();
}

si::gui::gui()
{
    // empty UI as start
    _current_ui = cc::make_unique<element_tree>();
    _input_state = cc::make_unique<input_state>();

    // TODO: make configurable
    load_ui_state();
}
