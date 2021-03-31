#include <algorithm>

#include <Api/KttException.h>
#include <TuningRunner/ConfigurationTree.h>
#include <Utility/ErrorHandling/Assert.h>
#include <Utility/StlHelpers.h>

namespace ktt
{

ConfigurationTree::ConfigurationTree() :
    m_Root(nullptr),
    m_IsBuilt(false)
{}

void ConfigurationTree::Build(const KernelParameterGroup& group)
{
    m_Root = std::make_unique<ConfigurationNode>();

    std::set<const KernelConstraint*> processedConstraints;
    std::set<std::string> processedParameters;

    while (processedConstraints.size() != group.GetConstraints().size())
    {
        const KernelConstraint& constraint = group.GetNextConstraintToProcess(processedConstraints, processedParameters);
        const uint64_t affectedCount = constraint.GetAffectedParameterCount(processedParameters);

        constraint.EnumerateParameterIndices([this, &processedParameters, &constraint, affectedCount] (std::vector<size_t>& indices,
            const bool validIndices)
        {
            if (validIndices && affectedCount < constraint.GetParameterNames().size())
            {
                AddPaths(indices, constraint.GetParameters(), processedParameters);
            }
        });

        constraint.EnumerateParameterIndices([this, &constraint, affectedCount] (std::vector<size_t>& indices,
            const bool validIndices)
        {
            if (!validIndices && affectedCount > 0)
            {
                PrunePaths(indices, constraint.GetParameters());
            }
        });

        processedConstraints.insert(&constraint);

        for (const auto& name : constraint.GetParameterNames())
        {
            processedParameters.insert(name);
        }
    }

    for (const auto* parameter : group.GetParameters())
    {
        if (ContainsKey(processedParameters, parameter->GetName()))
        {
            continue;
        }

        for (size_t index = 0; index < parameter->GetValuesCount(); ++index)
        {
            AddPaths({index}, {parameter}, processedParameters);
        }

        processedParameters.insert(parameter->GetName());
    }

    m_Root->ComputeConfigurationsCount();
    m_IsBuilt = true;
}

void ConfigurationTree::Clear()
{
    m_ParameterToLevel.clear();
    m_Root.reset();
    m_IsBuilt = false;
}

bool ConfigurationTree::IsBuilt() const
{
    return m_IsBuilt;
}

bool ConfigurationTree::HasParameter(const std::string& name) const
{
    for (const auto& pair : m_ParameterToLevel)
    {
        if (pair.first->GetName() == name)
        {
            return true;
        }
    }

    return false;
}

uint64_t ConfigurationTree::GetDepth() const
{
    return m_ParameterToLevel.size();
}

uint64_t ConfigurationTree::GetConfigurationsCount() const
{
    KttAssert(m_IsBuilt, "The tree must be built before submitting queries");
    return m_Root->GetConfigurationsCount();
}

KernelConfiguration ConfigurationTree::GetConfiguration(const uint64_t index) const
{
    KttAssert(m_IsBuilt, "The tree must be built before submitting queries");
    
    if (index >= GetConfigurationsCount())
    {
        throw KttException("Invalid configuration index");
    }

    std::vector<size_t> indices;
    m_Root->GatherParameterIndices(index + 1, indices);
    return GetConfigurationFromIndices(indices);
}

std::vector<KernelConfiguration> ConfigurationTree::GetNeighbourConfigurations(const KernelConfiguration& configuration,
    const uint64_t maxDifferences, const size_t maxNeighbours, const std::set<uint64_t> exploredConfigurations) const
{
    KttAssert(m_IsBuilt, "The tree must be built before submitting queries");
    const std::vector<size_t> indices = GetIndicesFromConfiguration(configuration);
    std::set<std::vector<size_t>> exploredIndices;

    for (const auto exploredIndex : exploredConfigurations)
    {
        std::vector<size_t> exploredParameterIndices;
        m_Root->GatherParameterIndices(exploredIndex + 1, exploredParameterIndices);
        exploredIndices.insert(exploredParameterIndices);
    }

    const auto neighbours = m_Root->GatherNeighbourIndices(indices, maxDifferences, maxNeighbours, exploredIndices);
    std::vector<KernelConfiguration> result;

    for (const auto& neighbourIndices : neighbours)
    {
        result.push_back(GetConfigurationFromIndices(neighbourIndices));
    }

    return result;
}

uint64_t ConfigurationTree::GetLocalConfigurationIndex(const KernelConfiguration& configuration) const
{
    KttAssert(m_IsBuilt, "The tree must be built before submitting queries");
    const std::vector<size_t> indices = GetIndicesFromConfiguration(configuration);
    return m_Root->ComputeLocalIndex(indices);
}

void ConfigurationTree::AddPaths(const std::vector<size_t>& indices, const std::vector<const KernelParameter*>& parameters,
    std::set<std::string>& lockedParameters)
{
    std::vector<uint64_t> levels;
    const auto finalIndices = PreprocessIndices(indices, parameters, levels);
    const auto lockedLevels = GetParameterLevels(lockedParameters);
    m_Root->AddPaths(finalIndices, 0, levels, 0, lockedLevels);
}

void ConfigurationTree::PrunePaths(const std::vector<size_t>& indices, const std::vector<const KernelParameter*>& parameters)
{
    std::vector<uint64_t> levels;
    const auto finalIndices = PreprocessIndices(indices, parameters, levels);
    m_Root->PrunePaths(finalIndices, 0, levels, 0);
}

std::vector<size_t> ConfigurationTree::PreprocessIndices(const std::vector<size_t>& indices,
    const std::vector<const KernelParameter*>& parameters, std::vector<uint64_t>& levels)
{
    auto parameterIndices = MergeParametersWithIndices(parameters, indices);

    auto iterator = std::partition(parameterIndices.begin(), parameterIndices.end(), [this](const auto& pair)
    {
        return ContainsKey(m_ParameterToLevel, pair.first);
    });

    for (; iterator != parameterIndices.end(); ++iterator)
    {
        const uint64_t depth = GetDepth();
        m_ParameterToLevel[iterator->first] = depth + 1;
    }

    std::sort(parameterIndices.begin(), parameterIndices.end(), [this](const auto& leftPair, const auto& rightPair)
    {
        return m_ParameterToLevel[leftPair.first] < m_ParameterToLevel[rightPair.first];
    });

    std::vector<size_t> finalIndices;

    for (const auto& pair : parameterIndices)
    {
        finalIndices.push_back(pair.second);
        levels.push_back(m_ParameterToLevel[pair.first]);
    }

    return finalIndices;
}

std::vector<uint64_t> ConfigurationTree::GetParameterLevels(const std::set<std::string>& parameters) const
{
    std::vector<uint64_t> result;

    for (const auto& parameterName : parameters)
    {
        for (const auto& pair : m_ParameterToLevel)
        {
            if (pair.first->GetName() == parameterName)
            {
                result.push_back(pair.second);
                break;
            }
        }
    }

    return result;
}

KernelConfiguration ConfigurationTree::GetConfigurationFromIndices(const std::vector<size_t>& indices) const
{
    std::vector<ParameterPair> pairs;

    for (size_t i = 0; i < indices.size(); ++i)
    {
        for (const auto& pair : m_ParameterToLevel)
        {
            if (pair.second == i + 1)
            {
                pairs.push_back(pair.first->GeneratePair(indices[i]));
                break;
            }
        }
    }

    return KernelConfiguration(pairs);
}

std::vector<size_t> ConfigurationTree::GetIndicesFromConfiguration(const KernelConfiguration& configuration) const
{
    std::vector<size_t> indices;

    for (uint64_t level = 1; level <= GetDepth(); ++level)
    {
        const KernelParameter* currentParameter = nullptr;

        for (const auto& pair : m_ParameterToLevel)
        {
            if (pair.second == level)
            {
                currentParameter = pair.first;
                break;
            }
        }

        for (const auto& pair : configuration.GetPairs())
        {
            if (pair.GetName() != currentParameter->GetName())
            {
                continue;
            }

            for (size_t index = 0; index < currentParameter->GetValuesCount(); ++index)
            {
                if (pair.HasSameValue(currentParameter->GeneratePair(index)))
                {
                    indices.push_back(index);
                    break;
                }
            }

            break;
        }
    }

    return indices;
}

std::vector<std::pair<const KernelParameter*, size_t>> ConfigurationTree::MergeParametersWithIndices(
    const std::vector<const KernelParameter*>& parameters, const std::vector<size_t>& indices)
{
    KttAssert(parameters.size() == indices.size(), "Vector sizes must be equal");
    std::vector<std::pair<const KernelParameter*, size_t>> result;
    result.reserve(parameters.size());

    for (size_t i = 0; i < parameters.size(); ++i)
    {
        result.emplace_back(parameters[i], indices[i]);
    }

    return result;
}

} // namespace ktt
