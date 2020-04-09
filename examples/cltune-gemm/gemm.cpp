#include <cstdint>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <vector>
#include "tuner_api.h"

#if defined(_MSC_VER)
    const std::string kernelFilePrefix = "";
#else
    const std::string kernelFilePrefix = "../";
#endif

#if KTT_CUDA_EXAMPLE
    const std::string defaultKernelFile = kernelFilePrefix + "../examples/cltune-gemm/gemm.cu";
    const std::string defaultReferenceKernelFile = kernelFilePrefix + "../examples/cltune-gemm/gemm_reference.cu";
    const auto computeAPI = ktt::ComputeAPI::CUDA;
#elif KTT_OPENCL_EXAMPLE
    const std::string defaultKernelFile = kernelFilePrefix + "../examples/cltune-gemm/gemm.cl";
    const std::string defaultReferenceKernelFile = kernelFilePrefix + "../examples/cltune-gemm/gemm_reference.cl";
    const auto computeAPI = ktt::ComputeAPI::OpenCL;
#endif

#define RAPID_TEST 0
#define USE_PROFILING 1
#define USE_REDUCED_SET 0 /* reduced tuning parameters set, taken from CLTune */

// Helper function to determine whether or not 'a' is a multiple of 'b'
bool IsMultiple(const size_t a, const size_t b) {
    return ((a / b) * b == a) ? true : false;
};

