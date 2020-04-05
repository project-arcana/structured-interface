#include "recorder.hh"

#include <cstring>

#include <clean-core/array.hh>

#include <structured-interface/fwd.hh>
#include <structured-interface/detail/record.hh>

static cc::array<std::byte>& recording_backing_buffer()
{
    static thread_local auto buffer = cc::array<std::byte>::uninitialized(1 << 20);
    return buffer;
}

void si::detail::start_recording(gui const& ui)
{
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
