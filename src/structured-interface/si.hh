#pragma once

// TODO: can probably be replaced by a fwd decl
#include <clean-core/string.hh>

#include <clean-core/format.hh>
#include <clean-core/string_view.hh>
#include <clean-core/to_string.hh>
#include <clean-core/type_id.hh>

#include <typed-geometry/tg-lean.hh>

#include <structured-interface/detail/record.hh>
#include <structured-interface/detail/ui_context.hh>
#include <structured-interface/element_type.hh>
#include <structured-interface/handles.hh>
#include <structured-interface/input_state.hh>
#include <structured-interface/properties.hh>

// NOTE: this header includes all important user elements
//       (i.e. what is needed to create the UIs)

// =======================================
//
//   Element type definitions
//
//  (scroll down to "Element Creation API" for the actual si API)
//

namespace si
{
struct ui_element_base
{
    element_handle id;

    bool was_clicked() const { return detail::current_input_state().was_clicked(id); }
    bool is_hovered() const { return detail::current_input_state().is_hovered(id); }
    bool is_pressed() const { return detail::current_input_state().is_pressed(id); }
    bool is_dragged() const { return detail::current_input_state().is_dragged(id); }
    // ...

    /// returns layouted aabb of this element
    /// NOTE: not all mergers might set this
    /// NOTE: returns an empty aabb if none set (TODO: is this good API?)
    tg::aabb2 aabb() const;

    ui_element_base(element_handle id) : id(id) { CC_ASSERT(id.is_valid()); }
    ~ui_element_base() { si::detail::end_element(id); }

    // non-copyable / non-movable
    ui_element_base(ui_element_base&&) = delete;
    ui_element_base(ui_element_base const&) = delete;
    ui_element_base& operator=(ui_element_base&&) = delete;
    ui_element_base& operator=(ui_element_base const&) = delete;
};

template <class this_t>
struct ui_element : ui_element_base
{
    using ui_element_base::ui_element_base;

    this_t& tooltip(cc::string_view text)
    {
        // TODO
        (void)text;
        return static_cast<this_t&>(*this);
    }
};
template <class this_t>
struct scoped_ui_element : ui_element<this_t>
{
    scoped_ui_element(element_handle id, bool children_visible) : ui_element<this_t>(id), _children_visible(children_visible) {}

