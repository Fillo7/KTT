#pragma once

#include <cstdint>
#include <string>

#include <Output/TimeConfiguration/TimeUnit.h>
#include <Utility/DisableCopyMove.h>
#include <KttTypes.h>

namespace ktt
{

class TimeConfiguration : public DisableCopyMove
{
public:
    static TimeConfiguration& GetInstance();

    void SetTimeUnit(const TimeUnit unit);

    TimeUnit GetTimeUnit() const;
    uint64_t ConvertDuration(const Nanoseconds duration) const;
    double ConvertDurationDouble(const Nanoseconds duration) const;
    std::string GetUnitTag() const;

private:
    TimeUnit m_TimeUnit;

    TimeConfiguration();
};

} // namespace ktt
