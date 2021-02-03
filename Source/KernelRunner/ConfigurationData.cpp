#include <algorithm>
#include <limits>

#include <KernelRunner/ConfigurationData.h>
#include <Utility/ErrorHandling/Assert.h>
#include <Utility/Logger/Logger.h>
#include <Utility/StlHelpers.h>

namespace ktt
{

ConfigurationData::ConfigurationData(const DeviceInfo& deviceInfo, Searcher& searcher, const Kernel& kernel) :
    m_DeviceInfo(deviceInfo),
    m_Searcher(searcher),
    m_Kernel(kernel),
    m_CurrentGroup(std::numeric_limits<size_t>::max())
{
    m_Groups = kernel.GenerateParameterGroups();

    if (!IsProcessed())
    {
        InitializeNextGroup();
    }
}

ConfigurationData::~ConfigurationData()
{
    m_Searcher.Reset();
}

bool ConfigurationData::CalculateNextConfiguration(const KernelResult& previousResult)
{
    m_ProcessedConfigurations.insert(std::make_pair(previousResult.GetTotalDuration(), GetCurrentConfiguration()));

    if (m_Searcher.GetUnexploredConfigurationCount() > 0)
    {
        m_Searcher.CalculateNextConfiguration(previousResult);
        return true;
    }

    return InitializeNextGroup();
}

uint64_t ConfigurationData::GetConfigurationCount() const
{
    return static_cast<uint64_t>(m_Configurations.size());
}

bool ConfigurationData::IsProcessed() const
{
    return m_CurrentGroup >= m_Groups.size();
}

const KernelParameterGroup& ConfigurationData::GetCurrentGroup() const
{
    KttAssert(!IsProcessed(), "Current group can only be retrieved for configuration data that is not processed");
    return m_Groups[m_CurrentGroup];
}

const KernelConfiguration& ConfigurationData::GetCurrentConfiguration() const
{
    if (m_Configurations.empty() || m_Searcher.GetUnexploredConfigurationCount() == 0)
    {
        static KernelConfiguration defaultConfiguration;
        return defaultConfiguration;
    }

    return m_Searcher.GetCurrentConfiguration();
}

const KernelConfiguration& ConfigurationData::GetBestConfiguration() const
{
    if (!m_ProcessedConfigurations.empty())
    {
        const auto& bestPair = *m_ProcessedConfigurations.cbegin();
        return bestPair.second;
    }

    return GetCurrentConfiguration();
}

bool ConfigurationData::InitializeNextGroup()
{
    m_Searcher.Reset();
    m_Configurations.clear();
    ++m_CurrentGroup;

    if (IsProcessed())
    {
        return false;
    }

    const auto& group = GetCurrentGroup();
    ComputeConfigurations(group, 0, std::vector<ParameterPair>{}, m_Configurations);
    m_Searcher.Initialize(m_Configurations);

    Logger::LogInfo("Starting to explore configurations for kernel " + std::to_string(m_Kernel.GetId()) + " and group "
        + group.GetName() + ", configuration count in the current group is " + std::to_string(GetConfigurationCount()));
    return true;
}

void ConfigurationData::ComputeConfigurations(const KernelParameterGroup& group, const size_t currentIndex,
    const std::vector<ParameterPair>& pairs, std::vector<KernelConfiguration>& finalResult) const
{
    if (currentIndex >= group.GetParameters().size())
    {
        // All parameters are now included in the configuration
        std::vector<ParameterPair> allPairs = GetExtraParameterPairs(pairs);
        allPairs.insert(allPairs.end(), pairs.cbegin(), pairs.cend());

        KernelConfiguration configuration(allPairs);

        if (IsConfigurationValid(configuration))
        {
            finalResult.push_back(configuration);
        }

        return;
    }

    const KernelParameter& parameter = *group.GetParameters()[currentIndex]; 

    for (const auto& pair : parameter.GeneratePairs())
    {
        // Recursively build tree of configurations for each parameter value
        std::vector<ParameterPair> newPairs = pairs;
        newPairs.push_back(pair);

        if (!EvaluateConstraints(newPairs))
        {
            continue;
        }

        ComputeConfigurations(group, currentIndex + 1, newPairs, finalResult);
    }
}

std::vector<ParameterPair> ConfigurationData::GetExtraParameterPairs(const std::vector<ParameterPair>& pairs) const
{
    std::vector<std::string> addedParameters;
    const auto& currentGroup = GetCurrentGroup();

    for (const auto* parameter : currentGroup.GetParameters())
    {
        addedParameters.push_back(parameter->GetName());
    }

    std::vector<ParameterPair> result;
    KernelConfiguration bestConfiguration;
    const bool valid = GetBestCompatibleConfiguration(pairs, bestConfiguration);

    if (valid)
    {
        for (const auto& bestPair : bestConfiguration.GetPairs())
        {
            if (!ContainsElement(addedParameters, bestPair.GetName()))
            {
                result.push_back(bestPair);
                addedParameters.push_back(bestPair.GetName());
            }
        }
    }

    for (const auto& parameter : m_Kernel.GetParameters())
    {
        if (!ContainsElement(addedParameters, parameter.GetName()))
        {
            result.push_back(parameter.GeneratePair(0));
            addedParameters.push_back(parameter.GetName());
        }
    }

    return result;
}

bool ConfigurationData::GetBestCompatibleConfiguration(const std::vector<ParameterPair>& pairs, KernelConfiguration& output) const
{
    for (const auto& configuration : m_ProcessedConfigurations)
    {
        if (IsConfigurationCompatible(pairs, configuration.second))
        {
            output = configuration.second;
            return true;
        }
    }

    return false;
}

bool ConfigurationData::IsConfigurationCompatible(const std::vector<ParameterPair>& pairs,
    const KernelConfiguration& configuration) const
{
    for (const auto& pair : pairs)
    {
        const auto& configurationPairs = configuration.GetPairs();

        const auto iterator = std::find_if(configurationPairs.cbegin(), configurationPairs.cend(), [&pair](const auto& configurationPair)
        {
            return pair.GetName() == configurationPair.GetName();
        });

        if (iterator == configurationPairs.cend())
        {
            continue;
        }

        if (!pair.HasSameValue(*iterator))
        {
            return false;
        }
    }

    return true;
}

bool ConfigurationData::IsConfigurationValid(const KernelConfiguration& configuration) const
{
    const std::vector<ParameterPair>& pairs = configuration.GetPairs();

    for (const auto* definition : m_Kernel.GetDefinitions())
    {
        DimensionVector localSize = m_Kernel.GetModifiedLocalSize(definition->GetId(), pairs);

        if (localSize.GetTotalSize() > static_cast<size_t>(m_DeviceInfo.GetMaxWorkGroupSize()))
        {
            return false;
        }
    }

    if (!EvaluateConstraints(pairs))
    {
        return false;
    }

    return true;
}

bool ConfigurationData::EvaluateConstraints(const std::vector<ParameterPair>& pairs) const
{
    for (const auto& constraint : m_Kernel.GetConstraints())
    {
        if (!constraint.IsFulfilled(pairs))
        {
            return false;
        }
    }

    return true;
}

} // namespace ktt
