#pragma once

// TODO: can probably be replaced by a fwd decl
#include <clean-core/string.hh>

#include <clean-core/format.hh>
#include <clean-core/function_ref.hh>
#include <clean-core/pair.hh>
#include <clean-core/string_view.hh>
#include <clean-core/to_string.hh>
#include <clean-core/type_id.hh>

// for canvas
#include <clean-core/vector.hh>

#include <typed-geometry/tg-lean.hh>

#include <structured-interface/anchor.hh>
#include <structured-interface/detail/record.hh>
#include <structured-interface/detail/ui_context.hh>
#include <structured-interface/element_type.hh>
#include <structured-interface/handles.hh>
#include <structured-interface/input_state.hh>
#include <structured-interface/options.hh>
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
    /// can be invalid for conditional elements like a tooltip
    element_handle id;

    bool was_clicked() const { return detail::current_input_state().was_clicked(id); }
    bool is_hovered() const { return detail::current_input_state().is_hovered(id); }
    bool is_hover_entered() const { return detail::current_input_state().is_hover_entered(id); }
    bool is_hover_left() const { return detail::current_input_state().is_hover_left(id); }
    bool is_direct_hovered() const { return detail::current_input_state().is_direct_hovered(id); }
    bool is_direct_hover_entered() const { return detail::current_input_state().is_direct_hover_entered(id); }
    bool is_direct_hover_left() const { return detail::current_input_state().is_direct_hover_left(id); }
    bool is_pressed() const { return detail::current_input_state().is_pressed(id); }
    bool is_focused() const { return detail::current_input_state().is_focused(id); }
    bool is_focus_gained() const { return detail::current_input_state().is_focus_gained(id); }
    bool is_focus_lost() const { return detail::current_input_state().is_focus_lost(id); }
    bool is_dragged() const { return detail::current_input_state().is_dragged(id); }
    // ...

    /// if true, id is valid and can be used to query state
    bool is_valid_element() const { return id.is_valid(); }
    /// if true, new si::elements are not children of this element
    bool is_finished_element() const { return _is_finished; }

    // styling
public:
    /// sets the stylesheet class for this element
    /// NOTE: these IDs can be created by StyleSheet::add_or_get_class
    void set_style_class(uint16_t class_id);

    void set_left(float px);
    void set_left_relative(float v);
    void set_top(float px);
    void set_top_relative(float v);
    void set_width(float px);
    void set_width_relative(float v);
    void set_height(float px);
    void set_height_relative(float v);

    // advanced api
public:
    /// [advanced usage]
    /// manually closes an element
    /// (so new elements are not inside this one even if not out of scope yet)
    void finish_element()
    {
        if (!_is_finished)
            si::detail::end_element(id);
        _is_finished = true;
    }

    // special members
public:
    ui_element_base(element_handle id) : id(id) { _is_finished = !id.is_valid(); }
    ~ui_element_base() { finish_element(); }

    // non-copyable / non-movable
    ui_element_base(ui_element_base&&) = delete;
    ui_element_base(ui_element_base const&) = delete;
    ui_element_base& operator=(ui_element_base&&) = delete;
    ui_element_base& operator=(ui_element_base const&) = delete;

protected:
    bool _is_finished = false;
};
template <class this_t>
struct ui_element : ui_element_base
{
    using ui_element_base::ui_element_base;

    /// creates a simple text tooltip when this element is hovered over
    /// more sophisticated tooltips can be create via other overloads or si::tooltip
    this_t& tooltip(cc::string_view text, placement placement = placement::tooltip_default());

    /// creates a tooltip on hover and executes on_tooltip inside it
    /// usage:
    ///
    ///   si::text("complex tooltip").tooltip([&]{
    ///       si::text("tooltip text");
    ///       si::text(".. and other stuff");
    ///   }, si::placement::centered_above());
    this_t& tooltip(cc::function_ref<void()> on_tooltip, placement placement = placement::tooltip_default());

    /// creates a simple text popover when this element is clicked
    /// more sophisticated popovers can be create via other overloads or si::popover
    this_t& popover(cc::string_view text, placement placement = placement::popover_default());

