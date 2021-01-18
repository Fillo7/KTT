#include <ComputeEngine/KernelComputeData.h>

namespace ktt
{

KernelComputeData::KernelComputeData(const std::string& name, const std::string& defaultSource, const std::string& configurationPrefix,
    const DimensionVector& globalSize, const DimensionVector& localSize, const std::vector<ParameterPair>& parameterPairs,
    const std::vector<const KernelArgument*>& arguments) :
    m_Name(name),
    m_DefaultSource(defaultSource),
    m_ConfigurationPrefix(configurationPrefix),
    m_GlobalSize(globalSize),
    m_LocalSize(localSize),
    m_ParameterPairs(parameterPairs),
    m_Arguments(arguments)
{}

void KernelComputeData::SetGlobalSize(const DimensionVector& globalSize)
{
    m_GlobalSize = globalSize;
}

void KernelComputeData::SetLocalSize(const DimensionVector& localSize)
{
    m_LocalSize = localSize;
}

const std::string& KernelComputeData::GetName() const
{
    return m_Name;
}

const std::string& KernelComputeData::GetDefaultSource() const
{
    return m_DefaultSource;
}

const std::string& KernelComputeData::GetConfigurationPrefix() const
{
    return m_ConfigurationPrefix;
}

std::string KernelComputeData::GetSource() const
{
    return m_ConfigurationPrefix + m_DefaultSource;
}

KernelComputeId KernelComputeData::GetUniqueIdentifier() const
{
    return m_Name + m_ConfigurationPrefix;
}

const DimensionVector& KernelComputeData::GetGlobalSize() const
{
    return m_GlobalSize;
}

const DimensionVector& KernelComputeData::GetLocalSize() const
{
    return m_LocalSize;
}

const std::vector<ParameterPair>& KernelComputeData::GetParameterPairs() const
{
    return m_ParameterPairs;
}

const std::vector<const KernelArgument*>& KernelComputeData::GetArguments() const
{
    return m_Arguments;
}

} // namespace ktt
