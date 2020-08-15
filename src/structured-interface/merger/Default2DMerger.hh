#pragma once

#include <cstdint>

#include <clean-core/string.hh>
#include <clean-core/string_view.hh>
#include <clean-core/vector.hh>

#include <typed-geometry/tg-lean.hh>

#include <structured-interface/anchor.hh>
#include <structured-interface/fwd.hh>
#include <structured-interface/handles.hh>
#include <structured-interface/merger/StyleSheet.hh>
#include <structured-interface/merger/editable_text.hh>
#include <structured-interface/style.hh>

namespace si
{
/**
 * A simple UI merger for aabb-based 2D UIs
 *
 * Does:
 *   - AABB based layouting
 *   - simple font handling
 *   - simple input handling
 *   - render call generation
 *
 * Usage:
 *
 *   Default2DMerger merger;
 *   gui.update(recorded_ui, merger);
 *
 * Notes:
 *
 *   - the coordinate system is in pixels
 */
class Default2DMerger
{
    struct layouted_element;

    // settings
public:
    tg::aabb2 viewport = {{0, 0}, {1920, 1080}};
    double total_time = 0;

    // input test!
public:
    tg::pos2 mouse_pos;
    bool is_lmb_down = false;

private:
    tg::pos2 prev_mouse_pos;
    bool was_lmb_down = false;
    float drag_distance = 0;

    // ctor
public:
    Default2DMerger();

    // font data
public:
    struct glyph_info
    {
        tg::aabb2 uv; // in atlas (0..1)
        // see https://www.freetype.org/freetype2/docs/glyphs/glyphs-3.html
        // NOTE: in px coordinates (when using ref_size)
        float bearingX = 0;
        float bearingY = 0;
        float width = 0;
        float height = 0;
        float advance = 0; // TODO: maybe separate so text_bounds is faster?
    };
    struct font_atlas
    {
        cc::vector<std::byte> data;
        int width = 0;
        int height = 0;
        float ref_size = 0;

        // NOTE: in px coordinates (when using ref_size)
        float ascender = 0;
        float baseline_height = 0;

        // coord in the center of a 2x2 full pixel
        tg::pos2 full_uv;

        // currently only suitable for ASCII
        cc::vector<glyph_info> glyphs;
    };

    font_atlas const& get_font_atlas() const { return _font; }

    // render data
    // (inspired by imgui)
public:
    struct vertex
    {
        tg::pos2 pos; // in pixels
        tg::pos2 uv;  // in pixels?
        uint32_t color = 0xFFFFFFFF;
    };
    struct draw_cmd
    {
        uint64_t texture_handle = 0;
        uint32_t indices_start = 0;
        uint32_t indices_count = 0;
    };
    struct render_list
    {
        cc::vector<vertex> vertices;
        cc::vector<int> indices;
        cc::vector<draw_cmd> cmds;
    };
    struct render_data
    {
        cc::vector<render_list> lists;
    };

    render_data const& get_render_data() const { return _render_data; }

public:
    /// performs the merge operation (called in gui::update)
    element_tree operator()(element_tree const&, element_tree&& new_ui, input_state& input);

    /// builds a si:: ui showing internal stats
    void show_stats_ui(bool use_window = true);
    /// builds a si:: ui for inspecting si uis
    /// previous element_tree must be passed
    void show_inspector_ui(si::element_tree const& ui);

    /// used in stats::ui for displaying time for user recording
    void set_record_timings(double seconds) { _seconds_record = seconds; }

    // private helper
private:
    void load_default_font();

    void emit_warning(element_handle id, cc::string_view msg);

    void set_editable_text_glyphs(cc::string_view txt, float x, float y, style::font const& font);
    tg::aabb2 get_text_bounds(cc::string_view txt, float x, float y, style::font const& font);
    void add_text_render_data(render_list& rl, cc::string_view txt, float x, float y, style::font const& font, tg::aabb2 const& clip, size_t selection_start, size_t selection_count);

    // styling
private:
    /// entry point for allocating layouted_element and computing style of a single element
    void compute_style(si::element_tree& tree,
                       si::element_tree_element& e,
                       int layout_idx,
                       StyleSheet::computed_style const& parent_style,
                       int child_idx,
                       int child_cnt,
                       cc::span<element_handle> hover_stack);
    /// recursive helper for computing style of children
    void compute_child_style(si::element_tree& tree,
                             int parent_layout_idx,
                             cc::span<si::element_tree_element> elements,
                             StyleSheet::computed_style const& style,
                             cc::span<element_handle> hover_stack);

    // layouting
private:
    /// entry point for layouting a single element
    /// NOTE: collapsible_margin is from point of view of this element
    cc::pair<tg::aabb2, style::margin> perform_layout(si::element_tree& tree, int layout_idx, float x, float y, tg::aabb2 const& parent_bb, style::margin const& collapsible_margin);

