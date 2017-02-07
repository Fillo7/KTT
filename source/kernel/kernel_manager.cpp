#include <fstream>
#include <sstream>

#include "kernel_manager.h"

namespace ktt
{

KernelManager::KernelManager():
    kernelCount(static_cast<size_t>(0))
{}

size_t KernelManager::addKernel(const std::string& source, const std::string& kernelName, const DimensionVector& globalSize,
    const DimensionVector& localSize)
{
    Kernel kernel(source, kernelName, globalSize, localSize);
    kernels.push_back(std::make_shared<Kernel>(kernel));

    return kernelCount++;
}

size_t KernelManager::addKernelFromFile(const std::string& filename, const std::string& kernelName, const DimensionVector& globalSize,
    const DimensionVector& localSize)
{
    std::string source = loadFileToString(filename);
    return addKernel(source, kernelName, globalSize, localSize);
}

std::string KernelManager::getKernelSourceWithDefines(const size_t id, const KernelConfiguration& kernelConfiguration) const
{
    std::string source = getKernel(id)->getSource();

    for (auto& parameterValue : kernelConfiguration.getParameterValues())
    {
        std::stringstream stream;
        stream << std::get<1>(parameterValue); // clean way to convert number to string
        source = std::string("#define ") + std::get<0>(parameterValue) + std::string(" ") + stream.str() + std::string("\n") + source;
    }

    return source;
}

std::vector<KernelConfiguration> KernelManager::getKernelConfigurations(const size_t id) const
{
    if (id >= kernelCount)
    {
        throw std::runtime_error("Invalid kernel id: " + id);
    }

    std::vector<KernelConfiguration> configurations;
    computeConfigurations(static_cast<size_t>(0), kernels.at(id)->getParameters(), std::vector<ParameterValue>(0), kernels.at(id)->getGlobalSize(),
        kernels.at(id)->getLocalSize(), configurations);
    return configurations;
}

void KernelManager::addParameter(const size_t id, const KernelParameter& parameter)
{
    if (id >= kernelCount)
    {
        throw std::runtime_error("Invalid kernel id: " + id);
    }
    kernels.at(id)->addParameter(parameter);
}

void KernelManager::addArgumentInt(const size_t id, const std::vector<int>& data)
{
    if (id >= kernelCount)
    {
        throw std::runtime_error("Invalid kernel id: " + id);
    }
    kernels.at(id)->addArgumentInt(data);
}

void KernelManager::addArgumentFloat(const size_t id, const std::vector<float>& data)
{
    if (id >= kernelCount)
    {
        throw std::runtime_error("Invalid kernel id: " + id);
    }
    kernels.at(id)->addArgumentFloat(data);
}

void KernelManager::addArgumentDouble(const size_t id, const std::vector<double>& data)
{
    if (id >= kernelCount)
    {
        throw std::runtime_error("Invalid kernel id: " + id);
    }
    kernels.at(id)->addArgumentDouble(data);
}

void KernelManager::useSearchMethod(const size_t id, const SearchMethod& searchMethod, const std::vector<double>& searchArguments)
{
    if (id >= kernelCount)
    {
        throw std::runtime_error("Invalid kernel id: " + id);
    }
    kernels.at(id)->useSearchMethod(searchMethod, searchArguments);
}

size_t KernelManager::getKernelCount() const
{
    return kernelCount;
}

const std::shared_ptr<const Kernel> KernelManager::getKernel(const size_t id) const
{
    if (id >= kernelCount)
    {
        throw std::runtime_error("Invalid kernel id: " + id);
    }
    return kernels.at(id);
}

std::string KernelManager::loadFileToString(const std::string& filename) const
{
    std::ifstream file(filename);
    std::stringstream stream;
    stream << file.rdbuf();
    return stream.str();
}

void KernelManager::computeConfigurations(const size_t currentParameterIndex, const std::vector<KernelParameter>& parameters,
    std::vector<ParameterValue> computedParameterValues, DimensionVector computedGlobalSize, DimensionVector computedLocalSize,
    std::vector<KernelConfiguration>& finalResult) const
{
    if (currentParameterIndex >= parameters.size())
    {
        KernelConfiguration configuration(computedGlobalSize, computedLocalSize, computedParameterValues);
        finalResult.push_back(configuration);
        return;
    }

    KernelParameter parameter = parameters.at(currentParameterIndex);
    for (auto& value : parameter.getValues())
    {
        auto copy = computedParameterValues;
        copy.push_back(ParameterValue(parameter.getName(), value));

        if (parameter.getThreadModifierType() != ThreadModifierType::None)
        {
            // to do: compute correct global / local thread sizes
        }

        computeConfigurations(currentParameterIndex + 1, parameters, copy, computedGlobalSize, computedLocalSize, finalResult);
    }
}

} // namespace ktt
