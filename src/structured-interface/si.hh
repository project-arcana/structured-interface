#pragma once

#include <clean-core/string.hh>
#include <clean-core/string_view.hh>
#include <clean-core/type_id.hh>

#include <typed-geometry/tg.hh>

#include <structured-interface/detail/record.hh>
#include <structured-interface/element_type.hh>
#include <structured-interface/handles.hh>
#include <structured-interface/properties.hh>

// NOTE: this header includes all important user elements
//       (i.e. what is needed to create the UIs)

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

    bool was_clicked() const { return false; }
    bool is_hovered() const { return false; }
    bool is_pressed() const { return false; }
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

    operator bool() const&& = delete; // make if (si::window(...)) a compile error
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
};
struct checkbox_t : ui_element<checkbox_t>
{
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

template <class T>
input_t<T> input(cc::string_view text, T& value)
{
    auto id = si::detail::start_element(element_type::input, text);
    // TODO
    return {id};
}
template <class T>
slider_t<T> slider(cc::string_view text, T& value, tg::dont_deduce<T> const& min, tg::dont_deduce<T> const& max)
{
    auto id = si::detail::start_element(element_type::slider, text);
    // TODO
    return {id};
}
inline button_t button(cc::string_view text)
{
    auto id = si::detail::start_element(element_type::button, text);
    si::detail::write_property(id, si::property::text, text);
    return {id};
}
inline checkbox_t checkbox(cc::string_view text, bool& ok)
{
    auto id = si::detail::start_element(element_type::checkbox, text);
    // TODO
    return {id};
}
inline toggle_t toggle(cc::string_view text, bool& ok)
{
    auto id = si::detail::start_element(element_type::toggle, text);
    // TODO
    return {id};
}
inline text_t text(cc::string_view text)
{
    auto id = si::detail::start_element(element_type::text, text);
    // TODO
    return {id};
}
template <class A, class... Args>
text_t text(char const* format, A const& firstArg, Args const&... otherArgs)
{
    auto id = si::detail::start_element(element_type::text, format);
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
template <class T>
dropdown_t<T> dropdown(cc::string_view text, T& value, tg::dont_deduce<tg::span<T>> options, tg::span<cc::string> names)
{
    auto id = si::detail::start_element(element_type::dropdown, text);
    // TODO
    return {id};
}
template <class T>
listbox_t<T> listbox(cc::string_view text, T& value, tg::dont_deduce<tg::span<T>> options)
{
    auto id = si::detail::start_element(element_type::listbox, text);
    // TODO
    return {id};
}
template <class T>
listbox_t<T> listbox(cc::string_view text, T& value, tg::dont_deduce<tg::span<T>> options, tg::span<cc::string> names)
{
    auto id = si::detail::start_element(element_type::listbox, text);
    // TODO
    return {id};
}
template <class T>
combobox_t<T> combobox(cc::string_view text, T& value, tg::dont_deduce<tg::span<T>> options)
{
    auto id = si::detail::start_element(element_type::combobox, text);
    // TODO
    return {id};
}
template <class T>
combobox_t<T> combobox(cc::string_view text, T& value, tg::dont_deduce<tg::span<T>> options, tg::span<cc::string> names)
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
