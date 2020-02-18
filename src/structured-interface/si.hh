#pragma once

#include <clean-core/string_view.hh>
#include <clean-core/string.hh>

#include <typed-geometry/tg.hh>

namespace si
{
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
};
struct scoped_ui_element
{
    operator bool() const&& = delete; // make if (si::window(...)) a compile error
    // TODO: see ui_element
    operator bool() const& { return false; }
};

struct world_element
{
    // TODO: see ui_element
    operator bool() const { return false; }
};

template <class T>
struct input_t : ui_element
{
};
template <class T>
struct slider_t : ui_element
{
};
struct button_t : ui_element
{
};
struct checkbox_t : ui_element
{
};
struct toggle_t : ui_element
{
};
struct text_t : ui_element
{
};
template <class T>
struct radio_button_t : ui_element
{
};
template <class T>
struct dropdown_t : ui_element
{
};
template <class T>
struct listbox_t : ui_element
{
};
template <class T>
struct combobox_t : ui_element
{
};

struct window_t : scoped_ui_element
{
};
struct tree_node_t : scoped_ui_element
{
};

struct gizmo_t : world_element
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
button_t button(cc::string_view text)
{
    return {}; // TODO
}
checkbox_t checkbox(cc::string_view text, bool& ok)
{
    return {}; // TODO
}
toggle_t toggle(cc::string_view text, bool& ok)
{
    return {}; // TODO
}
text_t text(cc::string_view text)
{
    return {}; // TODO
}
template <class A, class... Args>
text_t text(char const* format, A const& firstArg, Args const&... otherArgs)
{
    return {}; // TODO
}
[[nodiscard]] radio_button_t<void> radio_button(cc::string_view text, bool active)
{
    return {}; // TODO
}
template <class T>
radio_button_t<T> radio_button(cc::string_view text, T& value, tg::dont_deduce<T> value_button)
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

[[nodiscard]] window_t window(cc::string_view title)
{
    return {}; // TODO
}
[[nodiscard]] tree_node_t tree_node(cc::string_view text)
{
    return {}; // TODO
}

gizmo_t gizmo(tg::pos3& pos) // translation gizmo
{
    return {}; // TODO
}

// ??
void spacing() {}
void separator() {}

}
