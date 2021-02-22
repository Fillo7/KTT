#include <string>

#include <Api/KttException.h>
#include <KernelRunner/KernelActivator.h>
#include <KernelRunner/KernelRunner.h>
#include <Utility/Logger/Logger.h>
#include <Utility/Timer/Timer.h>

namespace ktt
{

KernelRunner::KernelRunner(ComputeEngine& engine, KernelArgumentManager& argumentManager) :
    m_ComputeLayer(std::make_unique<ComputeLayer>(engine, argumentManager)),
    m_Validator(std::make_unique<ResultValidator>(*this)),
    m_Engine(engine),
    m_ArgumentManager(argumentManager),
    m_TimeUnit(TimeUnit::Milliseconds),
    m_ReadOnlyCacheFlag(true),
    m_ProfilingFlag(false)
{}

KernelResult KernelRunner::RunKernel(const Kernel& kernel, const KernelConfiguration& configuration, const KernelRunMode mode,
    const std::vector<BufferOutputDescriptor>& output)
{
    const auto id = kernel.GetId();

    if (!m_Validator->HasReferenceResult(kernel))
    {
        m_Validator->ComputeReferenceResult(kernel, mode);
    }

    if (mode == KernelRunMode::Running)
    {
        SetupBuffers(kernel);
    }

    Logger::LogInfo("Running kernel " + std::to_string(id) + " with configuration: " + configuration.GetString());
    auto launcher = GetKernelLauncher(kernel);
    KernelResult result = RunKernelInternal(kernel, configuration, launcher, output);
    ValidateResult(kernel, result, mode);

    if (mode == KernelRunMode::Running)
    {
        CleanupBuffers(kernel);
    }

    return result;
}

void KernelRunner::SetupBuffers(const Kernel& kernel)
{
    const auto vectorArguments = kernel.GetVectorArguments();

    for (auto* argument : vectorArguments)
    {
        const auto id = argument->GetId();

        if (argument->GetManagementType() == ArgumentManagementType::User)
        {
            if (!kernel.HasLauncher() && !argument->HasUserBuffer())
            {
                Logger::LogWarning("Kernel argument with id " + std::to_string(id) + " has buffer managed by user, but its "
                    + "associated kernel with id " + std::to_string(kernel.GetId()) + " does not have a launcher defined by user");
            }

            continue;
        }

        if (m_ReadOnlyCacheFlag && argument->GetAccessType() == ArgumentAccessType::ReadOnly && m_Engine.HasBuffer(id))
        {
            continue;
        }

        const auto actionId = m_Engine.UploadArgument(*argument, m_ComputeLayer->GetDefaultQueue());
        m_Engine.WaitForTransferAction(actionId);
    }
}

void KernelRunner::CleanupBuffers(const Kernel& kernel)
{
    const auto vectorArguments = kernel.GetVectorArguments();

    for (const auto* argument : vectorArguments)
    {
        const auto id = argument->GetId();

        if (argument->GetManagementType() == ArgumentManagementType::User)
        {
            continue;
        }

        if (m_ReadOnlyCacheFlag && argument->GetAccessType() == ArgumentAccessType::ReadOnly)
        {
            continue;
        }

        m_Engine.ClearBuffer(id);
    }
}

void KernelRunner::DownloadBuffers(const std::vector<BufferOutputDescriptor>& output)
{
    for (const auto& descriptor : output)
    {
        const auto id = m_Engine.DownloadArgument(descriptor.GetArgumentId(), m_Engine.GetDefaultQueue(),
            descriptor.GetOutputDestination(), descriptor.GetOutputSize());
        m_Engine.WaitForTransferAction(id);
    }
}

void KernelRunner::SetTimeUnit(const TimeUnit unit)
{
    m_TimeUnit = unit;
}

void KernelRunner::SetReadOnlyArgumentCache(const bool flag)
{
    m_ReadOnlyCacheFlag = flag;
}

void KernelRunner::SetProfiling(const bool flag)
{
    m_ProfilingFlag = flag;
}

bool KernelRunner::IsProfilingActive() const
{
    return m_ProfilingFlag;
}

void KernelRunner::SetValidationMethod(const ValidationMethod method, const double toleranceThreshold)
{
    m_Validator->SetValidationMethod(method, toleranceThreshold);
}

void KernelRunner::SetValidationMode(const ValidationMode mode)
{
    m_Validator->SetValidationMode(mode);
}

void KernelRunner::SetValidationRange(const ArgumentId id, const size_t range)
{
    PrepareValidationData(id);
    m_Validator->SetValidationRange(id, range);
}

void KernelRunner::SetValueComparator(const ArgumentId id, ValueComparator comparator)
{
    PrepareValidationData(id);
    m_Validator->SetValueComparator(id, comparator);
}

void KernelRunner::SetReferenceComputation(const ArgumentId id, ReferenceComputation computation)
{
    PrepareValidationData(id);
    m_Validator->SetReferenceComputation(id, computation);
}

void KernelRunner::SetReferenceKernel(const ArgumentId id, const Kernel& kernel, const KernelConfiguration& configuration)
{
    PrepareValidationData(id);
    m_Validator->SetReferenceKernel(id, kernel, configuration);
}

void KernelRunner::ClearReferenceResult(const Kernel& kernel)
{
    m_Validator->ClearReferenceResult(kernel);
}

KernelLauncher KernelRunner::GetKernelLauncher(const Kernel& kernel)
{
    if (kernel.HasLauncher())
    {
        return kernel.GetLauncher();
    }

    if (kernel.IsComposite())
    {
        throw KttException("Composite kernel must have kernel launcher defined by user");
    }

    const auto primaryId = kernel.GetPrimaryDefinition().GetId();
    KernelLauncher result;

    if (IsProfilingActive())
    {
        result = [primaryId](ComputeInterface& interface)
        {
            interface.RunKernelWithProfiling(primaryId);
        };
    }
    else
    {
        result = [primaryId](ComputeInterface& interface)
        {
            interface.RunKernel(primaryId);
        };
    }

    return result;
}

KernelResult KernelRunner::RunKernelInternal(const Kernel& kernel, const KernelConfiguration& configuration,
    KernelLauncher launcher, const std::vector<BufferOutputDescriptor>& output)
{
    m_ComputeLayer->AddData(kernel, configuration);
    const KernelId id = kernel.GetId();

    auto activator = std::make_unique<KernelActivator>(*m_ComputeLayer, id);
    KernelResult result(id, configuration);

    try
    {
        const Nanoseconds duration = RunLauncher(launcher);
        result = m_ComputeLayer->GenerateResult(id, duration);
    }
    catch (const KttException& exception)
    {
        Logger::LogWarning(std::string("Kernel run failed with reason: ") + exception.what());
        
        m_ComputeLayer->SynchronizeDevice();
        m_ComputeLayer->ClearKernelData();

        result.SetStatus(ResultStatus::ComputationFailed);
    }

    DownloadBuffers(output);
    m_ComputeLayer->ClearData(id);

    return result;
}

Nanoseconds KernelRunner::RunLauncher(KernelLauncher launcher)
{
    Timer timer;

    timer.Start();
    launcher(*m_ComputeLayer);
    timer.Stop();

    return timer.GetElapsedTime();
}

void KernelRunner::PrepareValidationData(const ArgumentId id)
{
    if (!m_Validator->HasValidationData(id))
    {
        const auto& argument = m_ArgumentManager.GetArgument(id);
        m_Validator->InitializeValidationData(argument);
    }
}

void KernelRunner::ValidateResult(const Kernel& kernel, KernelResult& result, const KernelRunMode mode)
{
    if (!result.IsValid())
    {
        return;
    }

    const bool validResult = m_Validator->ValidateArguments(kernel, mode);
    const uint64_t duration = Timer::ConvertDuration(result.GetTotalDuration(), m_TimeUnit);
    const uint64_t kernelDuration = Timer::ConvertDuration(result.GetKernelDuration(), m_TimeUnit);
    const std::string tag = Timer::GetTag(m_TimeUnit);

    if (validResult)
    {
        Logger::LogInfo("Kernel run completed successfully in " + std::to_string(duration) + tag
            + ", kernel duration was " + std::to_string(kernelDuration) + tag);
        return;
    }

    Logger::LogWarning("Kernel run completed in " + std::to_string(duration) + tag
        + ", kernel duration was " + std::to_string(kernelDuration) + tag + ", but results differ");
    result.SetStatus(ResultStatus::ValidationFailed);
}

} // namespace ktt
