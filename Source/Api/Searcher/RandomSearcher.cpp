#include <algorithm>
#include <random>

#include <Api/Searcher/RandomSearcher.h>

namespace ktt
{

RandomSearcher::RandomSearcher() :
    Searcher(),
    m_Index(0)
{}

void RandomSearcher::OnInitialize()
{
    m_ConfigurationIndices.resize(GetConfigurationsCount());

    for (size_t i = 0; i < m_ConfigurationIndices.size(); ++i)
    {
        m_ConfigurationIndices[i] = i;
    }

    std::random_device device;
    std::default_random_engine engine(device());
    std::shuffle(std::begin(m_ConfigurationIndices), std::end(m_ConfigurationIndices), engine);
}

void RandomSearcher::OnReset()
{
    m_Index = 0;
    m_ConfigurationIndices.clear();
}

void RandomSearcher::CalculateNextConfiguration([[maybe_unused]] const KernelResult& previousResult)
{
    ++m_Index;
}

KernelConfiguration RandomSearcher::GetCurrentConfiguration() const
{
    const uint64_t currentIndex = m_ConfigurationIndices[m_Index];
    return GetConfiguration(currentIndex);
}

} // namespace ktt
