#pragma once

#include <cstdint>

#include <clean-core/string.hh>
#include <clean-core/string_view.hh>
#include <clean-core/vector.hh>

#include <typed-geometry/tg-lean.hh>

#include <structured-interface/anchor.hh>
#include <structured-interface/fwd.hh>
#include <structured-interface/merger/StyleSheet.hh>
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
    void show_inspector_ui();

    // private helper
private:
    void load_default_font();

    tg::aabb2 get_text_bounds(cc::string_view txt, float x, float y, style::font const& font);
    void add_text_render_data(render_list& rl, cc::string_view txt, float x, float y, style::font const& font, tg::aabb2 const& clip);

    // layouting
private:
    /// entry point for layouting a single element
    /// NOTE: collapsible_margin is from point of view of this element
    cc::pair<tg::aabb2, style::margin> perform_layout(si::element_tree& tree,
                                                      si::element_tree_element& e,
                                                      int layout_idx,
                                                      float x,
                                                      float y,
                                                      StyleSheet::computed_style const& parent_style,
                                                      int child_idx,
                                                      int child_cnt,
                                                      style::margin const& collapsible_margin);

    // special elements
    // returns max position of children
    // NOTE: input x,y is already at the inner element (after margin, border, padding)
    tg::pos2 perform_checkbox_layout(si::element_tree& tree, si::element_tree_element& e, int layout_idx, float x, float y, StyleSheet::computed_style const& style);
    tg::pos2 perform_slider_layout(si::element_tree& tree, si::element_tree_element& e, int layout_idx, float x, float y, StyleSheet::computed_style const& style);
    tg::pos2 perform_window_layout(si::element_tree& tree, si::element_tree_element& e, int layout_idx, float x, float y, StyleSheet::computed_style const& style);

    /// helper for applying the default child layouting
    /// returns a tight, non-padded bounding box
    /// does NOT include absolutely positioned elements in its return value
    /// NOTE: parent_layout_idx can be -1 for original root elements
    tg::aabb2 perform_child_layout_default(
        si::element_tree& tree, int parent_layout_idx, cc::span<si::element_tree_element> elements, float x, float y, StyleSheet::computed_style const& style);

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
    void render_text(si::element_tree const& tree, layouted_element const& le, tg::aabb2 const& clip); // NOTE: requires text and text_origin properties
    void render_checkbox(si::element_tree const& tree, layouted_element const& le, tg::aabb2 const& clip);
    void render_slider(si::element_tree const& tree, layouted_element const& le, tg::aabb2 const& clip);
    void render_window(si::element_tree const& tree, layouted_element const& le, tg::aabb2 const& clip);

    // NOTE: end is exclusive
    void render_child_range(si::element_tree const& tree, int range_start, int range_end, tg::aabb2 const& clip);

    // private member
private:
    font_atlas _font;
    render_data _render_data;
    StyleSheet _style_cache;

    // tmp external data
private:
    si::element_tree const* _prev_ui = nullptr;
    input_state* _input = nullptr;


    // text edit
    // TODO: move to own struct
public:
    bool is_in_text_edit() const { return _is_in_text_edit; }
    void text_edit_add_char(char c);
    void text_edit_backspace();
    void text_edit_entf();

private:
    bool _is_in_text_edit = false;
    cc::string _editable_text;

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
        style::border border;
        style::background bg;
        style::font font;
        // TODO: total_bounds that can be larger for when children move out of inner bounds (e.g. menus)
        // TODO: text layout cache (like cached glyphs)
        // TODO: local coords, mat2 transformation, polar coords
        tg::pos2 text_origin;
        int child_start = 0;
        int child_count = 0;
        bool no_input = false;  // ignores input
        bool is_visible = true; // is rendered
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
};
}
