#include "StyleSheet.hh"

#include <clean-core/xxHash.hh>

void si::StyleSheet::load_default_light_style()
{
    clear();

    // default
    add_rule("*", [](computed_style& s) {
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
    add_rule(element_type::input, [](computed_style& s) { s.font.color = {0.2f, 0.2f, 0.2f}; });
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
    auto& p = r.parts.emplace_back();
    p.key = cc::bit_cast<style_key_int_t>(selector.key);
    p.mask = cc::bit_cast<style_key_int_t>(selector.mask);
    CC_ASSERT((p.key & ~p.mask) == 0 && "selector key outside of mask is not allowed");
    r.apply = cc::move(on_apply);
}

void si::StyleSheet::add_rule(cc::string_view selector, cc::unique_function<void(si::StyleSheet::computed_style&)> on_apply)
{
    // TODO: better error handling
    CC_ASSERT(!selector.empty() && "select-all must be done via '*'");

    auto& r = _rules.emplace_back();
    r.apply = cc::move(on_apply);

    auto string_to_type = [](cc::string_view s) -> element_type {
        for (auto i = 0; i < 128; ++i)
            if (to_string(element_type(i)) == s)
                return element_type(i);
        CC_UNREACHABLE("unknown type");
    };

    auto make_part = [&](cc::string_view ss, bool is_immediate) {
        CC_ASSERT(!ss.empty());
        auto& p = r.parts.emplace_back();
        p.immediate_only = is_immediate;

        // start with select all
        style_key key = cc::bit_cast<style_key>(style_key_int_t(0));
        style_key mask = cc::bit_cast<style_key>(style_key_int_t(0));

        auto is_first = true;
        for (auto s : ss.split(':'))
        {
            if (is_first) // primary selector
            {
                if (s == "" || s == "*") // select all
                {
                    // nothing to do
                }
                else if (s.starts_with('.')) // classes
                {
                    CC_ASSERT(false && "classes not implemented");
                }
                else if (s.starts_with('#'))
                {
                    CC_ASSERT(false && "ids not supported");
                }
                else // must be element type
                {
                    key.type = string_to_type(s);
                    mask.type = element_type(0xFF);
                }

                is_first = false;
            }
            else // modifiers
            {
                // TODO: "not" versions?
                if (s == "hover")
                {
                    key.is_hovered = true;
                    mask.is_hovered = true;
                }
                else if (s == "press")
                {
                    key.is_pressed = true;
                    mask.is_pressed = true;
                }
                else if (s == "focus")
                {
                    key.is_focused = true;
                    mask.is_focused = true;
                }
                else if (s == "first-child")
                {
                    key.is_first_child = true;
                    mask.is_first_child = true;
                }
                else if (s == "last-child")
                {
                    key.is_last_child = true;
                    mask.is_last_child = true;
                }
                else if (s == "odd-child")
                {
                    key.is_odd_child = true;
                    mask.is_odd_child = true;
                }
                else
                {
                    CC_ASSERT(false && "unknown modifier");
                }
            }
        }

        p.key = cc::bit_cast<style_key_int_t>(key);
        p.mask = cc::bit_cast<style_key_int_t>(mask);
    };

    // parse parts
    auto is_immediate = false;
    for (auto s : selector.split())
    {
        if (s == ">")
        {
            is_immediate = true;
            continue;
        }

        CC_ASSERT(s != "+" && "not implemented");
        CC_ASSERT(s != "~" && "not implemented");
        CC_ASSERT(s != "," && "not implemented");

        make_part(s, is_immediate);

        is_immediate = false;
    }

    CC_ASSERT(!is_immediate && "selector cannot end with '>'");
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

    for (auto const& r : _rules)
    {
        CC_ASSERT(!r.parts.empty());

        if (r.parts.size() > parent_keys.size() + 1)
            continue; // cannot possibly match (too many parts)

        // last part must match
        auto const& lp = r.parts.back();
        if (lp.matches(key))
        {
            auto is_matching = true;

            // check parents
            if (r.parts.size() >= 2)
            {
                CC_ASSERT(parent_keys.size() > 0);
                auto parent_idx = int(parent_keys.size() - 1);
                auto part_idx = int(r.parts.size() - 2); // -1 is already matching current key

                // try to match all remaining parts (backwards)
                while (part_idx >= 0)
                {
                    auto const& p = r.parts[part_idx];
                    CC_ASSERT(!p.immediate_only && "not supported yet");
                    while (parent_idx >= 0 && !p.matches(parent_keys[parent_idx]))
                        --parent_idx;

                    if (parent_idx < 0) // could not match
                    {
                        is_matching = false;
                        break;
                    }

                    --parent_idx;
                    --part_idx;
                }
            }

            if (is_matching)
                r.apply(style);
        }
    }

    return style;
}