    /// creates a popover on click and executes on_popover inside of it while it is open
    /// usage:
    ///
    ///   si::text("complex popover").popover([&]{
    ///       si::text("popover text");
    ///       si::text(".. and other stuff");
    ///   }, si::placement::centered_above());
    this_t& popover(cc::function_ref<void()> on_popover, placement placement = placement::popover_default());
};
template <class this_t>
struct scoped_ui_element : ui_element<this_t>
{
    scoped_ui_element(element_handle id, bool children_visible) : ui_element<this_t>(id), _children_visible(children_visible) {}

    // explicit operator bool() const&& = delete; // make "if (si::window(...))" a compile error
    // explicit operator bool() const& { return _children_visible; }

    // TODO: msvc thinks the const&& triggers an error
    explicit operator bool() const { return _children_visible; }

private:
    bool _children_visible = false;
};

template <class this_t>
struct world_element
{
    // TODO: see ui_element
    operator bool() const { return false; }
};

// TODO: use this for si::text etc.
struct id_text
{
    element_handle id;
    cc::string_view text;

    id_text(cc::string_view text) : text(text) {}
};

template <class T>
struct input_t : ui_element<input_t<T>>
{
};
template <class T>
struct slider_t : ui_element<slider_t<T>>
{
    slider_t(element_handle id, bool changed) : ui_element<slider_t<T>>(id), _changed(changed) {}
    bool was_changed() const { return _changed; }
    operator bool() const { return _changed; }

private:
    bool _changed = false;
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
    toggle_t(element_handle id, bool changed) : ui_element(id), _changed(changed) {}
    bool was_changed() const { return _changed; }
    operator bool() const { return _changed; }

private:
    bool _changed = false;
};
struct text_t : ui_element<text_t>
{
};
struct separator_t : ui_element<separator_t>
{
};
struct spacing_t : ui_element<spacing_t>
{
};
struct heading_t : ui_element<heading_t>
{
};
struct textbox_t : ui_element<textbox_t>
{
    textbox_t(element_handle id, bool changed) : ui_element(id), _changed(changed) {}
    bool was_changed() const { return _changed; }
    operator bool() const { return _changed; }

private:
    bool _changed = false;
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

struct box_t : scoped_ui_element<box_t>
{
    using scoped_ui_element<box_t>::scoped_ui_element;
};
struct collapsible_group_t : scoped_ui_element<collapsible_group_t>
{
    using scoped_ui_element<collapsible_group_t>::scoped_ui_element;
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
struct tooltip_t : scoped_ui_element<tooltip_t>
{
    using scoped_ui_element<tooltip_t>::scoped_ui_element;
};
struct popover_t : scoped_ui_element<popover_t>
{
    using scoped_ui_element<popover_t>::scoped_ui_element;
};
struct scroll_area_t : scoped_ui_element<scroll_area_t>
{
    scroll_area_t(element_handle id, element_handle box_id) : scoped_ui_element<scroll_area_t>(id, true), inner_box(box_id, true) {}

private:
    box_t inner_box;
};

/**
 * Canvas API
 * custom vector graphics in si
 *
 * TODO: maybe move in seperate header if too expensive?
 * TODO: currently implemented via triangulation, might get custom shader in the future?
 * TODO: either sbo_vector or some external allocator?
 */
struct canvas_t : scoped_ui_element<canvas_t>
{
    using scoped_ui_element<canvas_t>::scoped_ui_element;

    ~canvas_t();

    // structs
    // TODO: find a better place for them
public:
    struct fill_style
    {
        tg::color4 color;
        fill_style() = default;
        fill_style(tg::color3 c) : color(c, 1.f) {}
        fill_style(tg::color4 c) : color(c) {}
    };

