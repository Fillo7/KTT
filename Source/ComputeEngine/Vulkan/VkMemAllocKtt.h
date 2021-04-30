#pragma once

#ifdef KTT_API_VULKAN

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4100) // Unreferenced parameter
#pragma warning(disable : 4127) // Constant conditional expression
#pragma warning(disable : 4324) // Structure padding due to alignment specifier
#endif // _MSC_VER

#include <vk_mem_alloc.h>

#if defined(_MSC_VER)
#pragma warning(pop)
#endif // _MSC_VER

#endif // KTT_API_VULKAN
