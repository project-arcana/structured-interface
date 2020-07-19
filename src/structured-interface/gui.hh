#pragma once

#include <clean-core/function_ref.hh>
#include <clean-core/span.hh>
#include <clean-core/string_view.hh>
#include <clean-core/unique_ptr.hh>
#include <clean-core/vector.hh>

#include <structured-interface/fwd.hh>
#include <structured-interface/recorded_ui.hh>

namespace si
{
/// a user interface.
///
/// this struct is a value-type, i.e. it can be copied and passed around
/// multiple si::gui instances can be created, recorded, merged, stored, loaded, tested.
///
/// TODO: this might get problematic due to shared state and events (BUT maybe via merge phase stuff)
///   - multi-threaded UI creation: create one si::gui per thread and merge the results at the end
///   - compose multiple UIs by merging si::gui instances
///   - use si::gui completely without actual input or graphics API (for testing or remote access)
///
/// custom renderers:
///   - opengl/dx/vulkan
///   - terminal
///   - remote
///   - HTML
///   - file (png, svg)
struct gui
{
    // recording
public:
    /// calls the passed function and records all UI elements into a recorded_ui
    [[nodiscard]] recorded_ui record(cc::function_ref<void()> do_record);

    /// takes a recorded UI and merges it with the current ui
    /// this effectively computes an "update step" for the ui
    /// depending on the use case, the merger function has many tasks
    /// (e.g. layouting, input, render call gen, animation, styling)
    ///
    /// TODO: version that changes element_tree in-place?
    void update(recorded_ui const& ui, cc::function_ref<element_tree(element_tree const& old_ui, element_tree&& new_ui)> merger);

    // data structure
public:
    // query API
public:
    bool has(cc::string_view name) const;

public:
    gui();
    ~gui();

    // members
private:
    cc::unique_ptr<element_tree> _current_ui;
};
}
