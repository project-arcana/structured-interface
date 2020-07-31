#pragma once

#include <cstdint>

#include <clean-core/string_view.hh>
#include <clean-core/vector.hh>

#include <typed-geometry/tg-lean.hh>

#include <structured-interface/anchor.hh>
#include <structured-interface/fwd.hh>

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
 *   Simple2DMerger merger;
 *   gui.update(recorded_ui, merger);
 *
 * Notes:
 *
 *   - the coordinate system is in pixels
 */
class Simple2DMerger
{
    struct layouted_element;

    // settings
public:
    float padding_left = 4.0f;
    float padding_top = 4.0f;
    float padding_right = 4.0f;
    float padding_bottom = 4.0f;
    float padding_child = 2.0f;
    tg::color3 font_color = tg::color3::black;
    float font_size = 20.f;
    tg::aabb2 viewport = {{0, 0}, {1920, 1080}};

    // input test!
public:
    tg::pos2 mouse_pos;
    bool is_lmb_down = false;

private:
    tg::pos2 prev_mouse_pos;
    float drag_distance = 0;

    // ctor
public:
    Simple2DMerger();

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

    // private helper
private:
    void load_default_font();

    tg::aabb2 get_text_bounds(cc::string_view txt, float x, float y);
    void add_text_render_data(render_list& rl, cc::string_view txt, float x, float y, tg::aabb2 const& clip);

    // layouting
private:
    enum class layout_algo
    {
        top_down,
        left_right
    };

    /// entry point for layouting a single element
    tg::aabb2 perform_layout(si::element_tree& tree, si::element_tree_element& e, int layout_idx, float x, float y);

    // special elements
    tg::aabb2 perform_checkbox_layout(si::element_tree& tree, si::element_tree_element& e, int layout_idx, float x, float y);
    tg::aabb2 perform_slider_layout(si::element_tree& tree, si::element_tree_element& e, int layout_idx, float x, float y);
    tg::aabb2 perform_window_layout(si::element_tree& tree, si::element_tree_element& e, int layout_idx, float x, float y);

    /// helper for applying the default child layouting
    /// returns a tight, non-padded bounding box
    /// does NOT include absolutely positioned elements in its return value
    /// NOTE: parent_layout_idx can be -1 for original root elements
    tg::aabb2 perform_child_layout_default(si::element_tree& tree, int parent_layout_idx, cc::span<si::element_tree_element> elements, float x, float y, layout_algo lalgo);

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

    // render methods
    // note: clip is already clipped to element aabb
private:
    /// entry point for collecting render data
    void build_render_data(si::element_tree const& tree, layouted_element const& le, input_state const& input, tg::aabb2 clip);

    // special elements
    void render_text(si::element_tree const& tree, layouted_element const& le, tg::aabb2 const& clip); // NOTE: requires text and text_origin properties
    void render_checkbox(si::element_tree const& tree, layouted_element const& le, input_state const& input, tg::aabb2 const& clip);
    void render_slider(si::element_tree const& tree, layouted_element const& le, input_state const& input, tg::aabb2 const& clip);
    void render_window(si::element_tree const& tree, layouted_element const& le, input_state const& input, tg::aabb2 const& clip);

    // NOTE: end is exclusive
    void render_child_range(si::element_tree const& tree, int range_start, int range_end, input_state const& input, tg::aabb2 const& clip);

    // private member
private:
    font_atlas _font;
    render_data _render_data;
    si::element_tree const* _prev_ui = nullptr;

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
        tg::aabb2 bounds;
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
};
}
