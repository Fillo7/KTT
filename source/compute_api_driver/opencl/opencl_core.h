#pragma once

#include <memory>
#include <ostream>
#include <string>
#include <vector>

#include "opencl_buffer.h"
#include "opencl_command_queue.h"
#include "opencl_context.h"
#include "opencl_device.h"
#include "opencl_kernel.h"
#include "opencl_platform.h"
#include "opencl_program.h"
#include "../compute_api_driver.h"
#include "../../dto/device_info.h"
#include "../../dto/kernel_run_result.h"
#include "../../dto/platform_info.h"
#include "../../enum/argument_memory_type.h"
#include "../../kernel_argument/kernel_argument.h"

namespace ktt
{

class OpenclCore : public ComputeApiDriver
{
public:
    // Constructor
    explicit OpenclCore(const size_t platformIndex, const size_t deviceIndex);

    // Platform and device retrieval methods
    virtual void printComputeApiInfo(std::ostream& outputTarget) const override;
    virtual std::vector<PlatformInfo> getPlatformInfo() const override;
    virtual std::vector<DeviceInfo> getDeviceInfo(const size_t platformIndex) const override;

    // Compiler options setup
    virtual void setCompilerOptions(const std::string& options) override;

    // Cache handling
    virtual void clearCache() override;

    // High-level kernel execution methods
    virtual KernelRunResult runKernel(const std::string& source, const std::string& kernelName, const std::vector<size_t>& globalSize,
        const std::vector<size_t>& localSize, const std::vector<KernelArgument>& arguments) const override;

    // Low-level kernel execution methods
    std::unique_ptr<OpenclProgram> createAndBuildProgram(const std::string& source) const;
    std::unique_ptr<OpenclBuffer> createBuffer(const ArgumentMemoryType& argumentMemoryType, const size_t size, const size_t kernelArgumentId) const;
    void updateBuffer(OpenclBuffer& buffer, const void* source, const size_t dataSize) const;
    void getBufferData(const OpenclBuffer& buffer, void* destination, const size_t dataSize) const;
    std::unique_ptr<OpenclKernel> createKernel(const OpenclProgram& program, const std::string& kernelName) const;
    void setKernelArgumentScalar(OpenclKernel& kernel, const KernelArgument& argument) const;
    void setKernelArgumentVector(OpenclKernel& kernel, const OpenclBuffer& buffer) const;
    cl_ulong enqueueKernel(OpenclKernel& kernel, const std::vector<size_t>& globalSize, const std::vector<size_t>& localSize) const;

private:
    // Attributes
    std::unique_ptr<OpenclContext> context;
    std::unique_ptr<OpenclCommandQueue> commandQueue;
    std::string compilerOptions;
    mutable std::vector<std::unique_ptr<OpenclBuffer>> bufferCache;

    // Helper methods
    static PlatformInfo getOpenclPlatformInfo(const size_t platformIndex);
    static DeviceInfo getOpenclDeviceInfo(const size_t platformIndex, const size_t deviceIndex);
    static std::vector<OpenclPlatform> getOpenclPlatforms();
    static std::vector<OpenclDevice> getOpenclDevices(const OpenclPlatform& platform);
    static DeviceType getDeviceType(const cl_device_type deviceType);
    std::vector<KernelArgument> getResultArguments(const std::vector<std::unique_ptr<OpenclBuffer>>& outputBuffers,
        const std::vector<const KernelArgument*>& inputArgumentPointers) const;
    bool loadBufferFromCache(const size_t argumentId, OpenclKernel& openclKernel) const;
};

} // namespace ktt