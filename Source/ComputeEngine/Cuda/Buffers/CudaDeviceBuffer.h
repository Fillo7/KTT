#pragma once

#ifdef KTT_API_CUDA

#include <ComputeEngine/Cuda/Buffers/CudaBuffer.h>

namespace ktt
{

class CudaDeviceBuffer : public CudaBuffer
{
public:
    explicit CudaDeviceBuffer(KernelArgument& argument, IdGenerator<TransferActionId>& generator);
    explicit CudaDeviceBuffer(KernelArgument& argument, IdGenerator<TransferActionId>& generator, ComputeBuffer userBuffer);
    ~CudaDeviceBuffer() override;

    std::unique_ptr<CudaTransferAction> UploadData(const CudaStream& stream, const void* source,
        const size_t dataSize) override;
    std::unique_ptr<CudaTransferAction> DownloadData(const CudaStream& stream, void* destination,
        const size_t dataSize) const override;
    std::unique_ptr<CudaTransferAction> CopyData(const CudaStream& stream, const CudaBuffer& source,
        const size_t dataSize) override;
    void Resize(const size_t newSize, const bool preserveData) override;
};

} // namespace ktt

#endif // KTT_API_CUDA
