#include "anchor.hh"

#include <typed-geometry/tg.hh>

tg::pos2 si::placement::compute(tg::aabb2 ref_bb, tg::pos2 mouse_pos, tg::aabb2 this_bb) const
{
    auto ref_p = anchor_pos(ref_anchor, ref_bb, mouse_pos);
    auto this_p = anchor_pos(this_anchor, this_bb, mouse_pos);
    auto p = ref_p + offset + (this_bb.min - this_p);

    // unanchored
    if ((uint32_t(this_anchor) & 0x0F) == 0)
        p.x = this_bb.min.x;
    if ((uint32_t(this_anchor) & 0xF0) == 0)
        p.y = this_bb.min.y;

    return p;
}
