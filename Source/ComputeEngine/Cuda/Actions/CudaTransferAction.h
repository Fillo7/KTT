#pragma once

#ifdef KTT_API_CUDA

#include <memory>

#include <ComputeEngine/Cuda/CudaEvent.h>
#include <ComputeEngine/TransferResult.h>
#include <KttTypes.h>

namespace ktt
{

class CudaTransferAction
{
public:
    CudaTransferAction(const TransferActionId id);

    void IncreaseOverhead(const Nanoseconds overhead);
    void WaitForFinish();

    TransferActionId GetId() const;
    CUevent GetStartEvent() const;
    CUevent GetEndEvent() const;
    Nanoseconds GetDuration() const;
    Nanoseconds GetOverhead() const;
    TransferResult GenerateResult() const;

private:
    TransferActionId m_Id;
    std::unique_ptr<CudaEvent> m_StartEvent;
    std::unique_ptr<CudaEvent> m_EndEvent;
    Nanoseconds m_Overhead;
};

} // namespace ktt

#endif // KTT_API_CUDA
