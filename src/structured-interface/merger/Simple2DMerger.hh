#pragma once

#include <cstdint>

#include <clean-core/string_view.hh>
#include <clean-core/vector.hh>

#include <typed-geometry/tg-lean.hh>

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
    // settings
public:
    float padding_left = 4.0f;
    float padding_top = 4.0f;
    float padding_right = 4.0f;
    float padding_bottom = 4.0f;
    float padding_child = 4.0f;
    tg::color3 font_color = tg::color3::black;
    float font_size = 20.f;
    tg::aabb2 viewport = {{0, 0}, {1920, 1080}};

    // input test!
public:
    tg::pos2 mouse_pos;
    bool is_lmb_down = false;

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

    // private helper
private:
    tg::aabb2 perform_layout(si::element_tree& tree, si::element_tree_element& e, float x, float y);
    void build_render_data(si::element_tree const& tree, si::element_tree_element const& e, input_state const& input, tg::aabb2 const& clip);

    void load_default_font();

    tg::aabb2 get_text_bounds(cc::string_view txt, float x, float y);
    void add_text_render_data(render_list& rl, cc::string_view txt, float x, float y, tg::aabb2 const& clip);

    // layout methods
    tg::aabb2 perform_checkbox_layout(si::element_tree& tree, si::element_tree_element& e, float x, float y);

    // render methods
    // note: clip is already clipped to element aabb
    void render_text(si::element_tree const& tree, si::element_tree_element const& e, tg::aabb2 const& clip); // NOTE: requires text and text_origin properties
    void render_checkbox(si::element_tree const& tree, si::element_tree_element const& e, input_state const& input, tg::aabb2 const& clip);

    // private member
private:
    font_atlas _font;
    render_data _render_data;
};
}
