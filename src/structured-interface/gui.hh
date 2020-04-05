#pragma once

#include <clean-core/function_ref.hh>
#include <clean-core/string_view.hh>

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
public:
    [[nodiscard]] recorded_ui record(cc::function_ref<void()> do_record);

    // query API
public:
    bool has(cc::string_view name) const;
};
}
