#include <iostream>
#include <string>
#include <vector>

#include <Ktt.h>

#if defined(_MSC_VER)
const std::string kernelPrefix = "";
#else
const std::string kernelPrefix = "../";
#endif

int main(int argc, char** argv)
{
    // Initialize device index and path to kernel.
    ktt::DeviceIndex deviceIndex = 0;
    std::string kernelFile = kernelPrefix + "../Tutorials/02KernelRunning/VulkanKernel.glsl";

    if (argc >= 2)
    {
        deviceIndex = std::stoul(std::string(argv[1]));

        if (argc >= 3)
        {
            kernelFile = std::string(argv[2]);
        }
    }

    // Declare kernel parameters and data variables.
    const size_t numberOfElements = 1024 * 1024;
    // Work-group count and dimensions are specified with DimensionVector. Only single dimension is utilized in this tutorial.
    // In general, DimensionVector supports up to three dimensions.
    // Note that, in order for work-group dimensions to work correctly, shader source file must contain the following definition
    // for work-group sizes: local_size_x = LOCAL_SIZE_X, local_size_y = LOCAL_SIZE_Y, local_size_z = LOCAL_SIZE_Z
    const ktt::DimensionVector workGroupDimensions(256);
    const ktt::DimensionVector workGroupCount(numberOfElements / workGroupDimensions.GetSizeX());
    
    std::vector<float> a(numberOfElements);
    std::vector<float> b(numberOfElements);
    std::vector<float> result(numberOfElements, 0.0f);

    // Initialize data
    for (size_t i = 0; i < numberOfElements; ++i)
    {
        a[i] = static_cast<float>(i);
        b[i] = static_cast<float>(i + 1);
    }

    // Create new tuner for the specified device, tuner uses Vulkan as compute API. Platform index is ignored when using Vulkan.
    ktt::Tuner tuner(0, deviceIndex, ktt::ComputeApi::Vulkan);

    // Add new kernel definition. Specify kernel function name, path to source file, default work-group count and dimensions.
    // KTT returns handle to the newly added definition, which can be used to reference it in other API methods.
    const ktt::KernelDefinitionId definition = tuner.AddKernelDefinitionFromFile("main", kernelFile, workGroupCount,
        workGroupDimensions);

    // Add new kernel arguments to tuner. Argument data is copied from std::vector containers. Specify whether the arguments are
    // used as input or output. KTT returns handle to the newly added argument, which can be used to reference it in other API
    // methods. 
    const ktt::ArgumentId aId = tuner.AddArgumentVector(a, ktt::ArgumentAccessType::ReadOnly);
    const ktt::ArgumentId bId = tuner.AddArgumentVector(b, ktt::ArgumentAccessType::ReadOnly);
    const ktt::ArgumentId resultId = tuner.AddArgumentVector(result, ktt::ArgumentAccessType::WriteOnly);

    // Set arguments for the kernel definition. The order of argument ids must match the order of arguments inside corresponding
    // CUDA kernel function.
    tuner.SetArguments(definition, {aId, bId, resultId});

    // Create simple kernel from the specified definition. Specify name which will be used during logging and output operations.
    // In more complex scenarios, kernels can have multiple definitions. Definitions can be shared between multiple kernels.
    const ktt::KernelId kernel = tuner.CreateSimpleKernel("Addition", definition);

    // Set time unit used during printing of kernel duration. The default time unit is milliseconds, but since computation in
    // this tutorial is very short, microseconds are used instead.
    tuner.SetTimeUnit(ktt::TimeUnit::Microseconds);

    // Run the specified kernel. The second argument is related to kernel tuning and will be described in further tutorials.
    // In this case, it remains empty. The third argument is used to retrieve the kernel output. For each kernel argument that
    // is retrieved, one BufferOutputDescriptor must be specified. Each of these descriptors contains id of the retrieved argument
    // and memory location where the argument data will be stored. Optionally, it can also include number of bytes to be retrieved,
    // if only a part of the argument is needed. Here, the data is stored back into result buffer which was created earlier. Note
    // that the memory location size needs to be equal or greater than the retrieved argument size.
    tuner.RunKernel(kernel, {}, {ktt::BufferOutputDescriptor(resultId, result.data())});

    // Print first ten elements from the result to check they were computed correctly.
    std::cout << "Printing the first 10 elements from result: ";

    for (size_t i = 0; i < 10; ++i)
    {
        std::cout << result[i] << " ";
    }

    std::cout << std::endl;
    return 0;
}