    // fill API
public:
    void fill(tg::triangle2 const& t, fill_style const& s)
    {
        _triangles.push_back({t.pos0, s.color});
        _triangles.push_back({t.pos1, s.color});
        _triangles.push_back({t.pos2, s.color});
    }
    void fill(tg::aabb2 const& bb, fill_style const& s)
    {
        _triangles.push_back({{bb.min.x, bb.min.y}, s.color});
        _triangles.push_back({{bb.max.x, bb.min.y}, s.color});
        _triangles.push_back({{bb.max.x, bb.max.y}, s.color});

        _triangles.push_back({{bb.min.x, bb.min.y}, s.color});
        _triangles.push_back({{bb.max.x, bb.max.y}, s.color});
        _triangles.push_back({{bb.min.x, bb.max.y}, s.color});
    }
    void fill_triangle(tg::pos2 p0, tg::pos2 p1, tg::pos2 p2, tg::color3 c0, tg::color3 c1, tg::color3 c2)
    {
        _triangles.push_back({p0, c0});
        _triangles.push_back({p1, c1});
        _triangles.push_back({p2, c2});
    }

private:
    // TODO: maybe with indices?
    cc::vector<colored_vertex> _triangles;
};

struct gizmo_t : world_element<gizmo_t>
{
};

// helper
struct id_scope_t
{
    explicit id_scope_t(cc::hash_t seed)
    {
        auto& s = si::detail::id_seed();
        prev_seed = s;
        s = seed;
    }
    ~id_scope_t() { si::detail::id_seed() = prev_seed; }

private:
    cc::hash_t prev_seed;

