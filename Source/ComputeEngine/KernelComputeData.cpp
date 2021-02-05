#include <algorithm>
#include <limits>

#include <Api/Configuration/KernelConfiguration.h>
#include <ComputeEngine/KernelComputeData.h>
#include <Kernel/Kernel.h>
#include <Kernel/KernelDefinition.h>

namespace ktt
{

KernelComputeData::KernelComputeData(const Kernel& kernel, const KernelDefinition& definition, const KernelConfiguration& configuration) :
    m_Name(definition.GetName()),
    m_DefaultSource(definition.GetSource()),
    m_ConfigurationPrefix(configuration.GeneratePrefix()),
    m_Configuration(&configuration),
    m_Arguments(definition.GetArguments())
{
    m_GlobalSize = kernel.GetModifiedGlobalSize(definition.GetId(), configuration.GetPairs());
    m_LocalSize = kernel.GetModifiedLocalSize(definition.GetId(), configuration.GetPairs());
}

void KernelComputeData::SetGlobalSize(const DimensionVector& globalSize)
{
    m_GlobalSize = globalSize;
}

void KernelComputeData::SetLocalSize(const DimensionVector& localSize)
{
    m_LocalSize = localSize;
}

void KernelComputeData::UpdateArgument(const ArgumentId id, KernelArgument& argument)
{
    for (size_t i = 0; i < m_Arguments.size(); ++i)
    {
        if (m_Arguments[i]->GetId() == id)
        {
            m_Arguments[i] = &argument;
            break;
        }
    }
}

void KernelComputeData::SwapArguments(const ArgumentId first, const ArgumentId second)
{
    const size_t invalidIndex = std::numeric_limits<size_t>::max();
    size_t firstIndex = invalidIndex;
    size_t secondIndex = invalidIndex;

    for (size_t i = 0; i < m_Arguments.size(); ++i)
    {
        if (m_Arguments[i]->GetId() == first)
        {
            firstIndex = i;
        }
        else if (m_Arguments[i]->GetId() == second)
        {
            secondIndex = i;
        }
    }

    if (firstIndex == invalidIndex || secondIndex == invalidIndex)
    {
        throw KttException("One of the arguments with ids " + std::to_string(first) + " and " + std::to_string(second)
            + " is not associated with kernel definition " + m_Name);
    }

    std::swap(m_Arguments[firstIndex], m_Arguments[secondIndex]);
}

void KernelComputeData::SetArguments(const std::vector<KernelArgument*> arguments)
{
    m_Arguments = arguments;
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

const KernelConfiguration& KernelComputeData::GetConfiguration() const
{
    return *m_Configuration;
}

const std::vector<KernelArgument*>& KernelComputeData::GetArguments() const
{
    return m_Arguments;
}

} // namespace ktt
