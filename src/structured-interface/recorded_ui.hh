#pragma once

#include <clean-core/span.hh>
#include <clean-core/vector.hh>

#include <structured-interface/detail/record.hh>

namespace si
{
/// a binary blob containing the recorded ui data
/// this can be serialized and copied around (it's a value type)
/// it is only useful for getting merged into a si::gui
struct recorded_ui
{
    explicit recorded_ui(cc::vector<std::byte> data) : _data(cc::move(data)) {}

    // read-only view on the raw recorded bytes
    cc::span<std::byte const> raw_data() const { return _data; }

    /**
     * calls function on the visitor:
     * void start_element(size_t id)
     * void property(size_t prop_id, cc::span<cc::byte const> value)
     * void end_element()
     */
    template <class Visitor>
    void visit(Visitor&& visitor) const
    {
        using namespace si::detail;

        auto d = cc::span<std::byte const>(_data);
        size_t s, id;
        while (!d.empty())
        {
            switch (record_cmd(d.front()))
            {
            case record_cmd::start_element:
                id = *reinterpret_cast<size_t const*>(&d[1]);
                visitor.start_element(id);
                d = d.subspan(1 + sizeof(size_t));
                break;
            case record_cmd::end_element:
                visitor.end_element();
                d = d.subspan(1);
                break;
            case record_cmd::property:
                id = *reinterpret_cast<size_t const*>(&d[1]);
                s = *reinterpret_cast<size_t const*>(&d[1 + sizeof(size_t)]);
                visitor.property(id, d.subspan(1 + 2 * sizeof(size_t), s));
                d = d.subspan(1 + 2 * sizeof(size_t) + s);
                break;
            default:
                CC_UNREACHABLE("unknown command type");
            }
        }
    }

private:
    cc::vector<std::byte> _data;
};
}
