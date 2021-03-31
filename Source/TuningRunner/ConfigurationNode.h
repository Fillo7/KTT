#pragma once

#include <cstdint>
#include <memory>
#include <set>
#include <vector>

namespace ktt
{

class ConfigurationNode
{
public:
    ConfigurationNode();
    ConfigurationNode(const ConfigurationNode& parent, const size_t index);

    void AddPaths(const std::vector<size_t>& indices, const size_t indicesIndex, const std::vector<uint64_t>& levels,
        const size_t levelsIndex, const std::vector<uint64_t>& lockedLevels);
    void PrunePaths(const std::vector<size_t>& indices, const size_t indicesIndex, const std::vector<uint64_t>& levels,
        const size_t levelsIndex);
    void ComputeConfigurationsCount();
    void GatherParameterIndices(const uint64_t index, std::vector<size_t>& indices) const;
    std::vector<std::vector<size_t>> GatherNeighbourIndices(const std::vector<size_t>& parameterIndices,
        const uint64_t maxDifferences, const size_t maxNeighbours, const std::set<std::vector<size_t>>& exploredIndices) const;
    uint64_t ComputeLocalIndex(const std::vector<size_t>& parameterIndices) const;

    const ConfigurationNode* GetParent() const;
    uint64_t GetLevel() const;
    size_t GetIndex() const;
    size_t GetChildrenCount() const;
    uint64_t GetConfigurationsCount() const;

private:
    std::vector<std::unique_ptr<ConfigurationNode>> m_Children;
    const ConfigurationNode* m_Parent;
    size_t m_Index;
    uint64_t m_ConfigurationsCount;

    void AddChild(const size_t index);
    bool HasChildWithIndex(const size_t index) const;
    ConfigurationNode& GetChildWithIndex(const size_t index) const;
    ConfigurationNode* GetChildWithIndexPointer(const size_t index) const;
    bool HasBroadcastLevel(const std::vector<uint64_t>& levels, const size_t levelsIndex) const;
    void GatherNeighbours(std::vector<std::vector<size_t>>& result, std::vector<size_t>& partialResult,
        const std::vector<size_t>& parameterIndices, const uint64_t maxDifferences, const size_t maxNeighbours,
        const std::set<std::vector<size_t>>& exploredIndices) const;
};

} // namespace ktt
