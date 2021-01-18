#pragma once

#include <string>
#include <vector>

#include <Api/DimensionVector.h>
#include <Api/ParameterPair.h>
#include <KttTypes.h>

namespace ktt
{

class KernelComputeData
{
public:
    explicit KernelComputeData(const std::string& name, const std::string& defaultSource, const std::string& configurationPrefix,
        const DimensionVector& globalSize, const DimensionVector& localSize, const std::vector<ParameterPair>& parameterPairs,
        const std::vector<ArgumentId>& arguments);

    void SetGlobalSize(const DimensionVector& globalSize);
    void SetLocalSize(const DimensionVector& localSize);

    const std::string& GetName() const;
    const std::string& GetDefaultSource() const;
    std::string GetSource() const;
    KernelComputeId GetUniqueIdentifier() const;
    const DimensionVector& GetGlobalSize() const;
    const DimensionVector& GetLocalSize() const;
    std::vector<size_t> GetGlobalSizeVector() const;
    std::vector<size_t> GetLocalSizeVector() const;
    const std::vector<ParameterPair>& GetParameterPairs() const;
    const std::vector<ArgumentId>& GetArguments() const;

private:
    std::string m_Name;
    std::string m_DefaultSource;
    std::string m_ConfigurationPrefix;
    DimensionVector m_GlobalSize;
    DimensionVector m_LocalSize;
    std::vector<ParameterPair> m_ParameterPairs;
    std::vector<ArgumentId> m_Arguments;
};

} // namespace ktt