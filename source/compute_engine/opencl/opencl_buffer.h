#pragma once

#include <cstring>
#include <string>
#include <vector>

#include "CL/cl.h"
#include "opencl_command_queue.h"
#include "opencl_utility.h"
#include "enum/argument_access_type.h"
#include "enum/argument_data_type.h"
#include "enum/argument_memory_location.h"

namespace ktt
{

class OpenclBuffer
{
public:
    explicit OpenclBuffer(const cl_context context, const size_t kernelArgumentId, const size_t bufferSize, const size_t elementSize,
        const ArgumentDataType& dataType, const ArgumentMemoryLocation& memoryLocation, const ArgumentAccessType& accessType) :
        context(context),
        kernelArgumentId(kernelArgumentId),
        bufferSize(bufferSize),
        elementSize(elementSize),
        dataType(dataType),
        memoryLocation(memoryLocation),
        accessType(accessType)
    {
        openclMemoryFlag = getOpenclMemoryType(accessType);
        if (memoryLocation == ArgumentMemoryLocation::Host)
        {
            openclMemoryFlag = openclMemoryFlag | CL_MEM_ALLOC_HOST_PTR;
        }

        cl_int result;
        buffer = clCreateBuffer(context, openclMemoryFlag, bufferSize, nullptr, &result);
        checkOpenclError(result, "clCreateBuffer");
    }

    ~OpenclBuffer()
    {
        checkOpenclError(clReleaseMemObject(buffer), "clReleaseMemObject");
    }

    void resize(const size_t newBufferSize)
    {
        if (bufferSize == newBufferSize)
        {
            return;
        }

        checkOpenclError(clReleaseMemObject(buffer), "clReleaseMemObject");

        cl_int result;
        buffer = clCreateBuffer(context, openclMemoryFlag, newBufferSize, nullptr, &result);
        checkOpenclError(result, "clCreateBuffer");
        bufferSize = newBufferSize;
    }

    void uploadData(cl_command_queue queue, const void* source, const size_t dataSize)
    {
        if (bufferSize < dataSize)
        {
            resize(dataSize);
        }

        if (memoryLocation == ArgumentMemoryLocation::Device)
        {
            cl_int result = clEnqueueWriteBuffer(queue, buffer, CL_TRUE, 0, dataSize, source, 0, nullptr, nullptr);
            checkOpenclError(result, "clEnqueueWriteBuffer");
        }
        else
        {
            cl_int result;
            void* destination = clEnqueueMapBuffer(queue, buffer, CL_TRUE, CL_MAP_WRITE, 0, dataSize, 0, nullptr, nullptr, &result);
            checkOpenclError(result, "clEnqueueMapBuffer");

            std::memcpy(destination, source, dataSize);
            checkOpenclError(clEnqueueUnmapMemObject(queue, buffer, destination, 0, nullptr, nullptr), "clEnqueueUnmapMemObject");
        }
    }

    void downloadData(cl_command_queue queue, void* destination, const size_t dataSize) const
    {
        if (bufferSize < dataSize)
        {
            throw std::runtime_error("Size of data to download is larger than size of buffer");
        }

        if (memoryLocation == ArgumentMemoryLocation::Device)
        {
            cl_int result = clEnqueueReadBuffer(queue, buffer, CL_TRUE, 0, dataSize, destination, 0, nullptr, nullptr);
            checkOpenclError(result, "clEnqueueReadBuffer");
        }
        else
        {
            cl_int result;
            void* source = clEnqueueMapBuffer(queue, buffer, CL_TRUE, CL_MAP_READ, 0, dataSize, 0, nullptr, nullptr, &result);
            checkOpenclError(result, "clEnqueueMapBuffer");

            std::memcpy(destination, source, dataSize);
            checkOpenclError(clEnqueueUnmapMemObject(queue, buffer, source, 0, nullptr, nullptr), "clEnqueueUnmapMemObject");
        }
    }

    cl_context getContext() const
    {
        return context;
    }

    size_t getKernelArgumentId() const
    {
        return kernelArgumentId;
    }

    size_t getBufferSize() const
    {
        return bufferSize;
    }

    size_t getElementSize() const
    {
        return elementSize;
    }

    ArgumentDataType getDataType() const
    {
        return dataType;
    }

    ArgumentMemoryLocation getMemoryLocation() const
    {
        return memoryLocation;
    }

    ArgumentAccessType getAccessType() const
    {
        return accessType;
    }

    cl_mem_flags getOpenclMemoryFlag() const
    {
        return openclMemoryFlag;
    }
    
    cl_mem getBuffer() const
    {
        return buffer;
    }

private:
    cl_context context;
    size_t kernelArgumentId;
    size_t bufferSize;
    size_t elementSize;
    ArgumentDataType dataType;
    ArgumentMemoryLocation memoryLocation;
    ArgumentAccessType accessType;
    cl_mem_flags openclMemoryFlag;
    cl_mem buffer;
};

} // namespace ktt