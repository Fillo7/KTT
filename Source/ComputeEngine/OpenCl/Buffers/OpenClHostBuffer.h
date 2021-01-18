#pragma once

#ifdef KTT_API_OPENCL

#include <CL/cl.h>

#include <ComputeEngine/OpenCl/Buffers/OpenClBuffer.h>
#include <ComputeEngine/ActionIdGenerator.h>
#include <KernelArgument/KernelArgument.h>

namespace ktt
{

class OpenClHostBuffer : public OpenClBuffer
{
public:
    explicit OpenClHostBuffer(KernelArgument& argument, ActionIdGenerator& generator, const OpenClContext& context);
    explicit OpenClHostBuffer(KernelArgument& argument, ActionIdGenerator& generator, ComputeBuffer userBuffer);
    ~OpenClHostBuffer() override;

    std::unique_ptr<OpenClTransferAction> UploadData(const OpenClCommandQueue& queue, const void* source,
        const size_t dataSize) override;
    std::unique_ptr<OpenClTransferAction> DownloadData(const OpenClCommandQueue& queue, void* destination,
        const size_t dataSize) const override;
    std::unique_ptr<OpenClTransferAction> CopyData(const OpenClCommandQueue& queue, const OpenClBuffer& source,
        const size_t dataSize) override;
    void Resize(const OpenClCommandQueue& queue, const size_t newSize, const bool preserveData) override;

    ArgumentId GetArgumentId() const override;
    ArgumentAccessType GetAccessType() const override;
    ArgumentMemoryLocation GetMemoryLocation() const override;
    cl_mem GetBuffer() const override;
    void* GetRawBuffer() override;
    size_t GetSize() const override;

private:
    KernelArgument& m_Argument;
    ActionIdGenerator& m_Generator;
    cl_context m_Context;
    cl_mem m_Buffer;
    void* m_RawBuffer;
    size_t m_BufferSize;
    cl_mem_flags m_MemoryFlags;
    bool m_UserOwned;

    bool IsZeroCopy() const;
};

} // namespace ktt

#endif // KTT_API_OPENCL