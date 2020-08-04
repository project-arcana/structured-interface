#include "StyleSheet.hh"

#include <clean-core/xxHash.hh>


si::StyleSheet::StyleSheet()
{
    // from config?
    load_default_light_style();
}

void si::StyleSheet::load_default_light_style()
{
    clear();

    // default
    add_rule({}, [](computed_style& s) {
        s.font.color = tg::color3::black;
        s.margin = {4, 2};
    });

    add_rule(element_type::tooltip, [](computed_style& s) {
        s.bg = tg::color4(0.9f, 0.9f, 1.0f, 0.9f);
        s.border = 1.f;
        s.margin = 0;
        s.padding = {4, 8};
    });

    add_rule(element_type::popover, [](computed_style& s) {
        s.bg = tg::color4(0.9f, 0.9f, 1.0f, 0.9f);
        s.border = 1.f;
        s.margin = 0;
        s.padding = {4, 8};
    });

    add_rule(element_type::row, [](computed_style& s) { s.layout = style::layout::left_right; });

    add_rule(element_type::window, [](computed_style& s) {
        s.bg = tg::color4(0.8f, 0.8f, 1.0f, 0.9f);
        s.border = 1.f;
        s.padding = 2;
    });

    add_rule(element_type::button, [](computed_style& s) {
        s.bg = tg::color4(0, 0, 1, 0.2f);
        s.margin = 2;
        s.padding = {1, 2};
        s.border = 1.f;
        s.border.color = {0.3f, 0.3f, 1.0f};
    });
    add_rule(style_selector(element_type::button).hovered(), [](computed_style& s) { s.bg = tg::color4(0, 0, 1, 0.3f); });
    add_rule(style_selector(element_type::button).pressed(), [](computed_style& s) { s.bg = tg::color4(0, 0, 1, 0.5f); });

    // DEBUG
    add_rule(element_type::textbox, [](computed_style& s) {
        s.bg = tg::color4(0.9f, 0.9f, 1.0f, 0.9f);
        s.padding.left = 100;
    });
    add_rule(element_type::input, [](computed_style& s) { s.font.color = {0.3f, 0.3f, 0.3f}; });
    add_rule(style_selector().focused(), [](computed_style& s) { s.bg.color = tg::color3::red; });
}

void si::StyleSheet::clear()
{
    // clear styles
    _rules.clear();

    // clear cache
    _style_cache.clear();
}

void si::StyleSheet::add_rule(si::StyleSheet::style_selector selector, cc::unique_function<void(si::StyleSheet::computed_style&)> on_apply)
{
    auto& r = _rules.emplace_back();
    r.key = cc::bit_cast<style_key_int_t>(selector.key);
    r.mask = cc::bit_cast<style_key_int_t>(selector.mask);
    CC_ASSERT((r.key & ~r.mask) == 0 && "selector key outside of mask is not allowed");
    r.apply = cc::move(on_apply);
}

si::StyleSheet::computed_style si::StyleSheet::query_style(si::StyleSheet::style_key key,
                                                           si::StyleSheet::style_hash parent_hash,
                                                           cc::span<const si::StyleSheet::style_key> parent_keys)
{
    auto const hash = cc::hash_xxh3(cc::as_byte_span(key), parent_hash);

    // get or compute style
    computed_style style;
    if (!_style_cache.get_to(hash, style))
    {
        style = compute_style(key, parent_keys);
        style.hash = hash;
        _style_cache[hash] = style;
    }

    return style;
}

si::StyleSheet::computed_style si::StyleSheet::compute_style(si::StyleSheet::style_key key, cc::span<const si::StyleSheet::style_key> parent_keys) const
{
    computed_style style;

    auto ikey = cc::bit_cast<style_key_int_t>(key);
    // TODO: parent stuff, maybe some other speedups?
    (void)parent_keys;

    for (auto const& r : _rules)
        if ((ikey & r.mask) == r.key)
            r.apply(style);

    return style;
}
