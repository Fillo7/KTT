#pragma once

#include <memory>
#include <utility>
#include <vector>

#include "manipulator_interface_implementation.h"
#include "result_validator.h"
#include "searcher/searcher.h"
#include "../compute_api_driver/compute_api_driver.h"
#include "../dto/tuning_result.h"
#include "../kernel/kernel_manager.h"
#include "../kernel_argument/argument_manager.h"
#include "../utility/argument_printer.h"
#include "../utility/logger.h"

namespace ktt
{

class TuningRunner
{
public:
    // Constructor
    explicit TuningRunner(ArgumentManager* argumentManager, KernelManager* kernelManager, Logger* logger, ComputeApiDriver* computeApiDriver);

    // Core methods
    std::pair<std::vector<TuningResult>, std::vector<TuningResult>> tuneKernel(const size_t id);
    void setValidationMethod(const ValidationMethod& validationMethod, const double toleranceThreshold);
    void enableArgumentPrinting(const size_t argumentId, const std::string& filePath, const ArgumentPrintCondition& argumentPrintCondition);

private:
    // Attributes
    ArgumentManager* argumentManager;
    KernelManager* kernelManager;
    Logger* logger;
    ComputeApiDriver* computeApiDriver;
    ResultValidator resultValidator;
    ArgumentPrinter argumentPrinter;
    std::unique_ptr<ManipulatorInterfaceImplementation> manipulatorInterfaceImplementation;

    // Helper methods
    std::pair<KernelRunResult, uint64_t> runKernel(Kernel* kernel, const KernelConfiguration& currentConfiguration,
        const size_t currentConfigurationIndex, const size_t configurationsCount);
    std::pair<KernelRunResult, uint64_t> runKernelWithManipulator(TuningManipulator* manipulator,
        const std::vector<std::pair<size_t, KernelRuntimeData>>& kernelDataVector, const KernelConfiguration& currentConfiguration);
    std::unique_ptr<Searcher> getSearcher(const SearchMethod& searchMethod, const std::vector<double>& searchArguments,
        const std::vector<KernelConfiguration>& configurations, const std::vector<KernelParameter>& parameters) const;
    std::vector<KernelArgument> getKernelArguments(const size_t kernelId) const;
    std::vector<std::pair<size_t, KernelRuntimeData>> getKernelDataVector(const size_t tunedKernelId, const KernelRuntimeData& tunedKernelData,
        const std::vector<std::pair<size_t, ThreadSizeUsage>>& additionalKernelData, const KernelConfiguration& currentConfiguration) const;
    bool processResult(const Kernel* kernel, const KernelRunResult& result, const uint64_t manipulatorDuration,
        const KernelConfiguration& kernelConfiguration);
    bool validateResult(const Kernel* kernel, const KernelRunResult& result);
    bool validateResult(const Kernel* kernel, const KernelRunResult& result, bool useReferenceClass);
    std::vector<KernelArgument> getReferenceResultFromClass(const ReferenceClass* referenceClass,
        const std::vector<size_t>& referenceArgumentIndices) const;
    std::vector<KernelArgument> getReferenceResultFromKernel(const size_t referenceKernelId,
        const std::vector<ParameterValue>& referenceKernelConfiguration, const std::vector<size_t>& referenceArgumentIndices) const;
};

} // namespace ktt
