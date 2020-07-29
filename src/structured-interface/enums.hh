#pragma once

#include <cstdint>

namespace si
{
enum class visibility
{
    visible, ///< visible and takes up space
    hidden,  ///< invisible but takes up space
    none     ///< invisible and does not take up space
};
}
