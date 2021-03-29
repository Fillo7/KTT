/** @file RandomSearcher.h
  * Searcher which explores configurations in random order.
  */
#pragma once

#include <set>

#include <Api/Searcher/Searcher.h>
#include <KttPlatform.h>

namespace ktt
{

/** @class RandomSearcher
  * Searcher which explores configurations in random order.
  */
class KTT_API RandomSearcher : public Searcher
{
public:
    /** @fn RandomSearcher()
      * Initializes random searcher.
       */
    RandomSearcher();

    void OnInitialize() override;
    void OnReset() override;

    void CalculateNextConfiguration(const KernelResult& previousResult) override;
    KernelConfiguration GetCurrentConfiguration() const override;

private:
    std::set<uint64_t> m_ExploredIndices;
    uint64_t m_CurrentIndex;
};

} // namespace ktt
