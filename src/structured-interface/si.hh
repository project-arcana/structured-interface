#pragma once

// TODO: can probably be replaced by a fwd decl
#include <clean-core/string.hh>

#include <clean-core/format.hh>
#include <clean-core/string_view.hh>
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
template <class this_t>
struct ui_element
{
    element_handle id;

    // TODO: return true iff change
    // TODO: explicit or not? changed |= pattern relies on implicit
    // TOOD: only for those ui elements that can actually change / have useful feedback
    operator bool() const { return false; }

    bool was_clicked() const { return detail::current_input_state().was_clicked(id); }
    bool is_hovered() const { return detail::current_input_state().is_hovered(id); }
    bool is_pressed() const { return detail::current_input_state().is_pressed(id); }
    // ...

    this_t& tooltip(cc::string_view text)
    {
        // TODO
        return static_cast<this_t&>(*this);
    }

    ui_element(element_handle id) : id(id) { CC_ASSERT(id.is_valid()); }
    ~ui_element() { si::detail::end_element(id); }

    // non-copyable / non-movable
    ui_element(ui_element&&) = delete;
    ui_element(ui_element const&) = delete;
    ui_element& operator=(ui_element&&) = delete;
    ui_element& operator=(ui_element const&) = delete;
};
template <class this_t>
struct scoped_ui_element : ui_element<this_t>
{
    using ui_element<this_t>::ui_element;

    operator bool() const&& = delete; // make "if (si::window(...))" a compile error
    // TODO: see ui_element
    operator bool() const& { return false; }
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
};
struct tree_node_t : scoped_ui_element<tree_node_t>
{
};
struct tabs_t : scoped_ui_element<tabs_t>
{
};
struct tab_t : scoped_ui_element<tab_t>
{
};
struct grid_t : scoped_ui_element<grid_t>
{
};
struct row_t : scoped_ui_element<row_t>
{
};
struct flow_t : scoped_ui_element<flow_t>
{
};
struct container_t : scoped_ui_element<container_t>
{
};
struct canvas_t : scoped_ui_element<canvas_t>
{
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
 * creates a slider with description text
 * can be cast to bool to see if value was changed
 * TODO: add "always clamp" parameter
 *
 * usage:
 *
 *   int my_int = ...;
 *   float my_float = ...;
 *   changed |= si::slider("int slider", my_int, -10, 10);
 *   changed |= si::slider("float slider", my_float, 0.0f, 1.0f);
 */
template <class T>
slider_t<T> slider(cc::string_view text, T& value, tg::dont_deduce<T> const& min, tg::dont_deduce<T> const& max)
{
    auto id = si::detail::start_element(element_type::slider, text);
    si::detail::write_property(id, si::property::text, text);
    // TODO
    return {id};
}


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
    return {id};
}
template <class T>
radio_button_t<T> radio_button(cc::string_view text, T& value, tg::dont_deduce<T const&> option)
{
    auto id = si::detail::start_element(element_type::radio_button, text);
    // TODO
    return {id};
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

[[nodiscard]] inline window_t window(cc::string_view title)
{
    auto id = si::detail::start_element(element_type::window, title);
    // TODO
    return {id};
}
[[nodiscard]] inline tree_node_t tree_node(cc::string_view text)
{
    auto id = si::detail::start_element(element_type::tree_node, text);
    // TODO
    return {id};
}
[[nodiscard]] inline tabs_t tabs()
{
    auto id = si::detail::start_element(element_type::tabs);
    // TODO
    return {id};
}
[[nodiscard]] inline tab_t tab(cc::string_view title)
{
    auto id = si::detail::start_element(element_type::tab, title);
    // TODO
    return {id};
}
[[nodiscard]] inline flow_t flow()
{
    auto id = si::detail::start_element(element_type::flow);
    // TODO
    return {id};
}
[[nodiscard]] inline container_t container()
{
    auto id = si::detail::start_element(element_type::container);
    // TODO
    return {id};
}
[[nodiscard]] inline grid_t grid()
{
    auto id = si::detail::start_element(element_type::grid);
    // TODO
    return {id};
}
[[nodiscard]] inline row_t row()
{
    auto id = si::detail::start_element(element_type::row);
    // TODO
    return {id};
}

[[nodiscard]] inline canvas_t canvas()
{
    auto id = si::detail::start_element(element_type::canvas);
    // TODO
    return {id};
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
