#include "recorder.hh"

#include <rich-log/log.hh>

#include <cstring>

#include <clean-core/array.hh>
#include <clean-core/vector.hh>

#include <structured-interface/detail/record.hh>
#include <structured-interface/fwd.hh>
#include <structured-interface/properties.hh>

namespace
{
cc::array<std::byte>& recording_backing_buffer()
{
    static thread_local auto buffer = cc::array<std::byte>::uninitialized(1 << 20);
    return buffer;
}

struct element_stack_entry
{
    si::element_handle id;
    uint64_t prev_id_seed;
};

cc::vector<element_stack_entry>& recording_element_stack()
{
    static thread_local cc::vector<element_stack_entry> elements;
    return elements;
}
}

void si::detail::push_element(element_handle h)
{
    recording_element_stack().push_back({h, detail::id_seed()});
    si::detail::curr_element() = h;
    si::detail::id_seed() = h.id() ^ 0x9ac2'1712'39a8'b3c4;
}

void si::detail::pop_element(element_handle h)
{
    auto& s = recording_element_stack();
    CC_ASSERT(!s.empty() && s.back().id == h && "corrupted element stack");
    si::detail::id_seed() = s.back().prev_id_seed;
    s.pop_back();
    si::detail::curr_element() = s.empty() ? element_handle{} : s.back().id;
}

void si::detail::start_recording(gui const& ui)
{
    si::detail::init_default_properties();

    (void)ui; // TODO: use this for ID lookup

    // TODO: proper chunking allocator
    auto& buffer = recording_backing_buffer();
    si::detail::get_record_buffer() = buffer.data();
    si::detail::get_record_buffer_end() = buffer.data() + buffer.size();
}

void si::detail::end_recording(cc::vector<std::byte>& data)
{
    auto d_begin = recording_backing_buffer().data();
    auto d_end = si::detail::get_record_buffer();

    // TODO: faster
    data.resize(d_end - d_begin);
    std::memcpy(data.begin(), d_begin, d_end - d_begin);

    si::detail::get_record_buffer() = nullptr;
    si::detail::get_record_buffer_end() = nullptr;
}
