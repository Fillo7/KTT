#include <Output/TimeConfiguration/TimeConfiguration.h>
#include <Output/TunerMetadata.h>
#include <KttPlatform.h>

namespace ktt
{

TunerMetadata::TunerMetadata(const ComputeApi api, const PlatformInfo& platformInfo, const DeviceInfo& deviceInfo) :
    m_ComputeApi(api),
    m_PlatformName(platformInfo.GetName()),
    m_DeviceName(deviceInfo.GetName())
{
    m_KttVersion = GetKttVersionString();
    m_TimeUnit = TimeConfiguration::GetInstance().GetTimeUnit();
}

void TunerMetadata::SetComputeApi(const ComputeApi api)
{
    m_ComputeApi = api;
}

void TunerMetadata::SetPlatformName(const std::string& name)
{
    m_PlatformName = name;
}

void TunerMetadata::SetDeviceName(const std::string& name)
{
    m_DeviceName = name;
}

void TunerMetadata::SetKttVersion(const std::string& version)
{
    m_KttVersion = version;
}

void TunerMetadata::SetTimeUnit(const TimeUnit unit)
{
    m_TimeUnit = unit;
}

ComputeApi TunerMetadata::GetComputeApi() const
{
    return m_ComputeApi;
}

const std::string& TunerMetadata::GetPlatformName() const
{
    return m_PlatformName;
}

const std::string& TunerMetadata::GetDeviceName() const
{
    return m_DeviceName;
}

const std::string& TunerMetadata::GetKttVersion() const
{
    return m_KttVersion;
}

TimeUnit TunerMetadata::GetTimeUnit() const
{
    return m_TimeUnit;
}

} // namespace ktt