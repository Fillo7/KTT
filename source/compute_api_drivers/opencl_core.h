#pragma once

#include <memory>
#include <ostream>
#include <string>
#include <vector>

#include "../ktt_type_aliases.h"
#include "../dtos/device_info.h"
#include "../dtos/platform_info.h"
#include "../enums/argument_memory_type.h"
#include "opencl_buffer.h"
#include "opencl_command_queue.h"
#include "opencl_context.h"
#include "opencl_device.h"
#include "opencl_kernel.h"
#include "opencl_platform.h"
#include "opencl_program.h"

namespace ktt
{

class OpenCLCore
{
public:
    // Constructor
    explicit OpenCLCore(const size_t platformIndex, const size_t deviceIndex);

    // Platform and device retrieval methods
    static void printOpenCLInfo(std::ostream& outputTarget);
    static PlatformInfo getOpenCLPlatformInfo(const size_t platformIndex);
    static std::vector<PlatformInfo> getOpenCLPlatformInfoAll();
    static DeviceInfo getOpenCLDeviceInfo(const size_t platformIndex, const size_t deviceIndex);
    static std::vector<DeviceInfo> getOpenCLDeviceInfoAll(const size_t platformIndex);

    // Compiler options setup
    void setOpenCLCompilerOptions(const std::string& options);

    // High-level kernel execution methods
    // to do

    // Low-level kernel execution methods
    OpenCLProgram createAndBuildProgram(const std::string& source) const;
    OpenCLBuffer createBuffer(const ArgumentMemoryType& argumentMemoryType, const size_t size) const;
    void updateBuffer(OpenCLBuffer& buffer, const void* source, const size_t dataSize) const;
    void getBufferData(const OpenCLBuffer& buffer, void* destination, const size_t dataSize) const;
    OpenCLKernel createKernel(const OpenCLProgram& program, const std::string& kernelName) const;
    void setKernelArgument(OpenCLKernel& kernel, const OpenCLBuffer& buffer) const;
    cl_ulong runKernel(OpenCLKernel& kernel, const std::vector<size_t>& globalSize, const std::vector<size_t>& localSize) const;

private:
    // Attributes
    std::unique_ptr<OpenCLContext> context;
    std::unique_ptr<OpenCLCommandQueue> commandQueue;
    std::string compilerOptions;

    // Helper methods
    static std::vector<OpenCLPlatform> getOpenCLPlatforms();
    static std::vector<OpenCLDevice> getOpenCLDevices(const OpenCLPlatform& platform);
    static std::string getPlatformInfo(const cl_platform_id id, const cl_platform_info info);
    static std::string getDeviceInfo(const cl_device_id id, const cl_device_info info);
    static DeviceType getDeviceType(const cl_device_type deviceType);
    void OpenCLCore::buildProgram(OpenCLProgram& program) const;
    std::string getProgramBuildInfo(const cl_program program, const cl_device_id id) const;
};

} // namespace ktt
