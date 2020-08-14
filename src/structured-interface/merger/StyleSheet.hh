#pragma once

#include <clean-core/bit_cast.hh>
#include <clean-core/map.hh>
#include <clean-core/span.hh>
#include <clean-core/unique_function.hh>
#include <clean-core/vector.hh>

#include <structured-interface/element_type.hh>
#include <structured-interface/fwd.hh>
#include <structured-interface/style.hh>

namespace si
{
// TODO: cache clear policy
class StyleSheet
{
public:
    using style_key_int_t = uint32_t;
    using style_hash = cc::hash_t;
    struct style_key // key of a single element
    {
        // NOTE: is constructed in Default2DMerger::compute_style
        // TODO: more, e.g. https://www.w3schools.com/cssref/css_selectors.asp
        element_type type;
        uint8_t is_hovered : 1;
        uint8_t is_pressed : 1;
        uint8_t is_focused : 1;
        uint8_t is_first_child : 1;
        uint8_t is_last_child : 1;
        uint8_t is_odd_child : 1;
        uint8_t is_checked : 1;
        uint8_t is_enabled : 1;
        uint16_t style_class; // can be used to store multiple classes if a partial mask is used

        style_key() = default;
        style_key(element_type type)
        {
            *this = cc::bit_cast<style_key>(style_key_int_t(0));
            this->type = type;
        }
    };
    static_assert(sizeof(style_key) == sizeof(style_key_int_t));

    /// a complete computed style for an element
    /// loosely follows https://developer.mozilla.org/en-US/docs/Learn/CSS/Building_blocks/The_box_model
    /// NOTE: we use alternative box model where fixed size would include border and padding
    struct computed_style
    {
        style_hash hash = 0;
        style::layout layout = style::layout::top_down;
        style::visibility visibility = style::visibility::visible;
        style::positioning positioning = style::positioning::normal;
        style::overflow overflow = style::overflow::visible;
        style::margin margin;
        style::border border;
        style::padding padding;
        style::background bg;
        style::font font;
        style::bounds bounds;
        // TODO: more
        // TODO: arbitrary properties?
        // TODO: custom triangles
    };

    // Style API
public:
    /// clears this style and sets up the default light mode style
    void load_default_light_style();

    /// clears the style sheet
    void clear();

    /// adds a style sheet rule
    void add_rule(cc::string_view selector, cc::unique_function<void(computed_style&)> on_apply);

    /// queries a style for a given key
    /// will use cache, so is usually fast
    /// TODO: also add prev_key to support A + B syntax
    /// NOTE: parents are ordered root-to-child
    computed_style query_style(style_key key, style_hash parent_hash, cc::span<style_key const> parent_keys);

    /// computes the style of a given element
    /// NOTE: this does NOT use the cache!
    ///       usually query_style is the better choice!
    /// NOTE: parents are ordered root-to-child
    computed_style compute_style(style_key key, cc::span<style_key const> parent_keys) const;

    // uncommon API
public:
    size_t get_cached_styles_count() const { return _style_cache.size(); }
    size_t get_style_rule_count() const { return _rules.size(); }

    // style member
private:
    struct style_rule
    {
        struct part
        {
            style_key_int_t key;  // which elements this applies to
            style_key_int_t mask; // restricts which key parts are compared

            bool matches(style_key_int_t test_key) const { return (test_key & mask) == key; }
            bool matches(style_key test_key) const { return (cc::bit_cast<style_key_int_t>(test_key) & mask) == key; }

            bool immediate_only = false; // if true, must be immediate child
        };

        cc::vector<part> parts;
        cc::unique_function<void(computed_style&)> apply; // changes the style

        // TODO: priority?
    };

    cc::vector<style_rule> _rules;

    // cache member
private:
    cc::map<style_hash, computed_style> _style_cache;
};
}
