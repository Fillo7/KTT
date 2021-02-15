#include <algorithm>

#include <Api/Info/DeviceInfo.h>
#include <TuningRunner/TuningRunner.h>
#include <Utility/ErrorHandling/KttException.h>
#include <Utility/Logger/Logger.h>

namespace ktt
{

TuningRunner::TuningRunner(KernelRunner& kernelRunner, const DeviceInfo& info) :
    m_KernelRunner(kernelRunner),
    m_ConfigurationManager(std::make_unique<ConfigurationManager>(info))
{}

std::vector<KernelResult> TuningRunner::Tune(const Kernel& kernel, std::unique_ptr<StopCondition> stopCondition)
{
    if (kernel.HasWritableZeroCopyArgument())
    {
        throw KttException("Offline kernel tuning cannot be performed with writable zero-copy arguments");
    }

    if (stopCondition != nullptr)
    {
        stopCondition->Initialize();
    }

    std::vector<KernelResult> results;
    const auto id = kernel.GetId();
    KernelResult result(id, m_ConfigurationManager->GetCurrentConfiguration(id));

    do 
    {
        Logger::LogInfo("Launching new configuration for kernel " + std::to_string(id));

        do 
        {
            result = TuneIteration(kernel, KernelRunMode::OfflineTuning, std::vector<BufferOutputDescriptor>{}, false);
        }
        while (result.HasRemainingProfilingRuns());

        results.push_back(result);

        if (stopCondition != nullptr)
        {
            stopCondition->Update(result);
            Logger::LogInfo(stopCondition->GetStatusString());

            if (stopCondition->IsFulfilled())
            {
                break;
            }
        }
    }
    while (m_ConfigurationManager->CalculateNextConfiguration(id, result));

    m_KernelRunner.ClearReferenceResult(kernel);
    m_ConfigurationManager->ClearData(id);
    return results;
}

KernelResult TuningRunner::TuneIteration(const Kernel& kernel, const KernelRunMode mode,
    const std::vector<BufferOutputDescriptor>& output, const bool recomputeReference)
{
    if (recomputeReference)
    {
        m_KernelRunner.ClearReferenceResult(kernel);
    }

    const auto id = kernel.GetId();

    if (!m_ConfigurationManager->HasData(id))
    {
        m_ConfigurationManager->InitializeData(kernel);
    }

    m_KernelRunner.SetupBuffers(kernel);
    const auto& currentConfiguration = m_ConfigurationManager->GetCurrentConfiguration(id);
    KernelResult result = m_KernelRunner.RunKernel(kernel, currentConfiguration, mode, output);

    if (mode != KernelRunMode::OfflineTuning)
    {
        m_ConfigurationManager->CalculateNextConfiguration(id, result);
    }

    m_KernelRunner.CleanupBuffers(kernel);
    return result;
}

void TuningRunner::DryTune(const Kernel& kernel, const std::vector<KernelResult>& /*results*/, const uint64_t iterations)
{
    const auto id = kernel.GetId();

    if (!m_ConfigurationManager->HasData(id))
    {
        m_ConfigurationManager->InitializeData(kernel);
    }

    KernelResult result(id, m_ConfigurationManager->GetCurrentConfiguration(id));
    uint64_t passedIterations = 0;

    do
    {
        if (passedIterations >= iterations)
        {
            break;
        }

        //const auto& currentConfiguration = m_ConfigurationManager->GetCurrentConfiguration(id);

        try
        {
            Logger::LogInfo("Launching new configuration for kernel " + std::to_string(id));
            // todo: find corresponding result for configuration
        }
        catch (const KttException& error)
        {
            Logger::LogWarning(std::string("Kernel run failed, reason: ") + error.what());
            result.SetStatus(ResultStatus::ComputationFailed);
        }

        m_ConfigurationManager->CalculateNextConfiguration(id, result);
        ++passedIterations;
    }
    while (m_ConfigurationManager->CalculateNextConfiguration(id, result));

    m_ConfigurationManager->ClearData(id);
}

void TuningRunner::SetSearcher(const KernelId id, std::unique_ptr<Searcher> searcher)
{
    m_ConfigurationManager->SetSearcher(id, std::move(searcher));
}

void TuningRunner::ClearData(const KernelId id)
{
    m_ConfigurationManager->ClearData(id);
}

const KernelConfiguration& TuningRunner::GetBestConfiguration(const KernelId id) const
{
    return m_ConfigurationManager->GetBestConfiguration(id);
}

} // namespace ktt
