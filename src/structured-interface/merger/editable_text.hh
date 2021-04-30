#pragma once

#include <clean-core/string.hh>
#include <clean-core/string_view.hh>
#include <clean-core/vector.hh>

#include <typed-geometry/tg.hh>

namespace si::merger
{
/**
 * Helper class for managing an editable text
 * Includes selection handling
 */
struct editable_text
{
    struct glyph
    {
        size_t start = 0;
        size_t count = 1; // > 1 for unicode stuff
        tg::aabb2 bounds;
    };

    // query API
public:
    cc::string_view text() const { return _text; }
    size_t cursor() const { return _cursor; }
    size_t selection_start() const { return _selection_start; }
    size_t selection_count() const { return _selection_count; }
    bool has_selection() const { return _selection_count > 0; }
    cc::vector<glyph> const& glyphs() const { return _glyphs; }

    // text mod API
public:
    /// clears text and selection
    void reset(cc::string new_text = "");

    /// this should be called when text input is recieved
    void on_text_input(cc::string_view s);

    /// [backspace]
    void remove_prev_char();
    /// [entf]
    void remove_next_char();

    // selection API
public:
    /// sets the selection to the complete text
    void select_all();
    /// clears selection
    void deselect();

    // cursor API
public:
    void move_cursor_left();
    void move_cursor_right();

    // geometry
public:
    void set_glyphs(cc::vector<glyph> glyphs) { _glyphs = cc::move(glyphs); }

private:
    cc::string _text;
    size_t _cursor = 0; ///< added chars have cursor as their new index
    size_t _selection_start = 0;
    size_t _selection_count = 0;
    cc::vector<glyph> _glyphs;
};
}