    id_scope_t(id_scope_t const&) = delete;
    id_scope_t(id_scope_t&&) = delete;
    id_scope_t& operator=(id_scope_t const&) = delete;
    id_scope_t& operator=(id_scope_t&&) = delete;
};

namespace detail
{
cc::pair<element_handle, bool> impl_radio_button(cc::string_view text, bool active);
}


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
 * shows a heading-styled line of text
 *
 * usage:
 *
 *   si::heading("hello world");
 */
heading_t heading(cc::string_view text);

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
 * shortcut for a "name: value" text (uses cc::format)
 *
 * usage:
 *
 *   si::value("my var", my_var);
 */
template <class T>
text_t value(cc::string_view name, T const& value)
{
    return text("{}: {}", name, value);
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
 *
 * DOM notes:
 *   - contains a single [box] element that can be used to style the checkbox
 */
checkbox_t checkbox(cc::string_view text, bool& ok);

/**
 * creates a radio button with description text
 * can be cast to bool to see if value was changed
 * 'active' signals the current state of the radio button
 *
 * usage:
 *
 *   int my_int = ...;
 *   if (si::radio_button("my int = 3", my_int == 3))
 *       my_int = 3;
 *
 * DOM notes:
 *   - contains a single [box] element that can be used to style the radio_button
 */
[[nodiscard]] inline radio_button_t<void> radio_button(cc::string_view text, bool active)
{
    auto [id, changed] = detail::impl_radio_button(text, active);
    return {id, changed};
}

/**
 * creates a radio button with description text
 * can be cast to bool to see if value was changed
 * when clicked, sets the provided value to 'option'
 *
 * usage:
 *
 *   int my_int = ...;
 *   si::radio_button("my int = 3", my_int, 3));
 *   si::radio_button("my int = 4", my_int, 4));
 *   si::radio_button("my int = 5", my_int, 5));
 *
 * DOM notes:
 *   - contains a single [box] element that can be used to style the radio_button
 */
template <class T>
radio_button_t<T> radio_button(cc::string_view text, T& value, tg::dont_deduce<T const&> option)
{
    auto [id, changed] = detail::impl_radio_button(text, value == option);
    if (changed)
        value = option;
    return {id, changed};
}

/**
 * creates a toggle with description text
 * when clicked, toggles the value of "ok"
 * can be cast to bool to see if value was changed
 *
 * usage:
 *
 *   bool value = ...;
 *   changed |= si::toggle("bool value", value);
 *
 * DOM notes:
 *   - contains a single [box] element that can be used to style the toggle
 */
toggle_t toggle(cc::string_view text, bool& ok);

/**
 * creates a single line editable text box with description text
 * can be cast to bool to see if value was changed
 *
 * usage:
 *
 *   cc::string value = ...;
 *   changed |= si::textbox("some string", value);
 */
textbox_t textbox(cc::string_view desc, cc::string& value);

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
 *
 * DOM notes:
 *   - contains a single [box] for the knob (left is set to "t %")
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
 *   - contains a single [slider_area] with text parameter as child
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

    return {id, slider.was_changed()};
}

/**
 * creates a group with a heading that can be clicked to collapse or uncollapse the group
 * can be cast to bool to check if header is collapsed or not
 *
 * usage:
 *
 *   if (auto h = si::collapsible_group("section A"))
 *   {
 *       // .. child elements
 *   }
 *
 * DOM notes:
 *   - first child is a si::heading with the given text
 */
collapsible_group_t collapsible_group(cc::string_view text, cc::flags<collapsible_group_options> options = cc::no_flags);

/**
 * creates a simple box element
 * NOTE: this is per default unstyled
 * TODO: id?
 * can be cast to bool for "if (auto b = si::box())" pattern
 *
 * usage:
 *
 *   // without children
 *   si::box();
 *
 *   // with children
 *   if (auto b = si::box())
 *   {
 *       // .. child elements
 *   }
 */
box_t box();

/**
 * creates a horizontal separator
 * TODO: id?
 * TODO: vertical if in row?
 *
 * usage:
 *
 *   si::separator();
 */
separator_t separator();

/**
 * creates a vertical spacing
 * TODO: id?
 * TODO: horizontal if in row?
 *
 * usage:
 *
 *   si::spacing();
 */
spacing_t spacing(float size = 8.f);

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
[[nodiscard]] window_t window(cc::string_view title, bool& visible);

/**
 * creates a tooltip that is shown when the parent is hovered over
 * cast to bool is used to determine if content is visible
 * TODO: add anchor parameter
 * TODO: add force-open parameter
 *
 * usage:
 *
 *   if (auto tt = si::tooltip())
 *   {
 *       // .. child elements (shown in tooltip)
 *   }
 *
 * short versions:
 *
 *   si::text("hover me").tooltip("I'm a tooltip!");
 *   si::text("hover me").tooltip([&] { ... any si element ... });
 */
[[nodiscard]] tooltip_t tooltip(placement placement = placement::tooltip_default());

/**
 * creates a popover that is shown when the parent is clicked
 * cast to bool is used to determine if content is visible
 * TODO: add anchor parameter
 * TODO: add force-open parameter
 * TODO: add option so that popover closes on second click, not on unfocus
 * NOTE: don't confuse with popups, which are separate elements
 *
 * usage:
 *
 *   if (auto pp = si::popover())
 *   {
 *       // .. child elements (shown in popover)
 *   }
 *
 * short versions:
 *
 *   si::text("click me").popover("I'm a popover!");
 *   si::text("click me").popover([&] { ... any si element ... });
 */
[[nodiscard]] popover_t popover(placement placement = placement::tooltip_default());

/**
 * all elements inside this one are layouted left-to-right (forming a single row)
 * can be cast to bool for "if (auto r = si::row())" pattern (always true)
 * TODO: usage inside grids / tables
 * TODO: custom id scope?
 *
 * usage:
 *
 *   if (auto r = si::row())
 *   {
 *       si::text("text on");
 *       si::text("same row");
 *   }
 *
 * NOTE: currently multiple rows in the same scope have the same id
 */
[[nodiscard]] row_t row();

/**
 * the content of this element is scrollable (when it exceeds the parent bounds)
 *
 * usage:
 *
 *   if (auto s = si::scroll_area())
 *   {
 *       .. really long and big content
 *   }
 *
 * NOTE: scroll bars must be added separately
 * NOTE: currently multiple scroll_areas in the same scope have the same id
 *
 * DOM notes:
 *   - contains a [box] that is moved during scrolling
 */
[[nodiscard]] scroll_area_t scroll_area();

/**
 * creates a canvas object that can be used for 2D vector graphics
 * see canvas_t for its API
 * can be cast to bool for "if (auto c = si::canvas())" pattern (always true)
 * NOTE: currently requires a fixed size in pixels
 * TODO: custom id?
 *
 * usage:
 *
 *   if (auto c = si::canvas())
 *   {
 *       c.draw(tg::segment2({10, 10}, {100, 40}), {2.f, tg::color3::red});
 *       c.fill(tg::circle2({30, 20}, 7.5f), tg::color3::blue);
 *   }
 */
[[nodiscard]] canvas_t canvas(tg::size2 size);

/**
 * creates a scope with a new ID prefix.
 * any elements in this scope will not conflict with elements of other scopes
 *
 * usage:
 *
 *   {
 *       auto _ = si::id_scope(1, true);
 *       si::slider("slider", some_var, 0, 10);
 *   }
 *   {
 *       auto _ = si::id_scope(234, 'c', some_ptr);
 *       si::slider("slider", some_var, 0, 10); // will not conflict with previous slider
 *   }
 */
template <class... Values>
[[nodiscard]] id_scope_t id_scope(Values const&... values)
{
    static_assert(sizeof...(values) > 0, "must provide at least one value to hash");
    return id_scope_t(si::detail::make_hash(si::detail::id_seed(), values...));
}


// =======================================
//
// deferred implementation
//

template <class this_t>
this_t& ui_element<this_t>::tooltip(cc::string_view text, placement placement)
{
    if (auto tt = si::tooltip(placement))
        si::detail::write_property(tt.id, si::property::text, text);
    return static_cast<this_t&>(*this);
}
template <class this_t>
this_t& ui_element<this_t>::tooltip(cc::function_ref<void()> on_tooltip, placement placement)
{
    if (auto tt = si::tooltip(placement))
        on_tooltip();
    return static_cast<this_t&>(*this);
}

template <class this_t>
this_t& ui_element<this_t>::popover(cc::string_view text, placement placement)
{
    if (auto pp = si::popover(placement))
        si::detail::write_property(pp.id, si::property::text, text);
    return static_cast<this_t&>(*this);
}
template <class this_t>
this_t& ui_element<this_t>::popover(cc::function_ref<void()> on_popover, placement placement)
{
    if (auto pp = si::popover(placement))
        on_popover();
    return static_cast<this_t&>(*this);
}


// =======================================
//
//  not implemented:

// TODO:
// si::modal
// si::alert
// si::popup? (popover is extended tooltip)
// si::notify / notification?
// si::progressbar
// si::spinner
// si::icon
// si::button_group?
// h1,h2,h3,... ?
// si::paragraph?

template <class T>
input_t<T> input(cc::string_view text, T& value)
{
    auto id = si::detail::start_element(element_type::input, text);
    (void)value;
    // TODO
    // TODO: textbox if value is string
    return {id};
}

template <class T>
dropdown_t<T> dropdown(cc::string_view text, T& value, tg::dont_deduce<tg::span<T>> options)
{
    auto id = si::detail::start_element(element_type::dropdown, text);
    (void)value;
    (void)options;
    // TODO
    return {id};
}
template <class T, class OptionsT>
dropdown_t<T> dropdown(cc::string_view text, T& value, OptionsT const& options, tg::span<cc::string> names)
{
    auto id = si::detail::start_element(element_type::dropdown, text);
    (void)value;
    (void)options;
    (void)names;
    // TODO
    return {id};
}
template <class T, class OptionsT>
listbox_t<T> listbox(cc::string_view text, T& value, OptionsT const& options)
{
    auto id = si::detail::start_element(element_type::listbox, text);
    (void)value;
    (void)options;
    // TODO
    return {id};
}
template <class T, class OptionsT>
listbox_t<T> listbox(cc::string_view text, T& value, OptionsT const& options, tg::span<cc::string> names)
{
    auto id = si::detail::start_element(element_type::listbox, text);
    (void)value;
    (void)options;
    (void)names;
    // TODO
    return {id};
}
template <class T, class OptionsT>
combobox_t<T> combobox(cc::string_view text, T& value, OptionsT const& options)
{
    auto id = si::detail::start_element(element_type::combobox, text);
    (void)value;
    (void)options;
    // TODO
    return {id};
}
template <class T, class OptionsT>
combobox_t<T> combobox(cc::string_view text, T& value, OptionsT const& options, tg::span<cc::string> names)
{
    auto id = si::detail::start_element(element_type::combobox, text);
    (void)value;
    (void)options;
    (void)names;
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

inline gizmo_t gizmo(tg::pos3& pos) // translation gizmo
{
    (void)pos;
    // TODO: dir3, vec3, size3 versions
    return {}; // TODO
}

}