    /// helper for applying the default child layouting
    /// returns a tight, non-padded bounding box
    /// does NOT include absolutely positioned elements in its return value
    /// NOTE: parent_layout_idx can be -1 for original root elements
    tg::aabb2 perform_child_layout_relative(
        si::element_tree& tree, int parent_layout_idx, StyleSheet::computed_style const& parent_style, float x, float y, tg::aabb2 const& parent_bb);
    /// second pass of child layouts where parent width is computed if auto-sized
    /// x,y is parent start (without border / padding)
    void perform_child_layout_absolute(si::element_tree& tree, int parent_layout_idx, StyleSheet::computed_style const& parent_style, float x, float y, tg::aabb2 const& parent_bb);

    /// helper for adding a child to the layout tree
    int add_child_layout_element(int parent_idx);

    /// moves a layout element (and all children) to a given position
    /// afterwards, bounds.min += delta
    void move_layout(si::element_tree& tree, int layout_idx, tg::vec2 delta);

    /// returns the topmost input-receiving element at the given position
    /// returns nullptr if nothing hit
    /// (uses _layout_tree)
    si::element_tree_element* query_input_element_at(tg::pos2 p) const;
    /// implementation helper for query_input_element_at
    si::element_tree_element* query_input_child_element_at(int layout_idx, tg::pos2 p) const;
    /// same as query_input_element_at but returns a layouted_element
    /// and can also return any element, not only input receiving ones
    layouted_element const* query_layout_element_at(tg::pos2 p) const;
    layouted_element const* query_child_layout_element_at(int layout_idx, tg::pos2 p) const;

    // render methods
    // note: clip is already clipped to element aabb
private:
    /// entry point for collecting render data
    void build_render_data(si::element_tree const& tree, layouted_element const& le, tg::aabb2 clip);

    // special elements
    void render_text(si::element_tree const& tree, layouted_element const& le, tg::aabb2 const& clip, size_t selection_start = 0, size_t selection_count = 0); // NOTE: requires text and text_origin properties

    // NOTE: end is exclusive
    void render_child_range(si::element_tree const& tree, int range_start, int range_end, tg::aabb2 const& clip);

    // private member
private:
    font_atlas _font;
    render_data _render_data;
    StyleSheet _stylesheet;

    // tmp external data
private:
    si::element_tree const* _prev_ui = nullptr;
    input_state* _input = nullptr;

    // text edit
    // TODO: move to own struct
public:
    bool is_in_text_edit() const { return _is_in_text_edit; }
    merger::editable_text& editable_text() { return _editable_text; }
    merger::editable_text const& editable_text() const { return _editable_text; }

private:
    bool _is_in_text_edit = false;
    merger::editable_text _editable_text;

    // layout tree
private:
    /**
     * a layouted element is part of a derived UI tree
     *
     * this tree has the correct element draw order (back-to-front)
     * and is also used for input handling.
     *
     * NOTE:
     *  - the layout tree can contain holes (where detached elements are moved from)
     *    (this allows proper pre-allocation)
     *  - contains some duplicated properties for faster access
     */
    struct layouted_element
    {
        si::element_tree_element* element = nullptr;
        tg::aabb2 bounds; ///< including border and padding, excluding margin
        tg::pos2 content_start;
        StyleSheet::computed_style style;
        // TODO: total_bounds that can be larger for when children move out of inner bounds (e.g. menus)
        // TODO: total_bounds not needed because menus are separate, detached things
        // TODO: text layout cache (like cached glyphs)
        // TODO: local coords, mat2 transformation, polar coords
        tg::pos2 text_origin;
        int child_start = 0;
        int child_count = 0;
        bool no_input = false;        // ignores input
        bool is_in_text_edit = false; // for showing the cursor and selection
    };

    // TODO: maybe make roots sortable for different layers (e.g. tooltips > popovers)

    cc::vector<layouted_element> _layout_tree;
    cc::vector<int> _layout_roots;          // points into layout_tree
    cc::vector<int> _layout_detached_roots; // points into layout_tree
    int _layout_original_roots = 0;

    /**
     * "jobs" for resolving constraints (e.g. tooltip positions)
     */
    struct deferred_placement
    {
        si::placement placement;
        int layout_idx_ref;
        int layout_idx_this;
    };
    cc::vector<deferred_placement> _deferred_placements;

    /// e.g. detached elements can have placement constraints that require accurate resolution
    void resolve_deferred_placements(si::element_tree& tree);

    // temp helper
private:
    struct window_index
    {
        si::element_tree_element* window;
        int idx;

        bool operator<(window_index const& rhs) const { return idx < rhs.idx; }
    };
    cc::vector<window_index> _tmp_windows; ///< for sorting them

    cc::vector<StyleSheet::style_key> _style_stack_keys;

    // stats
private:
    double _seconds_record = 0;
    double _seconds_style = 0;
    double _seconds_layout = 0;
    double _seconds_input = 0;
    double _seconds_render_data = 0;
};
}
