#include <Utility/ErrorHandling/Assert.h>
#include <Utility/ErrorHandling/KttException.h>
#include <Tuner.h>
#include <TunerCore.h>

namespace ktt
{

Tuner::Tuner(const PlatformIndex platform, const DeviceIndex device, const ComputeApi api) :
    m_Tuner(std::make_unique<TunerCore>(platform, device, api, 1))
{}

Tuner::Tuner(const PlatformIndex platform, const DeviceIndex device, const ComputeApi api, const uint32_t computeQueueCount) :
    m_Tuner(std::make_unique<TunerCore>(platform, device, api, computeQueueCount))
{}

Tuner::Tuner(const ComputeApi api, const ComputeApiInitializer& initializer) :
    m_Tuner(std::make_unique<TunerCore>(api, initializer))
{}

Tuner::~Tuner() = default;

KernelDefinitionId Tuner::AddKernelDefinition(const std::string& name, const std::string& source, const DimensionVector& globalSize,
    const DimensionVector& localSize)
{
    try
    {
        return m_Tuner->AddKernelDefinition(name, source, globalSize, localSize);
    }
    catch (const KttException& exception)
    {
        TunerCore::Log(LoggingLevel::Error, exception.what());
        return InvalidKernelDefinitionId;
    }
}

KernelDefinitionId Tuner::AddKernelDefinitionFromFile(const std::string& name, const std::string& filePath,
    const DimensionVector& globalSize, const DimensionVector& localSize)
{
    try
    {
        return m_Tuner->AddKernelDefinitionFromFile(name, filePath, globalSize, localSize);
    }
    catch (const KttException& exception)
    {
        TunerCore::Log(LoggingLevel::Error, exception.what());
        return InvalidKernelDefinitionId;
    }
}

void Tuner::SetArguments(const KernelDefinitionId id, const std::vector<ArgumentId>& argumentIds)
{
    try
    {
        m_Tuner->SetArguments(id, argumentIds);
    }
    catch (const KttException& exception)
    {
        TunerCore::Log(LoggingLevel::Error, exception.what());
    }
}

KernelId Tuner::CreateSimpleKernel(const KernelDefinitionId definitionId)
{
    try
    {
        return m_Tuner->CreateKernel(definitionId);
    }
    catch (const KttException& exception)
    {
        TunerCore::Log(LoggingLevel::Error, exception.what());
        return InvalidKernelId;
    }
}

KernelId Tuner::CreateCompositeKernel(const std::vector<KernelDefinitionId>& definitionIds, KernelLauncher launcher)
{
    try
    {
        return m_Tuner->CreateKernel(definitionIds, launcher);
    }
    catch (const KttException& exception)
    {
        TunerCore::Log(LoggingLevel::Error, exception.what());
        return InvalidKernelId;
    }
}

void Tuner::SetLauncher(const KernelId id, KernelLauncher launcher)
{
    try
    {
        m_Tuner->SetLauncher(id, launcher);
    }
    catch (const KttException& exception)
    {
        TunerCore::Log(LoggingLevel::Error, exception.what());
    }
}

void Tuner::AddParameter(const KernelId id, const std::string& name, const std::vector<uint64_t>& values, const std::string& group)
{
    try
    {
        m_Tuner->AddParameter(id, name, values, group);
    }
    catch (const KttException& exception)
    {
        TunerCore::Log(LoggingLevel::Error, exception.what());
    }
}

void Tuner::AddParameter(const KernelId id, const std::string& name, const std::vector<double>& values, const std::string& group)
{
    try
    {
        m_Tuner->AddParameter(id, name, values, group);
    }
    catch (const KttException& exception)
    {
        TunerCore::Log(LoggingLevel::Error, exception.what());
    }
}

void Tuner::SetThreadModifier(const KernelId id, const ModifierType type, const ModifierDimension dimension,
    const std::vector<std::string>& parameters, const std::vector<KernelDefinitionId>& definitionIds,
    std::function<uint64_t(const uint64_t, const std::vector<uint64_t>&)> function)
{
    try
    {
        m_Tuner->SetThreadModifier(id, type, dimension, parameters, definitionIds, function);
    }
    catch (const KttException& exception)
    {
        TunerCore::Log(LoggingLevel::Error, exception.what());
    }
}

void Tuner::SetThreadModifier(const KernelId id, const ModifierType type, const ModifierDimension dimension,
    const std::string& parameter, const std::vector<KernelDefinitionId>& definitionIds, const ModifierAction action)
{
    std::function<uint64_t(const uint64_t, const std::vector<uint64_t>&)> function;

    switch (action)
    {
    case ModifierAction::Add:
        function = [](const uint64_t size, const std::vector<uint64_t>& parameters)
        {
            return size + parameters[0];
        };
        break;
    case ModifierAction::Subtract:
        function = [](const uint64_t size, const std::vector<uint64_t>& parameters)
        {
            return size - parameters[0];
        };
        break;
    case ModifierAction::Multiply:
        function = [](const uint64_t size, const std::vector<uint64_t>& parameters)
        {
            return size * parameters[0];
        };
        break;
    case ModifierAction::Divide:
        function = [](const uint64_t size, const std::vector<uint64_t>& parameters)
        {
            return size / parameters[0];
        };
        break;
    case ModifierAction::DivideCeil:
        function = [](const uint64_t size, const std::vector<uint64_t>& parameters)
        {
            return (size + parameters[0] - 1) / parameters[0];
        };
        break;
    default:
        KttError("Unhandled modifier action value");
    }

    SetThreadModifier(id, type, dimension, std::vector<std::string>{parameter}, definitionIds, function);
}

void Tuner::AddConstraint(const KernelId id, const std::vector<std::string>& parameters,
    std::function<bool(const std::vector<size_t>&)> function)
{
    try
    {
        m_Tuner->AddConstraint(id, parameters, function);
    }
    catch (const KttException& exception)
    {
        TunerCore::Log(LoggingLevel::Error, exception.what());
    }
}

void Tuner::SetProfiledDefinitions(const KernelId id, const std::vector<KernelDefinitionId>& definitionIds)
{
    try
    {
        m_Tuner->SetProfiledDefinitions(id, definitionIds);
    }
    catch (const KttException& exception)
    {
        TunerCore::Log(LoggingLevel::Error, exception.what());
    }
}

KernelResult Tuner::RunKernel(const KernelId id, const KernelConfiguration& configuration,
    const std::vector<BufferOutputDescriptor>& output)
{
    try
    {
        return m_Tuner->RunKernel(id, configuration, output);
    }
    catch (const KttException& exception)
    {
        TunerCore::Log(LoggingLevel::Error, exception.what());
        return KernelResult(id, configuration);
    }
}

void Tuner::SetProfiling(const bool flag)
{
    try
    {
        m_Tuner->SetProfiling(flag);
    }
    catch (const KttException& exception)
    {
        TunerCore::Log(LoggingLevel::Error, exception.what());
    }
}

void Tuner::SetValidationMethod(const ValidationMethod method, const double toleranceThreshold)
{
    try
    {
        m_Tuner->SetValidationMethod(method, toleranceThreshold);
    }
    catch (const KttException& exception)
    {
        TunerCore::Log(LoggingLevel::Error, exception.what());
    }
}

void Tuner::SetValidationMode(const ValidationMode mode)
{
    try
    {
        m_Tuner->SetValidationMode(mode);
    }
    catch (const KttException& exception)
    {
        TunerCore::Log(LoggingLevel::Error, exception.what());
    }
}

void Tuner::SetValidationRange(const ArgumentId id, const size_t range)
{
    try
    {
        m_Tuner->SetValidationRange(id, range);
    }
    catch (const KttException& exception)
    {
        TunerCore::Log(LoggingLevel::Error, exception.what());
    }
}

void Tuner::SetValueComparator(const ArgumentId id, ValueComparator comparator)
{
    try
    {
        m_Tuner->SetValueComparator(id, comparator);
    }
    catch (const KttException& exception)
    {
        TunerCore::Log(LoggingLevel::Error, exception.what());
    }
}

void Tuner::SetReferenceComputation(const ArgumentId id, ReferenceComputation computation)
{
    try
    {
        m_Tuner->SetReferenceComputation(id, computation);
    }
    catch (const KttException& exception)
    {
        TunerCore::Log(LoggingLevel::Error, exception.what());
    }
}

void Tuner::SetReferenceKernel(const ArgumentId id, const KernelId referenceId, const KernelConfiguration& configuration)
{
    try
    {
        m_Tuner->SetReferenceKernel(id, referenceId, configuration);
    }
    catch (const KttException& exception)
    {
        TunerCore::Log(LoggingLevel::Error, exception.what());
    }
}

std::vector<KernelResult> Tuner::TuneKernel(const KernelId id)
{
    return TuneKernel(id, nullptr);
}

std::vector<KernelResult> Tuner::TuneKernel(const KernelId id, std::unique_ptr<StopCondition> stopCondition)
{
    try
    {
        return m_Tuner->TuneKernel(id, std::move(stopCondition));
    }
    catch (const KttException& exception)
    {
        TunerCore::Log(LoggingLevel::Error, exception.what());
        return std::vector<KernelResult>{};
    }
}

KernelResult Tuner::TuneKernelIteration(const KernelId id, const std::vector<BufferOutputDescriptor>& output,
    const bool recomputeReference)
{
    try
    {
        return m_Tuner->TuneKernelIteration(id, output, recomputeReference);
    }
    catch (const KttException& exception)
    {
        TunerCore::Log(LoggingLevel::Error, exception.what());
        return KernelResult(id, KernelConfiguration());
    }
}

void Tuner::DryTuneKernel(const KernelId id, const std::vector<KernelResult>& results, const uint64_t iterations)
{
    try
    {
        m_Tuner->DryTuneKernel(id, results, iterations);
    }
    catch (const KttException& exception)
    {
        TunerCore::Log(LoggingLevel::Error, exception.what());
    }
}

void Tuner::SetSearcher(const KernelId id, std::unique_ptr<Searcher> searcher)
{
    try
    {
        m_Tuner->SetSearcher(id, std::move(searcher));
    }
    catch (const KttException& exception)
    {
        TunerCore::Log(LoggingLevel::Error, exception.what());
    }
}

void Tuner::ClearData(const KernelId id)
{
    try
    {
        m_Tuner->ClearData(id);
    }
    catch (const KttException& exception)
    {
        TunerCore::Log(LoggingLevel::Error, exception.what());
    }
}

const KernelConfiguration& Tuner::GetBestConfiguration(const KernelId id) const
{
    try
    {
        return m_Tuner->GetBestConfiguration(id);
    }
    catch (const KttException& exception)
    {
        TunerCore::Log(LoggingLevel::Error, exception.what());

        static KernelConfiguration invalidConfiguration;
        return invalidConfiguration;
    }
}

KernelConfiguration Tuner::CreateConfiguration(const KernelId id, const ParameterInput& parameters) const
{
    try
    {
        return m_Tuner->CreateConfiguration(id, parameters);
    }
    catch (const KttException& exception)
    {
        TunerCore::Log(LoggingLevel::Error, exception.what());
        return KernelConfiguration();
    }
}

std::string Tuner::GetKernelSource(const KernelId id, const KernelConfiguration& configuration) const
{
    try
    {
        return m_Tuner->GetKernelSource(id, configuration);
    }
    catch (const KttException& exception)
    {
        TunerCore::Log(LoggingLevel::Error, exception.what());
        return "";
    }
}

std::string Tuner::GetKernelDefinitionSource(const KernelDefinitionId id, const KernelConfiguration& configuration) const
{
    try
    {
        return m_Tuner->GetKernelDefinitionSource(id, configuration);
    }
    catch (const KttException& exception)
    {
        TunerCore::Log(LoggingLevel::Error, exception.what());
        return "";
    }
}

void Tuner::SetTimeUnit(const TimeUnit unit)
{
    try
    {
        m_Tuner->SetTimeUnit(unit);
    }
    catch (const KttException& exception)
    {
        TunerCore::Log(LoggingLevel::Error, exception.what());
    }
}

void Tuner::SaveResults(const std::vector<KernelResult>& /*results*/, const std::string& /*filePath*/, const OutputFormat /*format*/) const
{
    try
    {
        // Todo: implement new result serialization
        //m_Tuner->SaveResults(results, filePath, format);
    }
    catch (const KttException& exception)
    {
        TunerCore::Log(LoggingLevel::Error, exception.what());
    }
}

void Tuner::SetProfilingCounters(const std::vector<std::string>& counters)
{
    try
    {
        m_Tuner->SetProfilingCounters(counters);
    }
    catch (const KttException& exception)
    {
        TunerCore::Log(LoggingLevel::Error, exception.what());
    }
}

void Tuner::SetCompilerOptions(const std::string& options)
{
    try
    {
        m_Tuner->SetCompilerOptions(options);
    }
    catch (const KttException& exception)
    {
        TunerCore::Log(LoggingLevel::Error, exception.what());
    }
}

void Tuner::SetGlobalSizeType(const GlobalSizeType type)
{
    try
    {
        m_Tuner->SetGlobalSizeType(type);
    }
    catch (const KttException& exception)
    {
        TunerCore::Log(LoggingLevel::Error, exception.what());
    }
}

void Tuner::SetAutomaticGlobalSizeCorrection(const bool flag)
{
    try
    {
        m_Tuner->SetAutomaticGlobalSizeCorrection(flag);
    }
    catch (const KttException& exception)
    {
        TunerCore::Log(LoggingLevel::Error, exception.what());
    }
}

void Tuner::SetKernelCacheCapacity(const uint64_t capacity)
{
    try
    {
        m_Tuner->SetKernelCacheCapacity(capacity);
    }
    catch (const KttException& exception)
    {
        TunerCore::Log(LoggingLevel::Error, exception.what());
    }
}

std::vector<PlatformInfo> Tuner::GetPlatformInfo() const
{
    try
    {
        return m_Tuner->GetPlatformInfo();
    }
    catch (const KttException& exception)
    {
        TunerCore::Log(LoggingLevel::Error, exception.what());
        return std::vector<PlatformInfo>{};
    }
}

std::vector<DeviceInfo> Tuner::GetDeviceInfo(const PlatformIndex platform) const
{
    try
    {
        return m_Tuner->GetDeviceInfo(platform);
    }
    catch (const KttException& exception)
    {
        TunerCore::Log(LoggingLevel::Error, exception.what());
        return std::vector<DeviceInfo>{};
    }
}

DeviceInfo Tuner::GetCurrentDeviceInfo() const
{
    try
    {
        return m_Tuner->GetCurrentDeviceInfo();
    }
    catch (const KttException& exception)
    {
        TunerCore::Log(LoggingLevel::Error, exception.what());
        return DeviceInfo(0, "");
    }
}

void Tuner::SetLoggingLevel(const LoggingLevel level)
{
    TunerCore::SetLoggingLevel(level);
}

void Tuner::SetLoggingTarget(std::ostream& outputTarget)
{
    TunerCore::SetLoggingTarget(outputTarget);
}

void Tuner::SetLoggingTarget(const std::string& filePath)
{
    TunerCore::SetLoggingTarget(filePath);
}

ArgumentId Tuner::AddArgumentWithReferencedData(const size_t elementSize, const ArgumentDataType dataType,
    const ArgumentMemoryLocation memoryLocation, const ArgumentAccessType accessType, const ArgumentMemoryType memoryType,
    const ArgumentManagementType managementType, void* data, const size_t dataSize)
{
    try
    {
        return m_Tuner->AddArgumentWithReferencedData(elementSize, dataType, memoryLocation, accessType, memoryType, managementType,
            data, dataSize);
    }
    catch (const KttException& exception)
    {
        TunerCore::Log(LoggingLevel::Error, exception.what());
        return InvalidArgumentId;
    }
}

ArgumentId Tuner::AddArgumentWithOwnedData(const size_t elementSize, const ArgumentDataType dataType,
    const ArgumentMemoryLocation memoryLocation, const ArgumentAccessType accessType, const ArgumentMemoryType memoryType,
    const ArgumentManagementType managementType, const void* data, const size_t dataSize)
{
    try
    {
        return m_Tuner->AddArgumentWithOwnedData(elementSize, dataType, memoryLocation, accessType, memoryType, managementType,
            data, dataSize);
    }
    catch (const KttException& exception)
    {
        TunerCore::Log(LoggingLevel::Error, exception.what());
        return InvalidArgumentId;
    }
}

ArgumentId Tuner::AddUserArgument(ComputeBuffer buffer, const size_t elementSize, const ArgumentDataType dataType,
    const ArgumentMemoryLocation memoryLocation, const ArgumentAccessType accessType, const size_t dataSize)
{
    try
    {
        return m_Tuner->AddUserArgument(buffer, elementSize, dataType, memoryLocation, accessType, dataSize);
    }
    catch (const KttException& exception)
    {
        TunerCore::Log(LoggingLevel::Error, exception.what());
        return InvalidArgumentId;
    }
}

} // namespace ktt
