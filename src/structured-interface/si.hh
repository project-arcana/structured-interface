#pragma once

#include <clean-core/string.hh>
#include <clean-core/string_view.hh>

#include <typed-geometry/tg.hh>

#include <structured-interface/detail/record.hh>

// NOTE: this header includes all important user elements
//       (i.e. what is needed to create the UIs)

namespace si
{
template <class this_t>
struct ui_element
{
    // TODO: return true iff change
    // TODO: explicit or not? changed |= pattern relies on implicit
    // TOOD: only for those ui elements that can actually change / have useful feedback
    operator bool() const { return false; }

    bool was_clicked() const { return false; }
    bool is_hovered() const { return false; }
    bool is_pressed() const { return false; }
    // ...

    void tooltip(cc::string_view text)
    {
        // TODO
    }

    ui_element()
    {
        // TODO
        si::detail::start_element(0);
    }
    ~ui_element()
    {
        // TODO
        si::detail::end_element();
    }
    // non-copyable / non-movable
    ui_element(ui_element&&) = delete;
    ui_element(ui_element const&) = delete;
    ui_element& operator=(ui_element&&) = delete;
    ui_element& operator=(ui_element const&) = delete;
};
template <class this_t>
struct scoped_ui_element : ui_element<this_t>
{
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

struct gizmo_t : world_element<gizmo_t>
{
};

template <class T>
input_t<T> input(cc::string_view text, T& value)
{
    return {}; // TODO
}
template <class T>
slider_t<T> slider(cc::string_view text, T& value, tg::dont_deduce<T> const& min, tg::dont_deduce<T> const& max)
{
    return {}; // TODO
}
inline button_t button(cc::string_view text)
{
    return {}; // TODO
}
inline checkbox_t checkbox(cc::string_view text, bool& ok)
{
    return {}; // TODO
}
inline toggle_t toggle(cc::string_view text, bool& ok)
{
    return {}; // TODO
}
inline text_t text(cc::string_view text)
{
    return {}; // TODO
}
template <class A, class... Args>
text_t text(char const* format, A const& firstArg, Args const&... otherArgs)
{
    return {}; // TODO
}
[[nodiscard]] inline radio_button_t<void> radio_button(cc::string_view text, bool active)
{
    return {}; // TODO
}
template <class T>
radio_button_t<T> radio_button(cc::string_view text, T& value, tg::dont_deduce<T> option)
{
    return {}; // TODO
}
template <class T>
dropdown_t<T> dropdown(cc::string_view text, T& value, tg::dont_deduce<tg::span<T>> options)
{
    return {}; // TODO
}
template <class T>
dropdown_t<T> dropdown(cc::string_view text, T& value, tg::dont_deduce<tg::span<T>> options, tg::span<cc::string> names)
{
    return {}; // TODO
}
template <class T>
listbox_t<T> listbox(cc::string_view text, T& value, tg::dont_deduce<tg::span<T>> options)
{
    return {}; // TODO
}
template <class T>
listbox_t<T> listbox(cc::string_view text, T& value, tg::dont_deduce<tg::span<T>> options, tg::span<cc::string> names)
{
    return {}; // TODO
}
template <class T>
combobox_t<T> combobox(cc::string_view text, T& value, tg::dont_deduce<tg::span<T>> options)
{
    return {}; // TODO
}
template <class T>
combobox_t<T> combobox(cc::string_view text, T& value, tg::dont_deduce<tg::span<T>> options, tg::span<cc::string> names)
{
    return {}; // TODO
}

[[nodiscard]] inline window_t window(cc::string_view title)
{
    return {}; // TODO
}
[[nodiscard]] inline tree_node_t tree_node(cc::string_view text)
{
    return {}; // TODO
}
[[nodiscard]] inline tabs_t tabs()
{
    return {}; // TODO
}
[[nodiscard]] inline tab_t tab(cc::string_view title)
{
    return {}; // TODO
}
[[nodiscard]] inline flow_t flow()
{
    return {}; // TODO
}
[[nodiscard]] inline container_t container()
{
    return {}; // TODO
}
[[nodiscard]] inline grid_t grid()
{
    return {}; // TODO
}
[[nodiscard]] inline row_t row()
{
    return {}; // TODO
}

inline gizmo_t gizmo(tg::pos3& pos) // translation gizmo
{
    // TODO: dir3, vec3, size3 versions
    return {}; // TODO
}

// ??
inline void spacing() {}
inline void separator() {}

}