int main(int argc, char** argv)
{
    // Initialize platform and device index
    ktt::PlatformIndex platformIndex = 0;
    ktt::DeviceIndex deviceIndex = 0;
    std::string kernelFile = defaultKernelFile;
    std::string referenceKernelFile = defaultReferenceKernelFile;

    if (argc >= 2)
    {
        platformIndex = std::stoul(std::string(argv[1]));
        if (argc >= 3)
        {
            deviceIndex = std::stoul(std::string(argv[2]));
            if (argc >= 4)
            {
                kernelFile = std::string(argv[3]);
                if (argc >= 5)
                {
                    referenceKernelFile = std::string(argv[4]);
                }
            }
        }
    }

    // Declare data variables
    #if USE_PROFILING == 0
    const uint32_t kSizeM = 2048/2;
    const uint32_t kSizeN = 2048/2;
    const uint32_t kSizeK = 2048/2;
    #else
    const uint32_t kSizeM = 2048/2;
    const uint32_t kSizeN = 2048/2;
    const uint32_t kSizeK = 2048/2;
    #endif

    const ktt::DimensionVector ndRangeDimensions(kSizeM, kSizeN);
    const ktt::DimensionVector workGroupDimensions;
    const ktt::DimensionVector referenceWorkGroupDimensions(8, 8);

    // Initialize data
    std::random_device device;
    std::default_random_engine engine(device());
    std::uniform_real_distribution<float> distribution(-2.0f, 2.0f);

    std::vector<float> mat_a(kSizeM * kSizeK);
    std::vector<float> mat_b(kSizeN * kSizeK);
    std::vector<float> mat_c(kSizeM * kSizeN);
    for (uint32_t i = 0; i < kSizeM * kSizeK; i++)
        mat_a.at(i) = distribution(engine);
    for (uint32_t i = 0; i < kSizeN * kSizeK; i++)
        mat_b.at(i) = distribution(engine);
    for (uint32_t i = 0; i < kSizeM * kSizeN; i++)
        mat_c.at(i) = 0.0f;

    // Create tuner object for chosen platform and device
    ktt::Tuner tuner(platformIndex, deviceIndex, computeAPI);
    tuner.setGlobalSizeType(ktt::GlobalSizeType::OpenCL);
    tuner.setPrintingTimeUnit(ktt::TimeUnit::Microseconds);

    #if USE_PROFILING == 1
    printf("Executing with profiling switched ON.\n");
    tuner.setKernelProfiling(true);
    #endif

    // Add two kernels to tuner, one of the kernels acts as reference kernel
    ktt::KernelId kernelId = tuner.addKernelFromFile(kernelFile, "gemm_fast", ndRangeDimensions, workGroupDimensions);
    ktt::KernelId referenceKernelId = tuner.addKernelFromFile(referenceKernelFile, "gemm_reference", ndRangeDimensions, referenceWorkGroupDimensions);

    #if USE_REDUCED_SET == 1
    tuner.addParameter(kernelId, "MWG", {16, 32, 64});
    tuner.addParameter(kernelId, "NWG", {16, 32, 64});
    tuner.addParameter(kernelId, "KWG", {32});
    tuner.addParameter(kernelId, "MDIMC", {8, 16, 32});
    tuner.addParameter(kernelId, "NDIMC", {8, 16, 32});
    tuner.addParameter(kernelId, "MDIMA", {8, 16, 32});
    tuner.addParameter(kernelId, "NDIMB", {8, 16, 32});
    tuner.addParameter(kernelId, "KWI", {2});
    tuner.addParameter(kernelId, "VWM", {1, 2, 4});
    tuner.addParameter(kernelId, "VWN", {1, 2, 4});
    tuner.addParameter(kernelId, "STRM", {0});
    tuner.addParameter(kernelId, "STRN", {0});
    tuner.addParameter(kernelId, "SA", {0, 1});
    tuner.addParameter(kernelId, "SB", {0, 1});
    tuner.addParameter(kernelId, "PRECISION", {32});
    #else
    tuner.addParameter(kernelId, "MWG", {16, 32, 64, 128});
    tuner.addParameter(kernelId, "NWG", {16, 32, 64, 128});
    tuner.addParameter(kernelId, "KWG", {16, 32});
    tuner.addParameter(kernelId, "MDIMC", {8, 16, 32});
    tuner.addParameter(kernelId, "NDIMC", {8, 16, 32});
    tuner.addParameter(kernelId, "MDIMA", {8, 16, 32});
    tuner.addParameter(kernelId, "NDIMB", {8, 16, 32});
    tuner.addParameter(kernelId, "KWI", {2, 8});

    if (computeAPI == ktt::ComputeAPI::OpenCL)
    {
        tuner.addParameter(kernelId, "VWM", {1, 2, 4, 8});
        tuner.addParameter(kernelId, "VWN", {1, 2, 4, 8});
    }
    else
    {
        tuner.addParameter(kernelId, "VWM", {1, 2, 4});
        tuner.addParameter(kernelId, "VWN", {1, 2, 4});
    }

    tuner.addParameter(kernelId, "STRM", {0, 1});
    tuner.addParameter(kernelId, "STRN", {0, 1});
    tuner.addParameter(kernelId, "SA", {0, 1});
    tuner.addParameter(kernelId, "SB", {0, 1});
    tuner.addParameter(kernelId, "PRECISION", {32});
    #endif

    // Add kernel dimension modifiers based on added tuning parameters
    auto globalModifier = [](const size_t size, const std::vector<size_t>& vector) {return size * vector.at(0) / vector.at(1);};
    tuner.setThreadModifier(kernelId, ktt::ModifierType::Global, ktt::ModifierDimension::X, std::vector<std::string>{"MDIMC", "MWG"}, globalModifier);
    tuner.setThreadModifier(kernelId, ktt::ModifierType::Global, ktt::ModifierDimension::Y, std::vector<std::string>{"NDIMC", "NWG"}, globalModifier);

    tuner.setThreadModifier(kernelId, ktt::ModifierType::Local, ktt::ModifierDimension::X, "MDIMC", ktt::ModifierAction::Multiply);
    tuner.setThreadModifier(kernelId, ktt::ModifierType::Local, ktt::ModifierDimension::Y, "NDIMC", ktt::ModifierAction::Multiply);

    // Add all arguments utilized by kernels
    ktt::ArgumentId kSizeMId = tuner.addArgumentScalar(kSizeM);
    ktt::ArgumentId kSizeNId = tuner.addArgumentScalar(kSizeN);
    ktt::ArgumentId kSizeKId = tuner.addArgumentScalar(kSizeK);
    ktt::ArgumentId matAId = tuner.addArgumentVector(mat_a, ktt::ArgumentAccessType::ReadOnly);
    ktt::ArgumentId matBId = tuner.addArgumentVector(mat_b, ktt::ArgumentAccessType::ReadOnly);
    ktt::ArgumentId matCId = tuner.addArgumentVector(mat_c, ktt::ArgumentAccessType::WriteOnly);

#if RAPID_TEST == 1
    tuner.persistArgument(matAId, true);
    tuner.persistArgument(matBId, true);
    tuner.persistArgument(matCId, true);
#endif

    // Add conditions
    // Sets constraints: Set-up the constraints functions to use. The constraints require a function
    // object (in this case a lambda) which takes a vector of tuning parameter values and returns
    // a boolean value whether or not the tuning configuration is legal. In this case, the helper
    // function 'IsMultiple' is employed for convenience. In the calls to 'AddConstraint' below, the
    // vector of parameter names (as strings) matches the input integer vector of the lambda's.
    auto MultipleOfX = [](const std::vector<size_t>& v) {return IsMultiple(v[0], v[1]);};
    auto MultipleOfXMulY = [](const std::vector<size_t>& v) {return IsMultiple(v[0], v[1] * v[2]);};
    auto MultipleOfXMulYDivZ = [](const std::vector<size_t>& v) {return IsMultiple(v[0], (v[1] * v[2]) / v[3]);};

    // Sets constraints: Requirement for unrolling the KWG loop
    tuner.addConstraint(kernelId, {"KWG", "KWI"}, MultipleOfX);

    // Sets constraints: Required for integer MWI and NWI
    tuner.addConstraint(kernelId, {"MWG", "MDIMC", "VWM"}, MultipleOfXMulY);
    tuner.addConstraint(kernelId, {"NWG", "NDIMC", "VWN"}, MultipleOfXMulY);

    // Sets constraints: Required for integer MWIA and NWIB
    tuner.addConstraint(kernelId, {"MWG", "MDIMA", "VWM"}, MultipleOfXMulY);
    tuner.addConstraint(kernelId, {"NWG", "NDIMB", "VWN"}, MultipleOfXMulY);

    // Sets constraints: KWG has to be a multiple of KDIMA = ((MDIMC*NDIMC)/(MDIMA)) and KDIMB = (...)
    tuner.addConstraint(kernelId, {"KWG", "MDIMC", "NDIMC", "MDIMA"}, MultipleOfXMulYDivZ);
    tuner.addConstraint(kernelId, {"KWG", "MDIMC", "NDIMC", "NDIMB"}, MultipleOfXMulYDivZ);

    // Set kernel arguments for both tuned kernel and reference kernel, order of arguments is important
    tuner.setKernelArguments(kernelId, std::vector<ktt::ArgumentId>{kSizeMId, kSizeNId, kSizeKId, matAId, matBId, matCId});
    tuner.setKernelArguments(referenceKernelId, std::vector<ktt::ArgumentId>{kSizeMId, kSizeNId, kSizeKId, matAId, matBId, matCId});

#if RAPID_TEST == 0
    // Specify custom tolerance threshold for validation of floating point arguments. Default threshold is 1e-4.
    tuner.setValidationMethod(ktt::ValidationMethod::SideBySideComparison, 0.001);

    // Set reference kernel which validates results provided by tuned kernel, provide list of arguments which will be validated
    tuner.setReferenceKernel(kernelId, referenceKernelId, std::vector<ktt::ParameterPair>{}, std::vector<ktt::ArgumentId>{matCId});
#endif

    // Set and configure searcher
    unsigned int ccMajor = tuner.getCurrentDeviceInfo().getCUDAComputeCapabilityMajor();
    unsigned int ccMinor = tuner.getCurrentDeviceInfo().getCUDAComputeCapabilityMinor();
    tuner.setSearchMethod(ktt::SearchMethod::ProfileSearch, std::vector< double > {ccMajor + 0.1*(double)ccMinor});

    // Launch kernel tuning
    tuner.tuneKernel(kernelId, std::unique_ptr<ktt::ConfigurationCount>(new ktt::ConfigurationCount(20)));

    // Print tuning results to standard output and to output.csv file
    tuner.printResult(kernelId, std::cout, ktt::PrintFormat::Verbose);
    tuner.printResult(kernelId, "gemm_output.csv", ktt::PrintFormat::CSV);

    return 0;
};
