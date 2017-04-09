#pragma once

#include <string>
#include <vector>

#include "CL/cl.h"
#include "opencl_utility.h"

namespace ktt
{

class OpenCLProgram
{
public:
    explicit OpenCLProgram(const std::string source, const cl_context context, const std::vector<cl_device_id>& devices) :
        source(source),
        context(context),
        devices(devices)
    {
        cl_int result;
        size_t sourceLength = source.size();
        auto sourcePointer = &source[0];
        program = clCreateProgramWithSource(context, 1, &sourcePointer, &sourceLength, &result);
        checkOpenCLError(result, std::string("clCreateProgramWithSource"));
    }

    ~OpenCLProgram()
    {
        checkOpenCLError(clReleaseProgram(program), std::string("clReleaseProgram"));
    }

    void build(const std::string& compilerOptions)
    {
        cl_int result = clBuildProgram(program, static_cast<cl_uint>(devices.size()), &devices.at(0), &compilerOptions[0], nullptr, nullptr);
        std::string buildInfo = getBuildInfo();
        checkOpenCLError(result, buildInfo);
    }

    std::string getBuildInfo() const
    {
        size_t infoSize;
        checkOpenCLError(clGetProgramBuildInfo(program, devices.at(0), CL_PROGRAM_BUILD_LOG, 0, nullptr, &infoSize));
        std::string infoString(infoSize, ' ');
        checkOpenCLError(clGetProgramBuildInfo(program, devices.at(0), CL_PROGRAM_BUILD_LOG, infoSize, &infoString[0], nullptr));

        return infoString;
    }

    cl_context getContext() const
    {
        return context;
    }

    std::string getSource() const
    {
        return source;
    }

    std::vector<cl_device_id> getDevices() const
    {
        return devices;
    }

    cl_program getProgram() const
    {
        return program;
    }

private:
    std::string source;
    cl_context context;
    std::vector<cl_device_id> devices;
    cl_program program;
};

} // namespace ktt
