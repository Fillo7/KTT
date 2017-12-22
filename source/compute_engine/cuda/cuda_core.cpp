#include <stdexcept>
#include "cuda_core.h"
#include "utility/ktt_utility.h"
#include "utility/timer.h"

namespace ktt
{

#ifdef PLATFORM_CUDA

CudaCore::CudaCore(const size_t deviceIndex, const size_t queueCount) :
    deviceIndex(deviceIndex),
    queueCount(queueCount),
    compilerOptions(std::string("--gpu-architecture=compute_30")),
    globalSizeType(GlobalSizeType::Cuda),
    globalSizeCorrection(false),
	programCacheFlag(false)
{
    checkCudaError(cuInit(0), "cuInit");

    auto devices = getCudaDevices();
    if (deviceIndex >= devices.size())
    {
        throw std::runtime_error(std::string("Invalid device index: ") + std::to_string(deviceIndex));
    }

    context = std::make_unique<CudaContext>(devices.at(deviceIndex).getDevice());
    for (size_t i = 0; i < queueCount; i++)
    {
        auto stream = std::make_unique<CudaStream>(i, context->getContext(), devices.at(deviceIndex).getDevice());
        streams.push_back(std::move(stream));
    }
}

KernelResult CudaCore::runKernel(const KernelRuntimeData& kernelData, const std::vector<KernelArgument*>& argumentPointers,
    const std::vector<ArgumentOutputDescriptor>& outputDescriptors)
{
	std::unique_ptr<CudaProgram> program;
	CudaProgram* programPointer;

	if (programCacheFlag)
	{
		if (programCache.find(kernelData.getSource()) == programCache.end())
		{
			program = createAndBuildProgram(kernelData.getSource());
			programCache.insert(std::make_pair(kernelData.getSource(), std::move(program)));
		}
		auto cachePointer = programCache.find(kernelData.getSource());
		programPointer = cachePointer->second.get();
	}
	else
	{
		program = createAndBuildProgram(kernelData.getSource());
		programPointer = program.get();
	}
	std::unique_ptr<CudaKernel> kernel = createKernel(*programPointer, kernelData.getName());
    std::vector<CUdeviceptr*> kernelArguments = getKernelArguments(argumentPointers);

    float duration = enqueueKernel(*kernel, kernelData.getGlobalSize(), kernelData.getLocalSize(), kernelArguments,
        getSharedMemorySizeInBytes(argumentPointers), getDefaultQueue(), true);

    for (const auto& descriptor : outputDescriptors)
    {
        if (descriptor.getOutputSizeInBytes() == 0)
        {
            downloadArgument(descriptor.getArgumentId(), descriptor.getOutputDestination());
        }
        else
        {
            downloadArgument(descriptor.getArgumentId(), descriptor.getOutputDestination(), descriptor.getOutputSizeInBytes());
        }
    }

    return KernelResult(kernelData.getName(), static_cast<uint64_t>(duration));
}

void CudaCore::runKernel(const KernelRuntimeData& kernelData, const std::vector<KernelArgument*>& argumentPointers, const QueueId queue,
    const bool synchronizeFlag)
{
	std::unique_ptr<CudaProgram> program;
	CudaProgram* programPointer;

	if (programCacheFlag)
	{
		if (programCache.find(kernelData.getSource()) == programCache.end())
		{
			program = createAndBuildProgram(kernelData.getSource());
			programCache.insert(std::make_pair(kernelData.getSource(), std::move(program)));
		}
		auto cachePointer = programCache.find(kernelData.getSource());
		programPointer = cachePointer->second.get();
	}
	else
	{
		program = createAndBuildProgram(kernelData.getSource());
		programPointer = program.get();
	}
    std::unique_ptr<CudaKernel> kernel = createKernel(*programPointer, kernelData.getName());
    std::vector<CUdeviceptr*> kernelArguments = getKernelArguments(argumentPointers);

    enqueueKernel(*kernel, kernelData.getGlobalSize(), kernelData.getLocalSize(), kernelArguments, getSharedMemorySizeInBytes(argumentPointers),
        queue, synchronizeFlag);
}

void CudaCore::setCompilerOptions(const std::string& options)
{
    compilerOptions = options;
}

void CudaCore::setGlobalSizeType(const GlobalSizeType& type)
{
    globalSizeType = type;
}

void CudaCore::setAutomaticGlobalSizeCorrection(const bool flag)
{
    globalSizeCorrection = flag;
}

void CudaCore::setProgramCache(const bool flag)
{
	clearProgramCache();
	programCacheFlag = flag;
}

void CudaCore::clearProgramCache()
{
	programCache.clear();
}

QueueId CudaCore::getDefaultQueue() const
{
    return 0;
}

std::vector<QueueId> CudaCore::getAllQueues() const
{
    std::vector<QueueId> result;

    for (size_t i = 0; i < streams.size(); i++)
    {
        result.push_back(i);
    }

    return result;
}

void CudaCore::synchronizeQueue(const QueueId queue)
{
    if (queue >= streams.size())
    {
        throw std::runtime_error(std::string("Invalid stream index: ") + std::to_string(queue));
    }

    checkCudaError(cuStreamSynchronize(streams.at(queue)->getStream()), "cuStreamSynchronize");
}

void CudaCore::synchronizeDevice()
{
    for (auto& stream : streams)
    {
        checkCudaError(cuStreamSynchronize(stream->getStream()), "cuStreamSynchronize");
    }
}

void CudaCore::uploadArgument(KernelArgument& kernelArgument)
{
    if (kernelArgument.getUploadType() != ArgumentUploadType::Vector)
    {
        return;
    }
    
    clearBuffer(kernelArgument.getId());
    std::unique_ptr<CudaBuffer> buffer = nullptr;

    if (kernelArgument.getMemoryLocation() == ArgumentMemoryLocation::HostZeroCopy)
    {
        buffer = std::make_unique<CudaBuffer>(kernelArgument, true);
    }
    else
    {
        buffer = std::make_unique<CudaBuffer>(kernelArgument, false);
        buffer->uploadData(kernelArgument.getData(), kernelArgument.getDataSizeInBytes());
    }

    buffers.insert(std::move(buffer)); // buffer data will be stolen
}

void CudaCore::uploadArgument(KernelArgument& kernelArgument, const QueueId queue, const bool synchronizeFlag)
{
    if (queue >= streams.size())
    {
        throw std::runtime_error(std::string("Invalid stream index: ") + std::to_string(queue));
    }

    if (kernelArgument.getUploadType() != ArgumentUploadType::Vector)
    {
        return;
    }
    
    clearBuffer(kernelArgument.getId());
    std::unique_ptr<CudaBuffer> buffer = nullptr;

    if (kernelArgument.getMemoryLocation() == ArgumentMemoryLocation::HostZeroCopy)
    {
        buffer = std::make_unique<CudaBuffer>(kernelArgument, true);
    }
    else
    {
        buffer = std::make_unique<CudaBuffer>(kernelArgument, false);
        buffer->uploadData(streams.at(queue)->getStream(), kernelArgument.getData(), kernelArgument.getDataSizeInBytes(), synchronizeFlag);
    }

    buffers.insert(std::move(buffer)); // buffer data will be stolen
}

void CudaCore::updateArgument(const ArgumentId id, const void* data, const size_t dataSizeInBytes)
{
    CudaBuffer* buffer = findBuffer(id);

    if (buffer == nullptr)
    {
        throw std::runtime_error(std::string("Buffer with following id was not found: ") + std::to_string(id));
    }

    buffer->uploadData(data, dataSizeInBytes);
}

void CudaCore::updateArgument(const ArgumentId id, const void* data, const size_t dataSizeInBytes, const QueueId queue, const bool synchronizeFlag)
{
    if (queue >= streams.size())
    {
        throw std::runtime_error(std::string("Invalid stream index: ") + std::to_string(queue));
    }

    CudaBuffer* buffer = findBuffer(id);

    if (buffer == nullptr)
    {
        throw std::runtime_error(std::string("Buffer with following id was not found: ") + std::to_string(id));
    }

    buffer->uploadData(streams.at(queue)->getStream(), data, dataSizeInBytes, synchronizeFlag);
}

void CudaCore::downloadArgument(const ArgumentId id, void* destination) const
{
    CudaBuffer* buffer = findBuffer(id);

    if (buffer == nullptr)
    {
        throw std::runtime_error(std::string("Buffer with following id was not found: ") + std::to_string(id));
    }

    buffer->downloadData(destination, buffer->getBufferSize());
}

void CudaCore::downloadArgument(const ArgumentId id, void* destination, const QueueId queue, const bool synchronizeFlag) const
{
    if (queue >= streams.size())
    {
        throw std::runtime_error(std::string("Invalid stream index: ") + std::to_string(queue));
    }

    CudaBuffer* buffer = findBuffer(id);

    if (buffer == nullptr)
    {
        throw std::runtime_error(std::string("Buffer with following id was not found: ") + std::to_string(id));
    }

    buffer->downloadData(streams.at(queue)->getStream(), destination, buffer->getBufferSize(), synchronizeFlag);
}

void CudaCore::downloadArgument(const ArgumentId id, void* destination, const size_t dataSizeInBytes) const
{
    CudaBuffer* buffer = findBuffer(id);

    if (buffer == nullptr)
    {
        throw std::runtime_error(std::string("Buffer with following id was not found: ") + std::to_string(id));
    }

    buffer->downloadData(destination, dataSizeInBytes);
}

void CudaCore::downloadArgument(const ArgumentId id, void* destination, const size_t dataSizeInBytes, const QueueId queue,
    const bool synchronizeFlag) const
{
    if (queue >= streams.size())
    {
        throw std::runtime_error(std::string("Invalid stream index: ") + std::to_string(queue));
    }

    CudaBuffer* buffer = findBuffer(id);

    if (buffer == nullptr)
    {
        throw std::runtime_error(std::string("Buffer with following id was not found: ") + std::to_string(id));
    }

    buffer->downloadData(streams.at(queue)->getStream(), destination, dataSizeInBytes, synchronizeFlag);
}

KernelArgument CudaCore::downloadArgument(const ArgumentId id) const
{
    CudaBuffer* buffer = findBuffer(id);

    if (buffer == nullptr)
    {
        throw std::runtime_error(std::string("Buffer with following id was not found: ") + std::to_string(id));
    }

    KernelArgument argument(buffer->getKernelArgumentId(), buffer->getBufferSize() / buffer->getElementSize(), buffer->getElementSize(),
        buffer->getDataType(), buffer->getMemoryLocation(), buffer->getAccessType(), ArgumentUploadType::Vector);
    buffer->downloadData(argument.getData(), argument.getDataSizeInBytes());
    
    return argument;
}

void CudaCore::clearBuffer(const ArgumentId id)
{
    auto iterator = buffers.cbegin();

    while (iterator != buffers.cend())
    {
        if (iterator->get()->getKernelArgumentId() == id)
        {
            buffers.erase(iterator);
            return;
        }
        else
        {
            ++iterator;
        }
    }
}

void CudaCore::clearBuffers()
{
    buffers.clear();
}

void CudaCore::clearBuffers(const ArgumentAccessType& accessType)
{
    auto iterator = buffers.cbegin();

    while (iterator != buffers.cend())
    {
        if (iterator->get()->getAccessType() == accessType)
        {
            iterator = buffers.erase(iterator);
        }
        else
        {
            ++iterator;
        }
    }
}

void CudaCore::printComputeApiInfo(std::ostream& outputTarget) const
{
    outputTarget << "Platform 0: " << "NVIDIA CUDA" << std::endl;
    auto devices = getCudaDevices();

    for (size_t i = 0; i < devices.size(); i++)
    {
        outputTarget << "Device " << i << ": " << devices.at(i).getName() << std::endl;
    }
    outputTarget << std::endl;
}

std::vector<PlatformInfo> CudaCore::getPlatformInfo() const
{
    int driverVersion;
    checkCudaError(cuDriverGetVersion(&driverVersion), "cuDriverGetVersion");

    PlatformInfo cuda(0, "NVIDIA CUDA");
    cuda.setVendor("NVIDIA Corporation");
    cuda.setVersion(std::to_string(driverVersion));
    cuda.setExtensions("N/A");
    return std::vector<PlatformInfo>{ cuda };
}

std::vector<DeviceInfo> CudaCore::getDeviceInfo(const size_t) const
{
    std::vector<DeviceInfo> result;
    auto devices = getCudaDevices();

    for (size_t i = 0; i < devices.size(); i++)
    {
        result.push_back(getCudaDeviceInfo(i));
    }

    return result;
}

DeviceInfo CudaCore::getCurrentDeviceInfo() const
{
    return getCudaDeviceInfo(deviceIndex);
}

std::unique_ptr<CudaProgram> CudaCore::createAndBuildProgram(const std::string& source) const
{
    auto program = std::make_unique<CudaProgram>(source);
    program->build(compilerOptions);
    return program;
}

std::unique_ptr<CudaEvent> CudaCore::createEvent() const
{
    auto event = std::make_unique<CudaEvent>();
    return event;
}

std::unique_ptr<CudaKernel> CudaCore::createKernel(const CudaProgram& program, const std::string& kernelName) const
{
    auto kernel = std::make_unique<CudaKernel>(program.getPtxSource(), kernelName);
    return kernel;
}

float CudaCore::enqueueKernel(CudaKernel& kernel, const std::vector<size_t>& globalSize, const std::vector<size_t>& localSize,
    const std::vector<CUdeviceptr*>& kernelArguments, const size_t localMemorySize, const QueueId queue, const bool synchronizeFlag) const
{
    if (queue >= streams.size())
    {
        throw std::runtime_error(std::string("Invalid stream index: ") + std::to_string(queue));
    }

    std::vector<void*> kernelArgumentsVoid;
    for (size_t i = 0; i < kernelArguments.size(); i++)
    {
        kernelArgumentsVoid.push_back((void*)kernelArguments.at(i));
    }

    std::vector<size_t> correctedGlobalSize = globalSize;
    if (globalSizeCorrection)
    {
        correctedGlobalSize = roundUpGlobalSize(correctedGlobalSize, localSize);
    }
    if (globalSizeType != GlobalSizeType::Cuda)
    {
        correctedGlobalSize.at(0) /= localSize.at(0);
        correctedGlobalSize.at(1) /= localSize.at(1);
        correctedGlobalSize.at(2) /= localSize.at(2);
    }

    if (!synchronizeFlag)
    {
        checkCudaError(cuLaunchKernel(kernel.getKernel(), static_cast<unsigned int>(correctedGlobalSize.at(0)),
            static_cast<unsigned int>(correctedGlobalSize.at(1)), static_cast<unsigned int>(correctedGlobalSize.at(2)),
            static_cast<unsigned int>(localSize.at(0)), static_cast<unsigned int>(localSize.at(1)), static_cast<unsigned int>(localSize.at(2)),
            static_cast<unsigned int>(localMemorySize), streams.at(queue)->getStream(), kernelArgumentsVoid.data(), nullptr),
            "cuLaunchKernel");
        return 0.0f;
    }

    auto start = createEvent();
    auto end = createEvent();

    checkCudaError(cuEventRecord(start->getEvent(), streams.at(queue)->getStream()), "cuEventRecord");
    checkCudaError(cuLaunchKernel(kernel.getKernel(), static_cast<unsigned int>(correctedGlobalSize.at(0)),
        static_cast<unsigned int>(correctedGlobalSize.at(1)), static_cast<unsigned int>(correctedGlobalSize.at(2)),
        static_cast<unsigned int>(localSize.at(0)), static_cast<unsigned int>(localSize.at(1)), static_cast<unsigned int>(localSize.at(2)),
        static_cast<unsigned int>(localMemorySize), streams.at(queue)->getStream(), kernelArgumentsVoid.data(), nullptr),
        "cuLaunchKernel");
    checkCudaError(cuEventRecord(end->getEvent(), streams.at(queue)->getStream()), "cuEventRecord");

    // Wait for computation to finish
    checkCudaError(cuEventSynchronize(end->getEvent()), "cuEventSynchronize");

    return getKernelRunDuration(start->getEvent(), end->getEvent());
}

std::vector<CudaDevice> CudaCore::getCudaDevices() const
{
    int deviceCount;
    checkCudaError(cuDeviceGetCount(&deviceCount), "cuDeviceGetCount");

    std::vector<CUdevice> deviceIds(deviceCount);
    for (int i = 0; i < deviceCount; i++)
    {
        checkCudaError(cuDeviceGet(&deviceIds.at(i), i), "cuDeviceGet");
    }

    std::vector<CudaDevice> devices;
    for (const auto deviceId : deviceIds)
    {
        std::string name(40, ' ');
        checkCudaError(cuDeviceGetName(&name[0], 40, deviceId), "cuDeviceGetName");
        devices.push_back(CudaDevice(deviceId, name));
    }

    return devices;
}

DeviceInfo CudaCore::getCudaDeviceInfo(const size_t deviceIndex) const
{
    auto devices = getCudaDevices();
    DeviceInfo result(deviceIndex, devices.at(deviceIndex).getName());

    CUdevice id = devices.at(deviceIndex).getDevice();
    result.setExtensions("N/A");
    result.setVendor("NVIDIA Corporation");
    
    size_t globalMemory;
    checkCudaError(cuDeviceTotalMem(&globalMemory, id), "cuDeviceTotalMem");
    result.setGlobalMemorySize(globalMemory);

    int localMemory;
    checkCudaError(cuDeviceGetAttribute(&localMemory, CU_DEVICE_ATTRIBUTE_SHARED_MEMORY_PER_BLOCK, id), "cuDeviceGetAttribute");
    result.setLocalMemorySize(localMemory);

    int constantMemory;
    checkCudaError(cuDeviceGetAttribute(&constantMemory, CU_DEVICE_ATTRIBUTE_TOTAL_CONSTANT_MEMORY, id), "cuDeviceGetAttribute");
    result.setMaxConstantBufferSize(constantMemory);

    int computeUnits;
    checkCudaError(cuDeviceGetAttribute(&computeUnits, CU_DEVICE_ATTRIBUTE_MULTIPROCESSOR_COUNT, id), "cuDeviceGetAttribute");
    result.setMaxComputeUnits(computeUnits);

    int workGroupSize;
    checkCudaError(cuDeviceGetAttribute(&workGroupSize, CU_DEVICE_ATTRIBUTE_MAX_THREADS_PER_BLOCK, id), "cuDeviceGetAttribute");
    result.setMaxWorkGroupSize(workGroupSize);
    result.setDeviceType(DeviceType::GPU);

    return result;
}

std::vector<CUdeviceptr*> CudaCore::getKernelArguments(const std::vector<KernelArgument*>& argumentPointers)
{
    std::vector<CUdeviceptr*> result;

    for (const auto argument : argumentPointers)
    {
        if (argument->getUploadType() == ArgumentUploadType::Local)
        {
            continue;
        }
        else if (argument->getUploadType() == ArgumentUploadType::Vector)
        {
            CUdeviceptr* cachedBuffer = loadBufferFromCache(argument->getId());
            if (cachedBuffer == nullptr)
            {
                uploadArgument(*argument);
                cachedBuffer = loadBufferFromCache(argument->getId());
            }

            result.push_back(cachedBuffer);
        }
        else if (argument->getUploadType() == ArgumentUploadType::Scalar)
        {
            result.push_back((CUdeviceptr*)argument->getData());
        }
    }

    return result;
}

size_t CudaCore::getSharedMemorySizeInBytes(const std::vector<KernelArgument*>& argumentPointers) const
{
    size_t result = 0;

    for (const auto argument : argumentPointers)
    {
        if (argument->getUploadType() == ArgumentUploadType::Local)
        {
            result += argument->getDataSizeInBytes();
        }
    }

    return result;
}

CudaBuffer* CudaCore::findBuffer(const ArgumentId id) const
{
    for (const auto& buffer : buffers)
    {
        if (buffer->getKernelArgumentId() == id)
        {
            return buffer.get();
        }
    }

    return nullptr;
}

CUdeviceptr* CudaCore::loadBufferFromCache(const ArgumentId id) const
{
    CudaBuffer* buffer = findBuffer(id);

    if (buffer != nullptr)
    {
        return buffer->getBuffer();
    }

    return nullptr;
}

#else

CudaCore::CudaCore(const size_t, const size_t)
{
    throw std::runtime_error("Support for CUDA API is not included in this version of KTT library");
}

KernelResult CudaCore::runKernel(const KernelRuntimeData&, const std::vector<KernelArgument*>&, const std::vector<ArgumentOutputDescriptor>&)
{
    throw std::runtime_error("Support for CUDA API is not included in this version of KTT library");
}

void CudaCore::runKernel(const KernelRuntimeData&, const std::vector<KernelArgument*>&, const std::vector<ArgumentOutputDescriptor>&, const QueueId,
    const bool)
{
    throw std::runtime_error("Support for CUDA API is not included in this version of KTT library");
}

void CudaCore::setCompilerOptions(const std::string&)
{
    throw std::runtime_error("Support for CUDA API is not included in this version of KTT library");
}

void CudaCore::setGlobalSizeType(const GlobalSizeType&)
{
    throw std::runtime_error("Support for CUDA API is not included in this version of KTT library");
}

void CudaCore::setAutomaticGlobalSizeCorrection(const bool)
{
    throw std::runtime_error("Support for CUDA API is not included in this version of KTT library");
}

void CudaCore::setProgramCache(const bool)
{
	throw std::runtime_error("Support for CUDA API is not included in this version of KTT library");
}

void CudaCore::clearProgramCache()
{
	throw std::runtime_error("Support for CUDA API is not included in this version of KTT library");
}

QueueId CudaCore::getDefaultQueue() const
{
    throw std::runtime_error("Support for CUDA API is not included in this version of KTT library");
}

std::vector<QueueId> CudaCore::getAllQueues() const
{
    throw std::runtime_error("Support for CUDA API is not included in this version of KTT library");
}

void CudaCore::synchronizeQueue(const QueueId)
{
    throw std::runtime_error("Support for CUDA API is not included in this version of KTT library");
}

void CudaCore::synchronizeDevice()
{
    throw std::runtime_error("Support for CUDA API is not included in this version of KTT library");
}

void CudaCore::uploadArgument(KernelArgument&)
{
    throw std::runtime_error("Support for CUDA API is not included in this version of KTT library");
}

void CudaCore::uploadArgument(KernelArgument&, const QueueId, const bool)
{
    throw std::runtime_error("Support for CUDA API is not included in this version of KTT library");
}

void CudaCore::updateArgument(const ArgumentId, const void*, const size_t)
{
    throw std::runtime_error("Support for CUDA API is not included in this version of KTT library");
}

void CudaCore::updateArgument(const ArgumentId, const void*, const size_t, const QueueId, const bool)
{
    throw std::runtime_error("Support for CUDA API is not included in this version of KTT library");
}

void CudaCore::downloadArgument(const ArgumentId, void*) const
{
    throw std::runtime_error("Support for CUDA API is not included in this version of KTT library");
}

void CudaCore::downloadArgument(const ArgumentId, void*, const QueueId, const bool) const
{
    throw std::runtime_error("Support for CUDA API is not included in this version of KTT library");
}

void CudaCore::downloadArgument(const ArgumentId void*, const size_t) const
{
    throw std::runtime_error("Support for CUDA API is not included in this version of KTT library");
}

void CudaCore::downloadArgument(const ArgumentId, void*, const size_t, const QueueId, const bool) const
{
    throw std::runtime_error("Support for CUDA API is not included in this version of KTT library");
}

KernelArgument CudaCore::downloadArgument(const ArgumentId) const
{
    throw std::runtime_error("Support for CUDA API is not included in this version of KTT library");
}

void CudaCore::clearBuffer(const ArgumentId)
{
    throw std::runtime_error("Support for CUDA API is not included in this version of KTT library");
}

void CudaCore::clearBuffers()
{
    throw std::runtime_error("Support for CUDA API is not included in this version of KTT library");
}

void CudaCore::clearBuffers(const ArgumentAccessType&)
{
    throw std::runtime_error("Support for CUDA API is not included in this version of KTT library");
}

void CudaCore::printComputeApiInfo(std::ostream&) const
{
    throw std::runtime_error("Support for CUDA API is not included in this version of KTT library");
}

std::vector<PlatformInfo> CudaCore::getPlatformInfo() const
{
    throw std::runtime_error("Support for CUDA API is not included in this version of KTT library");
}

std::vector<DeviceInfo> CudaCore::getDeviceInfo(const size_t) const
{
    throw std::runtime_error("Support for CUDA API is not included in this version of KTT library");
}

DeviceInfo CudaCore::getCurrentDeviceInfo() const
{
    throw std::runtime_error("Support for CUDA API is not included in this version of KTT library");
}

#endif // PLATFORM_CUDA

} // namespace ktt
