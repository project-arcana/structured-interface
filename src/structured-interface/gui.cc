#include "gui.hh"

#include <structured-interface/detail/recorder.hh>
#include <structured-interface/element_tree.hh>

si::recorded_ui si::gui::record(cc::function_ref<void()> do_record)
{
    si::detail::start_recording(*this);

    // call user UI recorder
    do_record();

    // TODO: less copying
    cc::vector<std::byte> data;
    si::detail::end_recording(data);
    return recorded_ui(cc::move(data));
}

void si::gui::update(si::recorded_ui const& ui, cc::function_ref<si::element_tree(si::element_tree const&, si::element_tree&&)> merger)
{
    // convert to element tree
    auto new_ui = element_tree::from_record(ui);

    // perform merge
    *_current_ui = merger(*_current_ui, cc::move(new_ui));
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

si::gui::~gui() = default; // unique_ptr

si::gui::gui()
{
    // empty UI as start
    _current_ui = cc::make_unique<element_tree>();
}
