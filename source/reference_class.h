#pragma once

#include "enum/argument_data_type.h"

namespace ktt
{

class ReferenceClass
{
public:
    virtual ~ReferenceClass() = default;
    virtual void computeResult() = 0;
    virtual void* getData() const = 0;
    virtual ArgumentDataType getDataType() const = 0;
    virtual size_t getDataSizeInBytes() const = 0;
};

} // namespace ktt
