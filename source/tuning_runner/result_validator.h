#pragma once

#include <cmath>
#include <iostream>
#include <map>
#include <type_traits>
#include <vector>

#include "../enum/validation_method.h"
#include "../kernel_argument/kernel_argument.h"
#include "../utility/logger.h"

namespace ktt
{

class ResultValidator
{
public:
    // Constructor
    ResultValidator(Logger* logger);

    // Core methods
    bool validateArgumentWithClass(const size_t kernelId, const std::vector<KernelArgument>& resultArguments) const;
    bool validateArgumentWithKernel(const size_t kernelId, const std::vector<KernelArgument>& resultArguments) const;
    void setReferenceClassResult(const size_t kernelId, const std::vector<KernelArgument>& classResult);
    void setReferenceKernelResult(const size_t kernelId, const std::vector<KernelArgument>& kernelResult);
    bool hasReferenceClassResult(const size_t kernelId) const;
    bool hasReferenceKernelResult(const size_t kernelId) const;
    void clearReferenceResults();

    // Setters
    void setToleranceThreshold(const double toleranceThreshold);
    void setValidationMethod(const ValidationMethod& validationMethod);

    // Getters
    double getToleranceThreshold() const;
    ValidationMethod getValidationMethod() const;

private:
    // Attributes
    double toleranceThreshold;
    ValidationMethod validationMethod;
    std::map<size_t, std::vector<KernelArgument>> referenceClassResultMap;
    std::map<size_t, std::vector<KernelArgument>> referenceKernelResultMap;
    Logger* logger;

    // Helper methods
    bool validateArguments(const std::vector<KernelArgument>& resultArguments, const std::vector<KernelArgument>& referenceArguments) const;
    KernelArgument findArgument(const size_t argumentId, const std::vector<KernelArgument>& arguments) const;

    template <typename T> bool validateResult(const std::vector<T>& result, const std::vector<T>& referenceResult) const
    {
        if (result.size() != referenceResult.size())
        {
            logger->log(std::string("Number of elements in results differs, reference size: ") + std::to_string(referenceResult.size())
                + "; result size: " + std::to_string(result.size()));
            return false;
        }
        return validateResultInner(result, referenceResult, std::is_floating_point<T>());
    }

    template <typename T> bool validateResultInner(const std::vector<T>& result, const std::vector<T>& referenceResult,
        std::true_type) const
    {
        if (validationMethod == ValidationMethod::AbsoluteDifference)
        {
            double difference = 0.0;
            for (size_t i = 0; i < result.size(); i++)
            {
                difference += std::fabs(result.at(i) - referenceResult.at(i));
            }
            if (difference > toleranceThreshold)
            {
                logger->log(std::string("Results differ, absolute difference is: ") + std::to_string(difference));
                return false;
            }
            return true;
        }
        else
        {
            for (size_t i = 0; i < result.size(); i++)
            {
                if (std::fabs(result.at(i) - referenceResult.at(i)) > toleranceThreshold)
                {
                    logger->log(std::string("Results differ at index ") + std::to_string(i) + "; reference value: "
                        + std::to_string(referenceResult.at(i)) + "; result value: " + std::to_string(result.at(i)));
                    return false;
                }
            }
            return true;
        }
    }

    template <typename T> bool validateResultInner(const std::vector<T>& result, const std::vector<T>& referenceResult,
        std::false_type) const
    {
        for (size_t i = 0; i < result.size(); i++)
        {
            if (result.at(i) != referenceResult.at(i))
            {
                logger->log(std::string("Results differ at index ") + std::to_string(i) + "; reference value: "
                    + std::to_string(referenceResult.at(i)) + "; result value: " + std::to_string(result.at(i)));
                return false;
            }
        }
        return true;
    }
};

} // namespace ktt