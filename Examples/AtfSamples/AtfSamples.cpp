#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

#include <Ktt.h>

#if defined(_MSC_VER)
const std::string kernelPrefix = "";
#else
const std::string kernelPrefix = "../";
#endif

std::vector<uint64_t> ParameterRange(const uint64_t max)
{
    std::vector<uint64_t> values;

    for (uint64_t i = 1; i <= max; ++i)
    {
        values.push_back(i);
    }

    return values;
}

enum class AtfSampleType
{
    Convolution,
    GEMM,
    CCSD,
    PRL
};

constexpr AtfSampleType activeSample = AtfSampleType::Convolution;
const std::string kernelPath = kernelPrefix + "../Examples/AtfSamples/";

int main(int argc, char** argv)
{
    ktt::PlatformIndex platformIndex = 0;
    ktt::DeviceIndex deviceIndex = 0;

    if (argc >= 2)
    {
        platformIndex = std::stoul(std::string(argv[1]));

        if (argc >= 3)
        {
            deviceIndex = std::stoul(std::string(argv[2]));
        }
    }

    uint64_t inputSize1;
    uint64_t inputSize2;
    uint64_t inputSize3;
    uint64_t inputSize4;
    uint64_t inputSize5;
    uint64_t inputSize6;
    uint64_t inputSize7;

    if constexpr (activeSample == AtfSampleType::Convolution)
    {
        inputSize1 = 4096;
        inputSize2 = 4096;
    }
    else if constexpr (activeSample == AtfSampleType::GEMM)
    {
        inputSize1 = 10;
        inputSize2 = 500;
        inputSize3 = 64;
    }
    else if constexpr (activeSample == AtfSampleType::CCSD)
    {
        inputSize1 = 24;
        inputSize2 = 16;
        inputSize3 = 16;
        inputSize4 = 24;
        inputSize5 = 16;
        inputSize6 = 16;
        inputSize7 = 24;
    }
    else // AtfSampleType::PRL
    {
        inputSize1 = 1024;
        inputSize2 = 1024;
    }

    auto DescendingConstraint = [](const std::vector<uint64_t>& v)
    {
        bool valid = true;

        for (size_t i = 1; i < v.size(); ++i)
        {
            valid = valid && (v[i - 1] >= v[i]);
        }

        return valid;
    };

    auto UnequalConstraint = [](const std::vector<uint64_t>& v)
    {
        if (v.size() < 2)
        {
            return true;
        }

        bool valid = true;

        for (size_t i = 1; i < v.size(); ++i)
        {
            valid = valid && (v[i - 1] != v[i]);
        }

        valid = valid && (v[v.size() - 1] != v[0]);
        return valid;
    };

    auto LessThanOrEqualConstraint = [](const std::vector<uint64_t>& v) { return v[0] <= v[1]; };
    auto LessThanOrEqualCeilDivConstraint = [](const std::vector<uint64_t>& v) { return v[0] <= (v[1] + v[2] - 1) / v[2]; };
    auto DividesConstraint = [](const std::vector<uint64_t>& v) { return v[1] % v[0] == 0; };
    auto DividesDivConstraint = [](const std::vector<uint64_t>& v) { return (v[1] / v[2]) % v[0] == 0; };
    auto NoPostInSecondKernelConstraint = [](const std::vector<uint64_t>& v) { return v[0] == 1 || (v[0] % v[1] == 0); };

    ktt::Tuner tuner(platformIndex, deviceIndex, ktt::ComputeApi::OpenCL);
    ktt::KernelDefinitionId definition;
    ktt::KernelId kernel;

    if constexpr (activeSample == AtfSampleType::Convolution)
    {
        definition = tuner.AddKernelDefinitionFromFile("gaussian_1", kernelPath + "GaussianStatic1.cl", ktt::DimensionVector(), ktt::DimensionVector());
        kernel = tuner.CreateSimpleKernel("Convolution", definition);

        tuner.AddParameter(kernel, "CACHE_L_CB", std::vector<uint64_t>{0, 1});
        tuner.AddParameter(kernel, "CACHE_P_CB", std::vector<uint64_t>{0, 1});
        tuner.AddParameter(kernel, "G_CB_RES_DEST_LEVEL", std::vector<uint64_t>{2});
        tuner.AddParameter(kernel, "L_CB_RES_DEST_LEVEL", std::vector<uint64_t>{2, 1, 0});
        tuner.AddParameter(kernel, "P_CB_RES_DEST_LEVEL", std::vector<uint64_t>{2, 1, 0});

        tuner.AddParameter(kernel, "OCL_DIM_L_1", std::vector<uint64_t>{0, 1});
        tuner.AddParameter(kernel, "OCL_DIM_L_2", std::vector<uint64_t>{0, 1});

        tuner.AddParameter(kernel, "INPUT_SIZE_L_1", std::vector<uint64_t>{inputSize1});
        tuner.AddParameter(kernel, "L_CB_SIZE_L_1", ParameterRange(inputSize1));
        tuner.AddParameter(kernel, "P_CB_SIZE_L_1", ParameterRange(inputSize1));
        tuner.AddParameter(kernel, "NUM_WG_L_1", ParameterRange(inputSize1));
        tuner.AddParameter(kernel, "NUM_WI_L_1", ParameterRange(inputSize1));

        tuner.AddParameter(kernel, "INPUT_SIZE_L_2", std::vector<uint64_t>{inputSize2});
        tuner.AddParameter(kernel, "L_CB_SIZE_L_2", ParameterRange(inputSize2));
        tuner.AddParameter(kernel, "P_CB_SIZE_L_2", ParameterRange(inputSize2));
        tuner.AddParameter(kernel, "NUM_WG_L_2", ParameterRange(inputSize2));
        tuner.AddParameter(kernel, "NUM_WI_L_2", ParameterRange(inputSize2));

        tuner.AddParameter(kernel, "L_REDUCTION", std::vector<uint64_t>{1});
        tuner.AddParameter(kernel, "P_WRITE_BACK", std::vector<uint64_t>{0});
        tuner.AddParameter(kernel, "L_WRITE_BACK", std::vector<uint64_t>{2});

        tuner.AddConstraint(kernel, {"G_CB_RES_DEST_LEVEL", "L_CB_RES_DEST_LEVEL", "P_CB_RES_DEST_LEVEL"}, DescendingConstraint);
        tuner.AddConstraint(kernel, {"OCL_DIM_L_1", "OCL_DIM_L_2"}, UnequalConstraint);

        tuner.AddConstraint(kernel, {"L_CB_SIZE_L_1", "INPUT_SIZE_L_1"}, DividesConstraint);
        tuner.AddConstraint(kernel, {"P_CB_SIZE_L_1", "L_CB_SIZE_L_1"}, DividesConstraint);
        tuner.AddConstraint(kernel, {"NUM_WG_L_1", "INPUT_SIZE_L_1", "L_CB_SIZE_L_1"}, DividesDivConstraint);
        tuner.AddConstraint(kernel, {"NUM_WI_L_1", "L_CB_SIZE_L_1", "P_CB_SIZE_L_1"}, DividesDivConstraint);
        tuner.AddConstraint(kernel, {"NUM_WI_L_1", "INPUT_SIZE_L_1", "NUM_WG_L_1"}, LessThanOrEqualCeilDivConstraint);

        tuner.AddConstraint(kernel, {"L_CB_SIZE_L_2", "INPUT_SIZE_L_2"}, DividesConstraint);
        tuner.AddConstraint(kernel, {"P_CB_SIZE_L_2", "L_CB_SIZE_L_2"}, DividesConstraint);
        tuner.AddConstraint(kernel, {"NUM_WG_L_2", "INPUT_SIZE_L_2", "L_CB_SIZE_L_2"}, DividesDivConstraint);
        tuner.AddConstraint(kernel, {"NUM_WI_L_2", "L_CB_SIZE_L_2", "P_CB_SIZE_L_2"}, DividesDivConstraint);
        tuner.AddConstraint(kernel, {"NUM_WI_L_2", "INPUT_SIZE_L_2", "NUM_WG_L_2"}, LessThanOrEqualCeilDivConstraint);
    }
    else if constexpr (activeSample == AtfSampleType::GEMM)
    {
        definition = tuner.AddKernelDefinitionFromFile("gemm_1", kernelPath + "Gemm1.cl", ktt::DimensionVector(), ktt::DimensionVector());
        kernel = tuner.CreateSimpleKernel("GEMM", definition);

        tuner.AddParameter(kernel, "CACHE_L_CB", std::vector<uint64_t>{0, 1});
        tuner.AddParameter(kernel, "CACHE_P_CB", std::vector<uint64_t>{0, 1});
        tuner.AddParameter(kernel, "G_CB_RES_DEST_LEVEL", std::vector<uint64_t>{2});
        tuner.AddParameter(kernel, "L_CB_RES_DEST_LEVEL", std::vector<uint64_t>{2, 1, 0});
        tuner.AddParameter(kernel, "P_CB_RES_DEST_LEVEL", std::vector<uint64_t>{2, 1, 0});

        tuner.AddParameter(kernel, "OCL_DIM_L_1", std::vector<uint64_t>{0, 1, 2});
        tuner.AddParameter(kernel, "OCL_DIM_L_2", std::vector<uint64_t>{0, 1, 2});
        tuner.AddParameter(kernel, "OCL_DIM_R_1", std::vector<uint64_t>{0, 1, 2});

        tuner.AddParameter(kernel, "INPUT_SIZE_L_1", std::vector<uint64_t>{inputSize1});
        tuner.AddParameter(kernel, "L_CB_SIZE_L_1", ParameterRange(inputSize1));
        tuner.AddParameter(kernel, "P_CB_SIZE_L_1", ParameterRange(inputSize1));
        tuner.AddParameter(kernel, "NUM_WG_L_1", ParameterRange(inputSize1));
        tuner.AddParameter(kernel, "NUM_WI_L_1", ParameterRange(inputSize1));

        tuner.AddParameter(kernel, "INPUT_SIZE_L_2", std::vector<uint64_t>{inputSize2});
        tuner.AddParameter(kernel, "L_CB_SIZE_L_2", ParameterRange(inputSize2));
        tuner.AddParameter(kernel, "P_CB_SIZE_L_2", ParameterRange(inputSize2));
        tuner.AddParameter(kernel, "NUM_WG_L_2", ParameterRange(inputSize2));
        tuner.AddParameter(kernel, "NUM_WI_L_2", ParameterRange(inputSize2));

        tuner.AddParameter(kernel, "INPUT_SIZE_R_1", std::vector<uint64_t>{inputSize3});
        tuner.AddParameter(kernel, "L_CB_SIZE_R_1", ParameterRange(inputSize3));
        tuner.AddParameter(kernel, "P_CB_SIZE_R_1", ParameterRange(inputSize3));
        tuner.AddParameter(kernel, "NUM_WG_R_1", ParameterRange(inputSize3));
        tuner.AddParameter(kernel, "NUM_WI_R_1", ParameterRange(inputSize3));

        tuner.AddParameter(kernel, "L_REDUCTION", std::vector<uint64_t>{1});
        tuner.AddParameter(kernel, "P_WRITE_BACK", std::vector<uint64_t>{0});
        tuner.AddParameter(kernel, "L_WRITE_BACK", std::vector<uint64_t>{2});

        tuner.AddConstraint(kernel, {"G_CB_RES_DEST_LEVEL", "L_CB_RES_DEST_LEVEL", "P_CB_RES_DEST_LEVEL"}, DescendingConstraint);
        tuner.AddConstraint(kernel, {"OCL_DIM_L_1", "OCL_DIM_L_2"}, UnequalConstraint);
        tuner.AddConstraint(kernel, {"OCL_DIM_R_1", "OCL_DIM_L_2"}, UnequalConstraint);
        tuner.AddConstraint(kernel, {"OCL_DIM_L_1", "OCL_DIM_R_1"}, UnequalConstraint);

        tuner.AddConstraint(kernel, {"L_CB_SIZE_L_1", "INPUT_SIZE_L_1"}, DividesConstraint);
        tuner.AddConstraint(kernel, {"P_CB_SIZE_L_1", "L_CB_SIZE_L_1"}, DividesConstraint);
        tuner.AddConstraint(kernel, {"NUM_WG_L_1", "INPUT_SIZE_L_1", "L_CB_SIZE_L_1"}, DividesDivConstraint);
        tuner.AddConstraint(kernel, {"NUM_WI_L_1", "L_CB_SIZE_L_1", "P_CB_SIZE_L_1"}, DividesDivConstraint);
        tuner.AddConstraint(kernel, {"NUM_WI_L_1", "INPUT_SIZE_L_1", "NUM_WG_L_1"}, LessThanOrEqualCeilDivConstraint);

        tuner.AddConstraint(kernel, {"L_CB_SIZE_L_2", "INPUT_SIZE_L_2"}, DividesConstraint);
        tuner.AddConstraint(kernel, {"P_CB_SIZE_L_2", "L_CB_SIZE_L_2"}, DividesConstraint);
        tuner.AddConstraint(kernel, {"NUM_WG_L_2", "INPUT_SIZE_L_2", "L_CB_SIZE_L_2"}, DividesDivConstraint);
        tuner.AddConstraint(kernel, {"NUM_WI_L_2", "L_CB_SIZE_L_2", "P_CB_SIZE_L_2"}, DividesDivConstraint);
        tuner.AddConstraint(kernel, {"NUM_WI_L_2", "INPUT_SIZE_L_2", "NUM_WG_L_2"}, LessThanOrEqualCeilDivConstraint);

        tuner.AddConstraint(kernel, {"L_CB_SIZE_R_1", "INPUT_SIZE_R_1"}, DividesConstraint);
        tuner.AddConstraint(kernel, {"P_CB_SIZE_R_1", "L_CB_SIZE_R_1"}, DividesConstraint);
        tuner.AddConstraint(kernel, {"NUM_WG_R_1", "INPUT_SIZE_R_1", "L_CB_SIZE_R_1"}, DividesDivConstraint);
        tuner.AddConstraint(kernel, {"NUM_WI_R_1", "L_CB_SIZE_R_1", "P_CB_SIZE_R_1"}, DividesDivConstraint);
        tuner.AddConstraint(kernel, {"NUM_WI_R_1", "INPUT_SIZE_R_1", "NUM_WG_R_1"}, LessThanOrEqualCeilDivConstraint);
        tuner.AddConstraint(kernel, {"NUM_WG_R_1", "L_CB_SIZE_R_1"}, NoPostInSecondKernelConstraint);
    }
    else if constexpr (activeSample == AtfSampleType::CCSD)
    {
        definition = tuner.AddKernelDefinitionFromFile("tc_1", kernelPath + "TcAbcdefGebcDfga1.cl", ktt::DimensionVector(), ktt::DimensionVector());
        kernel = tuner.CreateSimpleKernel("CCSD", definition);

        tuner.AddParameter(kernel, "CACHE_L_CB", std::vector<uint64_t>{0, 1});
        tuner.AddParameter(kernel, "CACHE_P_CB", std::vector<uint64_t>{0, 1});
        tuner.AddParameter(kernel, "G_CB_RES_DEST_LEVEL", std::vector<uint64_t>{2});
        tuner.AddParameter(kernel, "L_CB_RES_DEST_LEVEL", std::vector<uint64_t>{2, 1, 0});
        tuner.AddParameter(kernel, "P_CB_RES_DEST_LEVEL", std::vector<uint64_t>{2, 1, 0});

        tuner.AddParameter(kernel, "OCL_DIM_L_1", std::vector<uint64_t>{0, 1, 2, 3, 4, 5, 6});
        tuner.AddParameter(kernel, "OCL_DIM_L_2", std::vector<uint64_t>{0, 1, 2, 3, 4, 5, 6});
        tuner.AddParameter(kernel, "OCL_DIM_L_3", std::vector<uint64_t>{0, 1, 2, 3, 4, 5, 6});
        tuner.AddParameter(kernel, "OCL_DIM_L_4", std::vector<uint64_t>{0, 1, 2, 3, 4, 5, 6});
        tuner.AddParameter(kernel, "OCL_DIM_L_5", std::vector<uint64_t>{0, 1, 2, 3, 4, 5, 6});
        tuner.AddParameter(kernel, "OCL_DIM_L_6", std::vector<uint64_t>{0, 1, 2, 3, 4, 5, 6});
        tuner.AddParameter(kernel, "OCL_DIM_R_1", std::vector<uint64_t>{0, 1, 2, 3, 4, 5, 6});

        tuner.AddParameter(kernel, "INPUT_SIZE_L_1", std::vector<uint64_t>{inputSize1});
        tuner.AddParameter(kernel, "L_CB_SIZE_L_1", ParameterRange(inputSize1));
        tuner.AddParameter(kernel, "P_CB_SIZE_L_1", ParameterRange(inputSize1));
        tuner.AddParameter(kernel, "NUM_WG_L_1", ParameterRange(inputSize1));
        tuner.AddParameter(kernel, "NUM_WI_L_1", ParameterRange(inputSize1));

        tuner.AddParameter(kernel, "INPUT_SIZE_L_2", std::vector<uint64_t>{inputSize2});
        tuner.AddParameter(kernel, "L_CB_SIZE_L_2", ParameterRange(inputSize2));
        tuner.AddParameter(kernel, "P_CB_SIZE_L_2", ParameterRange(inputSize2));
        tuner.AddParameter(kernel, "NUM_WG_L_2", ParameterRange(inputSize2));
        tuner.AddParameter(kernel, "NUM_WI_L_2", ParameterRange(inputSize2));

        tuner.AddParameter(kernel, "INPUT_SIZE_L_3", std::vector<uint64_t>{inputSize3});
        tuner.AddParameter(kernel, "L_CB_SIZE_L_3", ParameterRange(inputSize3));
        tuner.AddParameter(kernel, "P_CB_SIZE_L_3", ParameterRange(inputSize3));
        tuner.AddParameter(kernel, "NUM_WG_L_3", ParameterRange(inputSize3));
        tuner.AddParameter(kernel, "NUM_WI_L_3", ParameterRange(inputSize3));

        tuner.AddParameter(kernel, "INPUT_SIZE_L_4", std::vector<uint64_t>{inputSize4});
        tuner.AddParameter(kernel, "L_CB_SIZE_L_4", ParameterRange(inputSize4));
        tuner.AddParameter(kernel, "P_CB_SIZE_L_4", ParameterRange(inputSize4));
        tuner.AddParameter(kernel, "NUM_WG_L_4", ParameterRange(inputSize4));
        tuner.AddParameter(kernel, "NUM_WI_L_4", ParameterRange(inputSize4));

        tuner.AddParameter(kernel, "INPUT_SIZE_L_5", std::vector<uint64_t>{inputSize5});
        tuner.AddParameter(kernel, "L_CB_SIZE_L_5", ParameterRange(inputSize5));
        tuner.AddParameter(kernel, "P_CB_SIZE_L_5", ParameterRange(inputSize5));
        tuner.AddParameter(kernel, "NUM_WG_L_5", ParameterRange(inputSize5));
        tuner.AddParameter(kernel, "NUM_WI_L_5", ParameterRange(inputSize5));

        tuner.AddParameter(kernel, "INPUT_SIZE_L_6", std::vector<uint64_t>{inputSize6});
        tuner.AddParameter(kernel, "L_CB_SIZE_L_6", ParameterRange(inputSize6));
        tuner.AddParameter(kernel, "P_CB_SIZE_L_6", ParameterRange(inputSize6));
        tuner.AddParameter(kernel, "NUM_WG_L_6", ParameterRange(inputSize6));
        tuner.AddParameter(kernel, "NUM_WI_L_6", ParameterRange(inputSize6));

        tuner.AddParameter(kernel, "INPUT_SIZE_R_1", std::vector<uint64_t>{inputSize7});
        tuner.AddParameter(kernel, "L_CB_SIZE_R_1", ParameterRange(inputSize7));
        tuner.AddParameter(kernel, "P_CB_SIZE_R_1", ParameterRange(inputSize7));
        tuner.AddParameter(kernel, "NUM_WG_R_1", ParameterRange(inputSize7));
        tuner.AddParameter(kernel, "NUM_WI_R_1", ParameterRange(inputSize7));

        tuner.AddParameter(kernel, "L_REDUCTION", std::vector<uint64_t>{1});
        tuner.AddParameter(kernel, "P_WRITE_BACK", std::vector<uint64_t>{0});
        tuner.AddParameter(kernel, "L_WRITE_BACK", std::vector<uint64_t>{6});

        tuner.AddConstraint(kernel, {"G_CB_RES_DEST_LEVEL", "L_CB_RES_DEST_LEVEL", "P_CB_RES_DEST_LEVEL"}, DescendingConstraint);
        tuner.AddConstraint(kernel, {"OCL_DIM_L_1", "OCL_DIM_L_2"}, UnequalConstraint);
        tuner.AddConstraint(kernel, {"OCL_DIM_L_1", "OCL_DIM_L_3"}, UnequalConstraint);
        tuner.AddConstraint(kernel, {"OCL_DIM_L_1", "OCL_DIM_L_4"}, UnequalConstraint);
        tuner.AddConstraint(kernel, {"OCL_DIM_L_1", "OCL_DIM_L_5"}, UnequalConstraint);
        tuner.AddConstraint(kernel, {"OCL_DIM_L_1", "OCL_DIM_L_6"}, UnequalConstraint);
        tuner.AddConstraint(kernel, {"OCL_DIM_L_1", "OCL_DIM_R_1"}, UnequalConstraint);
        tuner.AddConstraint(kernel, {"OCL_DIM_L_2", "OCL_DIM_L_3"}, UnequalConstraint);
        tuner.AddConstraint(kernel, {"OCL_DIM_L_2", "OCL_DIM_L_4"}, UnequalConstraint);
        tuner.AddConstraint(kernel, {"OCL_DIM_L_2", "OCL_DIM_L_5"}, UnequalConstraint);
        tuner.AddConstraint(kernel, {"OCL_DIM_L_2", "OCL_DIM_L_6"}, UnequalConstraint);
        tuner.AddConstraint(kernel, {"OCL_DIM_L_2", "OCL_DIM_R_1"}, UnequalConstraint);
        tuner.AddConstraint(kernel, {"OCL_DIM_L_3", "OCL_DIM_L_4"}, UnequalConstraint);
        tuner.AddConstraint(kernel, {"OCL_DIM_L_3", "OCL_DIM_L_5"}, UnequalConstraint);
        tuner.AddConstraint(kernel, {"OCL_DIM_L_3", "OCL_DIM_L_6"}, UnequalConstraint);
        tuner.AddConstraint(kernel, {"OCL_DIM_L_3", "OCL_DIM_R_1"}, UnequalConstraint);
        tuner.AddConstraint(kernel, {"OCL_DIM_L_4", "OCL_DIM_L_5"}, UnequalConstraint);
        tuner.AddConstraint(kernel, {"OCL_DIM_L_4", "OCL_DIM_L_6"}, UnequalConstraint);
        tuner.AddConstraint(kernel, {"OCL_DIM_L_4", "OCL_DIM_R_1"}, UnequalConstraint);
        tuner.AddConstraint(kernel, {"OCL_DIM_L_5", "OCL_DIM_L_6"}, UnequalConstraint);
        tuner.AddConstraint(kernel, {"OCL_DIM_L_5", "OCL_DIM_R_1"}, UnequalConstraint);
        tuner.AddConstraint(kernel, {"OCL_DIM_L_6", "OCL_DIM_R_1"}, UnequalConstraint);

        tuner.AddConstraint(kernel, {"L_CB_SIZE_L_1", "INPUT_SIZE_L_1"}, DividesConstraint);
        tuner.AddConstraint(kernel, {"P_CB_SIZE_L_1", "L_CB_SIZE_L_1"}, DividesConstraint);
        tuner.AddConstraint(kernel, {"NUM_WG_L_1", "INPUT_SIZE_L_1", "L_CB_SIZE_L_1"}, DividesDivConstraint);
        tuner.AddConstraint(kernel, {"NUM_WI_L_1", "L_CB_SIZE_L_1", "P_CB_SIZE_L_1"}, DividesDivConstraint);
        tuner.AddConstraint(kernel, {"NUM_WI_L_1", "INPUT_SIZE_L_1", "NUM_WG_L_1"}, LessThanOrEqualCeilDivConstraint);

        tuner.AddConstraint(kernel, {"L_CB_SIZE_L_2", "INPUT_SIZE_L_2"}, DividesConstraint);
        tuner.AddConstraint(kernel, {"P_CB_SIZE_L_2", "L_CB_SIZE_L_2"}, DividesConstraint);
        tuner.AddConstraint(kernel, {"NUM_WG_L_2", "INPUT_SIZE_L_2", "L_CB_SIZE_L_2"}, DividesDivConstraint);
        tuner.AddConstraint(kernel, {"NUM_WI_L_2", "L_CB_SIZE_L_2", "P_CB_SIZE_L_2"}, DividesDivConstraint);
        tuner.AddConstraint(kernel, {"NUM_WI_L_2", "INPUT_SIZE_L_2", "NUM_WG_L_2"}, LessThanOrEqualCeilDivConstraint);

        tuner.AddConstraint(kernel, {"L_CB_SIZE_L_3", "INPUT_SIZE_L_3"}, DividesConstraint);
        tuner.AddConstraint(kernel, {"P_CB_SIZE_L_3", "L_CB_SIZE_L_3"}, DividesConstraint);
        tuner.AddConstraint(kernel, {"NUM_WG_L_3", "INPUT_SIZE_L_3", "L_CB_SIZE_L_3"}, DividesDivConstraint);
        tuner.AddConstraint(kernel, {"NUM_WI_L_3", "L_CB_SIZE_L_3", "P_CB_SIZE_L_3"}, DividesDivConstraint);
        tuner.AddConstraint(kernel, {"NUM_WI_L_3", "INPUT_SIZE_L_3", "NUM_WG_L_3"}, LessThanOrEqualCeilDivConstraint);

        tuner.AddConstraint(kernel, {"L_CB_SIZE_L_4", "INPUT_SIZE_L_4"}, DividesConstraint);
        tuner.AddConstraint(kernel, {"P_CB_SIZE_L_4", "L_CB_SIZE_L_4"}, DividesConstraint);
        tuner.AddConstraint(kernel, {"NUM_WG_L_4", "INPUT_SIZE_L_4", "L_CB_SIZE_L_4"}, DividesDivConstraint);
        tuner.AddConstraint(kernel, {"NUM_WI_L_4", "L_CB_SIZE_L_4", "P_CB_SIZE_L_4"}, DividesDivConstraint);
        tuner.AddConstraint(kernel, {"NUM_WI_L_4", "INPUT_SIZE_L_4", "NUM_WG_L_4"}, LessThanOrEqualCeilDivConstraint);

        tuner.AddConstraint(kernel, {"L_CB_SIZE_L_5", "INPUT_SIZE_L_5"}, DividesConstraint);
        tuner.AddConstraint(kernel, {"P_CB_SIZE_L_5", "L_CB_SIZE_L_5"}, DividesConstraint);
        tuner.AddConstraint(kernel, {"NUM_WG_L_5", "INPUT_SIZE_L_5", "L_CB_SIZE_L_5"}, DividesDivConstraint);
        tuner.AddConstraint(kernel, {"NUM_WI_L_5", "L_CB_SIZE_L_5", "P_CB_SIZE_L_5"}, DividesDivConstraint);
        tuner.AddConstraint(kernel, {"NUM_WI_L_5", "INPUT_SIZE_L_5", "NUM_WG_L_5"}, LessThanOrEqualCeilDivConstraint);

        tuner.AddConstraint(kernel, {"L_CB_SIZE_L_6", "INPUT_SIZE_L_6"}, DividesConstraint);
        tuner.AddConstraint(kernel, {"P_CB_SIZE_L_6", "L_CB_SIZE_L_6"}, DividesConstraint);
        tuner.AddConstraint(kernel, {"NUM_WG_L_6", "INPUT_SIZE_L_6", "L_CB_SIZE_L_6"}, DividesDivConstraint);
        tuner.AddConstraint(kernel, {"NUM_WI_L_6", "L_CB_SIZE_L_6", "P_CB_SIZE_L_6"}, DividesDivConstraint);
        tuner.AddConstraint(kernel, {"NUM_WI_L_6", "INPUT_SIZE_L_6", "NUM_WG_L_6"}, LessThanOrEqualCeilDivConstraint);

        tuner.AddConstraint(kernel, {"L_CB_SIZE_R_1", "INPUT_SIZE_R_1"}, DividesConstraint);
        tuner.AddConstraint(kernel, {"P_CB_SIZE_R_1", "L_CB_SIZE_R_1"}, DividesConstraint);
        tuner.AddConstraint(kernel, {"NUM_WG_R_1", "INPUT_SIZE_R_1", "L_CB_SIZE_R_1"}, DividesDivConstraint);
        tuner.AddConstraint(kernel, {"NUM_WI_R_1", "L_CB_SIZE_R_1", "P_CB_SIZE_R_1"}, DividesDivConstraint);
        tuner.AddConstraint(kernel, {"NUM_WI_R_1", "INPUT_SIZE_R_1", "NUM_WG_R_1"}, LessThanOrEqualCeilDivConstraint);
        tuner.AddConstraint(kernel, {"NUM_WG_R_1", "L_CB_SIZE_R_1"}, NoPostInSecondKernelConstraint);
    }
    else // AtfSampleType::PRL
    {
        definition = tuner.AddKernelDefinitionFromFile("rl_1", kernelPath + "Rl1.cl", ktt::DimensionVector(), ktt::DimensionVector());
        kernel = tuner.CreateSimpleKernel("PRL", definition);

        tuner.AddParameter(kernel, "CACHE_L_CB", std::vector<uint64_t>{0, 1});
        tuner.AddParameter(kernel, "CACHE_P_CB", std::vector<uint64_t>{0, 1});
        tuner.AddParameter(kernel, "G_CB_RES_DEST_LEVEL", std::vector<uint64_t>{2});
        tuner.AddParameter(kernel, "L_CB_RES_DEST_LEVEL", std::vector<uint64_t>{2, 1, 0});
        tuner.AddParameter(kernel, "P_CB_RES_DEST_LEVEL", std::vector<uint64_t>{2, 1, 0});

        tuner.AddParameter(kernel, "OCL_DIM_L_1", std::vector<uint64_t>{0, 1});
        tuner.AddParameter(kernel, "OCL_DIM_R_1", std::vector<uint64_t>{0, 1});

        tuner.AddParameter(kernel, "INPUT_SIZE_L_1", std::vector<uint64_t>{inputSize1});
        tuner.AddParameter(kernel, "L_CB_SIZE_L_1", ParameterRange(inputSize1));
        tuner.AddParameter(kernel, "P_CB_SIZE_L_1", ParameterRange(inputSize1));
        tuner.AddParameter(kernel, "NUM_WG_L_1", ParameterRange(inputSize1));
        tuner.AddParameter(kernel, "NUM_WI_L_1", ParameterRange(inputSize1));

        tuner.AddParameter(kernel, "INPUT_SIZE_R_1", std::vector<uint64_t>{inputSize2});
        tuner.AddParameter(kernel, "L_CB_SIZE_R_1", ParameterRange(inputSize2));
        tuner.AddParameter(kernel, "P_CB_SIZE_R_1", ParameterRange(inputSize2));
        tuner.AddParameter(kernel, "NUM_WG_R_1", ParameterRange(inputSize2));
        tuner.AddParameter(kernel, "NUM_WI_R_1", ParameterRange(inputSize2));

        tuner.AddParameter(kernel, "L_REDUCTION", std::vector<uint64_t>{1});
        tuner.AddParameter(kernel, "P_WRITE_BACK", std::vector<uint64_t>{0});
        tuner.AddParameter(kernel, "L_WRITE_BACK", std::vector<uint64_t>{1});

        tuner.AddConstraint(kernel, {"G_CB_RES_DEST_LEVEL", "L_CB_RES_DEST_LEVEL", "P_CB_RES_DEST_LEVEL"}, DescendingConstraint);
        tuner.AddConstraint(kernel, {"OCL_DIM_L_1", "OCL_DIM_R_1"}, UnequalConstraint);

        tuner.AddConstraint(kernel, {"L_CB_SIZE_L_1", "INPUT_SIZE_L_1"}, DividesConstraint);
        tuner.AddConstraint(kernel, {"P_CB_SIZE_L_1", "L_CB_SIZE_L_1"}, DividesConstraint);
        tuner.AddConstraint(kernel, {"NUM_WG_L_1", "INPUT_SIZE_L_1", "L_CB_SIZE_L_1"}, DividesDivConstraint);
        tuner.AddConstraint(kernel, {"NUM_WI_L_1", "L_CB_SIZE_L_1", "P_CB_SIZE_L_1"}, DividesDivConstraint);
        tuner.AddConstraint(kernel, {"NUM_WI_L_1", "INPUT_SIZE_L_1", "NUM_WG_L_1"}, LessThanOrEqualCeilDivConstraint);

        tuner.AddConstraint(kernel, {"L_CB_SIZE_R_1", "INPUT_SIZE_R_1"}, DividesConstraint);
        tuner.AddConstraint(kernel, {"P_CB_SIZE_R_1", "L_CB_SIZE_R_1"}, DividesConstraint);
        tuner.AddConstraint(kernel, {"NUM_WG_R_1", "INPUT_SIZE_R_1", "L_CB_SIZE_R_1"}, DividesDivConstraint);
        tuner.AddConstraint(kernel, {"NUM_WI_R_1", "L_CB_SIZE_R_1", "P_CB_SIZE_R_1"}, DividesDivConstraint);
        tuner.AddConstraint(kernel, {"NUM_WI_R_1", "INPUT_SIZE_R_1", "NUM_WG_R_1"}, LessThanOrEqualCeilDivConstraint);
        tuner.AddConstraint(kernel, {"NUM_WG_R_1", "L_CB_SIZE_R_1"}, NoPostInSecondKernelConstraint);
    }

    auto results = tuner.TuneKernel(kernel);
    tuner.SaveResults(results, "AtfOutput", ktt::OutputFormat::XML);
    return 0;
}