    explicit operator bool() const&& = delete; // make "if (si::window(...))" a compile error
    explicit operator bool() const& { return _children_visible; }

private:
    bool _children_visible = false;
};

template <class this_t>
struct world_element
{
    // TODO: see ui_element
    operator bool() const { return false; }
};

template <class T>
struct input_t : ui_element<input_t<T>>
{
};
template <class T>
struct slider_t : ui_element<slider_t<T>>
{
};
struct button_t : ui_element<button_t>
{
    operator bool() const { return was_clicked(); }
};
struct clickable_area_t : ui_element<clickable_area_t>
{
    operator bool() const { return was_clicked(); }
};
struct slider_area_t : ui_element<slider_area_t>
{
    slider_area_t(element_handle id, bool changed) : ui_element(id), _changed(changed) {}
    bool was_changed() const { return _changed; }
    operator bool() const { return _changed; }

private:
    bool _changed = false;
};
struct checkbox_t : ui_element<checkbox_t>
{
    checkbox_t(element_handle id, bool changed) : ui_element(id), _changed(changed) {}
    bool was_changed() const { return _changed; }
    operator bool() const { return _changed; }

private:
    bool _changed = false;
};
struct toggle_t : ui_element<toggle_t>
{
};
struct text_t : ui_element<text_t>
{
};
template <class T>
struct radio_button_t : ui_element<radio_button_t<T>>
{
    radio_button_t(element_handle id, bool changed) : ui_element<radio_button_t<T>>(id), _changed(changed) {}
    bool was_changed() const { return _changed; }
    operator bool() const { return _changed; }

private:
    bool _changed = false;
};
template <class T>
struct dropdown_t : ui_element<dropdown_t<T>>
{
};
template <class T>
struct listbox_t : ui_element<listbox_t<T>>
{
};
template <class T>
struct combobox_t : ui_element<combobox_t<T>>
{
};

struct window_t : scoped_ui_element<window_t>
{
    using scoped_ui_element<window_t>::scoped_ui_element;
};
struct tree_node_t : scoped_ui_element<tree_node_t>
{
    using scoped_ui_element<tree_node_t>::scoped_ui_element;
};
struct tabs_t : scoped_ui_element<tabs_t>
{
    using scoped_ui_element<tabs_t>::scoped_ui_element;
};
struct tab_t : scoped_ui_element<tab_t>
{
    using scoped_ui_element<tab_t>::scoped_ui_element;
};
struct grid_t : scoped_ui_element<grid_t>
{
    using scoped_ui_element<grid_t>::scoped_ui_element;
};
struct row_t : scoped_ui_element<row_t>
{
    using scoped_ui_element<row_t>::scoped_ui_element;
};
struct flow_t : scoped_ui_element<flow_t>
{
    using scoped_ui_element<flow_t>::scoped_ui_element;
};
struct container_t : scoped_ui_element<container_t>
{
    using scoped_ui_element<container_t>::scoped_ui_element;
};
struct canvas_t : scoped_ui_element<canvas_t>
{
    using scoped_ui_element<canvas_t>::scoped_ui_element;
};

struct gizmo_t : world_element<gizmo_t>
{
};

// =======================================
//
//    Element Creation API
//

/**
 * shows a simple line of text
 *
 * usage:
 *
 *   si::text("hello world");
 */
text_t text(cc::string_view text);

/**
 * uses cc::format to create a text element
 *
 * usage:
 *
 *   si::text("values are {} and {}", 17, true);
 */
template <class A, class... Args>
text_t text(char const* format, A const& firstArg, Args const&... otherArgs)
{
    auto id = si::detail::start_element(element_type::text, format);
    // TODO: replace by some buffer? at least make it non-allocating
    auto txt = cc::format(format, firstArg, otherArgs...);
    si::detail::write_property(id, si::property::text, cc::string_view(txt));
    return {id};
}

/**
 * creates a clickable button with a given caption
 * can be cast to bool to see if button was clicked
 *
 * usage:
 *
 *   if (si::button("destroy world"))
 *      destroy_world();
 */
button_t button(cc::string_view text);

/**
 * creates an invisible clickable button
 * can be cast to bool to see if button was clicked
 * NOTE: is useful for fine-control over interactions
 * TODO: make sized version (currently requires layout support)
 * TODO: parameter to customize ID?
 *
 * usage:
 *
 *   if (si::clickable_area())
 *      my_behavior();
 */
clickable_area_t clickable_area();

/**
 * creates a checkbox with description text
 * when clicked, toggles the value of "ok"
 * can be cast to bool to see if value was changed
 *
 * usage:
 *
 *   bool value = ...;
 *   changed |= si::checkbox("bool value", value);
 */
checkbox_t checkbox(cc::string_view text, bool& ok);

/**
 * creates an invisible slider area
 * can be cast to bool to see if value was changed
 * the parameter is set to 0..1 (both inclusive) depending on slider position
 * TODO: make sized version (currently requires layout support)
 * TODO: parameter to customize ID?
 *
 * usage:
 *
 *   float t = ...;
 *   changed |= slider_area(t);
 */
slider_area_t slider_area(float& t);

/**
 * creates a slider with description text
 * can be cast to bool to see if value was changed
 * TODO: add "always clamp" parameter
 * TODO: add "step" parameter
 *
 * usage:
 *
 *   int my_int = ...;
 *   float my_float = ...;
 *   changed |= si::slider("int slider", my_int, -10, 10);
 *   changed |= si::slider("float slider", my_float, 0.0f, 1.0f);
 *
 * DOM notes:
 *   - contains a single slider_area with text parameter as child
 */
template <class T>
slider_t<T> slider(cc::string_view text, T& value, tg::dont_deduce<T> const& min, tg::dont_deduce<T> const& max_inclusive)
{
    CC_ASSERT(max_inclusive >= min && "invalid range");
    auto id = si::detail::start_element(element_type::slider, text);
    si::detail::write_property(id, si::property::text, text);

    // TODO: proper handling of large doubles, extreme cases
    float t = value < min ? 0.f : value > max_inclusive ? 1.f : float(value - min) / float(max_inclusive - min);
    auto slider = si::slider_area(t);
    // TODO: non-allocating version
    si::detail::write_property(slider.id, si::property::text, cc::string_view(cc::to_string(value)));

    if (slider.was_changed())
    {
        if constexpr (std::is_integral_v<T>)
            value = min + T((max_inclusive - min) * t + 0.5f);
        else
            value = min + (max_inclusive - min) * t;
    }

    return {id};
}

/**
 * creates a movable, collapsible, closable window
 * cast to bool is used to determine if content is visible
 * TODO: add parameters
 *
 * usage:
 *
 *   if (auto w = si::window("my window"))
 *   {
 *       // .. child elements
 *   }
 *
 * DOM notes:
 *   - first child is a clickable_area (for title area)
 */
[[nodiscard]] window_t window(cc::string_view title);


// =======================================
//
//  not implemented:

inline toggle_t toggle(cc::string_view text, bool& ok)
{
    auto id = si::detail::start_element(element_type::toggle, text);
    // TODO
    return {id};
}

template <class T>
input_t<T> input(cc::string_view text, T& value)
{
    auto id = si::detail::start_element(element_type::input, text);
    // TODO
    return {id};
}

[[nodiscard]] inline radio_button_t<void> radio_button(cc::string_view text, bool active)
{
    auto id = si::detail::start_element(element_type::radio_button, text);
    // TODO
    return {id, false};
}
template <class T>
radio_button_t<T> radio_button(cc::string_view text, T& value, tg::dont_deduce<T const&> option)
{
    auto id = si::detail::start_element(element_type::radio_button, text);
    // TODO
    return {id, false};
}
template <class T>
dropdown_t<T> dropdown(cc::string_view text, T& value, tg::dont_deduce<tg::span<T>> options)
{
    auto id = si::detail::start_element(element_type::dropdown, text);
    // TODO
    return {id};
}
template <class T, class OptionsT>
dropdown_t<T> dropdown(cc::string_view text, T& value, OptionsT const& options, tg::span<cc::string> names)
{
    auto id = si::detail::start_element(element_type::dropdown, text);
    // TODO
    return {id};
}
template <class T, class OptionsT>
listbox_t<T> listbox(cc::string_view text, T& value, OptionsT const& options)
{
    auto id = si::detail::start_element(element_type::listbox, text);
    // TODO
    return {id};
}
template <class T, class OptionsT>
listbox_t<T> listbox(cc::string_view text, T& value, OptionsT const& options, tg::span<cc::string> names)
{
    auto id = si::detail::start_element(element_type::listbox, text);
    // TODO
    return {id};
}
template <class T, class OptionsT>
combobox_t<T> combobox(cc::string_view text, T& value, OptionsT const& options)
{
    auto id = si::detail::start_element(element_type::combobox, text);
    // TODO
    return {id};
}
template <class T, class OptionsT>
combobox_t<T> combobox(cc::string_view text, T& value, OptionsT const& options, tg::span<cc::string> names)
{
    auto id = si::detail::start_element(element_type::combobox, text);
    // TODO
    return {id};
}

[[nodiscard]] inline tree_node_t tree_node(cc::string_view text)
{
    auto id = si::detail::start_element(element_type::tree_node, text);
    // TODO
    return {id, true};
}
[[nodiscard]] inline tabs_t tabs()
{
    auto id = si::detail::start_element(element_type::tabs);
    // TODO
    return {id, true};
}
[[nodiscard]] inline tab_t tab(cc::string_view title)
{
    auto id = si::detail::start_element(element_type::tab, title);
    // TODO
    return {id, true};
}
[[nodiscard]] inline flow_t flow()
{
    auto id = si::detail::start_element(element_type::flow);
    // TODO
    return {id, true};
}
[[nodiscard]] inline container_t container()
{
    auto id = si::detail::start_element(element_type::container);
    // TODO
    return {id, true};
}
[[nodiscard]] inline grid_t grid()
{
    auto id = si::detail::start_element(element_type::grid);
    // TODO
    return {id, true};
}
[[nodiscard]] inline row_t row()
{
    auto id = si::detail::start_element(element_type::row);
    // TODO
    return {id, true};
}

[[nodiscard]] inline canvas_t canvas()
{
    auto id = si::detail::start_element(element_type::canvas);
    // TODO
    return {id, true};
}

inline gizmo_t gizmo(tg::pos3& pos) // translation gizmo
{
    // TODO: dir3, vec3, size3 versions
    return {}; // TODO
}

// ??
inline void spacing() {}
inline void separator() {}

// helper
struct id_scope_t
{
    cc::hash_t prev_seed;

    explicit id_scope_t(cc::hash_t seed)
    {
        auto& s = si::detail::id_seed();
        prev_seed = s;
        s = seed;
    }
    ~id_scope_t() { si::detail::id_seed() = prev_seed; }

    id_scope_t(id_scope_t const&) = delete;
    id_scope_t(id_scope_t&&) = delete;
    id_scope_t& operator=(id_scope_t const&) = delete;
    id_scope_t& operator=(id_scope_t&&) = delete;
};

template <class T>
[[nodiscard]] inline id_scope_t id_scope(T const& value)
{
    return id_scope_t(si::detail::make_hash(si::detail::id_seed(), value));
}

}
