#ifndef ALIGPURECONSTRUCTIONIMPL_H
#define ALIGPURECONSTRUCTIONIMPL_H

#include "AliGPUReconstruction.h"
#include "AliGPUConstantMem.h"
#include <stdexcept>
#include "utils/timer.h"

#include "AliGPUGeneralKernels.h"
#include "AliGPUTPCNeighboursFinder.h"
#include "AliGPUTPCNeighboursCleaner.h"
#include "AliGPUTPCStartHitsFinder.h"
#include "AliGPUTPCStartHitsSorter.h"
#include "AliGPUTPCTrackletConstructor.h"
#include "AliGPUTPCTrackletSelector.h"
#include "AliGPUTPCGMMergerGPU.h"
#include "AliGPUTRDTrackerGPU.h"

namespace AliGPUReconstruction_krnlHelpers {
template <class T, int I = 0> class classArgument {};

typedef void deviceEvent; //We use only pointers anyway, and since cl_event and cudaEvent_t are actually pointers, we can cast them to deviceEvent* this way.

enum class krnlDeviceType : int {CPU = 0, Device = 1, Auto = -1};
struct krnlExec
{
	krnlExec(unsigned int b, unsigned int t, int s, krnlDeviceType d = krnlDeviceType::Auto) : nBlocks(b), nThreads(t), stream(s), device(d) {}
	unsigned int nBlocks;
	unsigned int nThreads;
	int stream;
	krnlDeviceType device;
};
struct krnlRunRange
{
	krnlRunRange() : start(0), num(0) {}
	krnlRunRange(unsigned int a) : start(a), num(0) {}
	krnlRunRange(unsigned int s, int n) : start(s), num(n) {}
	
	unsigned int start;
	int num;
};
static const krnlRunRange krnlRunRangeNone(0, -1);
struct krnlEvent
{
	krnlEvent(deviceEvent* e = nullptr, deviceEvent* el = nullptr, int n = 1) : ev(e), evList(el), nEvents(n) {}
	deviceEvent* ev;
	deviceEvent* evList;
	int nEvents;
};
} //End Namespace

using namespace AliGPUReconstruction_krnlHelpers;

class AliGPUReconstructionCPUBackend : public AliGPUReconstruction
{
public:
	virtual ~AliGPUReconstructionCPUBackend() = default;
	
protected:
	AliGPUReconstructionCPUBackend(const AliGPUSettingsProcessing& cfg) : AliGPUReconstruction(cfg) {}
	template <class T, int I = 0, typename... Args> int runKernelBackend(const krnlExec& x, const krnlRunRange& y, const krnlEvent& z, const Args&... args);
};

#include "AliGPUReconstructionKernels.h"
#ifndef GPUCA_ALIGPURECONSTRUCTIONCPU_IMPLEMENTATION
	#define GPUCA_ALIGPURECONSTRUCTIONCPU_DECLONLY
	#undef ALIGPURECONSTRUCTIONKERNELS_H
	#include "AliGPUReconstructionKernels.h"
#endif

class AliGPUReconstructionCPU : public AliGPUReconstructionKernels<AliGPUReconstructionCPUBackend>
{
	friend class AliGPUReconstruction;
	
public:
	virtual ~AliGPUReconstructionCPU() = default;

#ifdef __APPLE__ //MacOS compiler BUG: clang seems broken and does not accept default parameters before parameter pack
	template <class S, int I = 0> inline int runKernel(const krnlExec& x, HighResTimer* t = nullptr, const krnlRunRange& y = krnlRunRangeNone)
	{
		return runKernel<S, I>(x, t, y, krnlEvent());
	}
	template <class S, int I = 0, typename... Args> inline int runKernel(const krnlExec& x, HighResTimer* t, const krnlRunRange& y, const krnlEvent& z, const Args&... args)
#else
	template <class S, int I = 0, typename... Args> inline int runKernel(const krnlExec& x, HighResTimer* t = nullptr, const krnlRunRange& y = krnlRunRangeNone, const krnlEvent& z = krnlEvent(), const Args&... args)
#endif
	{
		if (mDeviceProcessingSettings.debugLevel >= 3) printf("Running %s Stream %d (Range %d/%d)\n", typeid(S).name(), x.stream, y.start, y.num);
		if (t && mDeviceProcessingSettings.debugLevel) t->Start();
		if (runKernelImpl(classArgument<S, I>(), x, y, z, args...)) return 1;
		if (mDeviceProcessingSettings.debugLevel)
		{
			if (GPUDebug(typeid(S).name(), x.stream)) throw std::runtime_error("kernel failure");
			if (t) t->Stop();
		}
		return 0;
	}
	
	virtual int RunTPCTrackingSlices();
	virtual int RunTPCTrackingMerger();
	virtual int RefitMergedTracks(bool resetTimers);
	virtual int RunTRDTracking();
	virtual int RunStandalone();
	
	virtual int GPUDebug(const char* state = "UNKNOWN", int stream = -1);
	
	void TransferMemoryResourceToGPU(AliGPUMemoryResource* res, int stream = -1, deviceEvent* ev = nullptr, deviceEvent* evList = nullptr, int nEvents = 1) {TransferMemoryInternal(res, stream, ev, evList, nEvents, true, res->Ptr(), res->PtrDevice());}
	void TransferMemoryResourceToHost(AliGPUMemoryResource* res, int stream = -1, deviceEvent* ev = nullptr, deviceEvent* evList = nullptr, int nEvents = 1) {TransferMemoryInternal(res, stream, ev, evList, nEvents, false, res->PtrDevice(), res->Ptr());}
	virtual void TransferMemoryInternal(AliGPUMemoryResource* res, int stream, deviceEvent* ev, deviceEvent* evList, int nEvents, bool toGPU, void* src, void* dst);
	virtual void WriteToConstantMemory(size_t offset, const void* src, size_t size, int stream = -1, deviceEvent* ev = nullptr);

	void TransferMemoryResourcesToGPU(AliGPUProcessor* proc, int stream = -1, bool all = false) {TransferMemoryResourcesHelper(proc, stream, all, true);}
	void TransferMemoryResourcesToHost(AliGPUProcessor* proc, int stream = -1, bool all = false) {TransferMemoryResourcesHelper(proc, stream, all, false);}
	void TransferMemoryResourceLinkToGPU(short res, int stream = -1, deviceEvent* ev = nullptr, deviceEvent* evList = nullptr, int nEvents = 1) {TransferMemoryResourceToGPU(&mMemoryResources[res], stream, ev, evList, nEvents);}
	void TransferMemoryResourceLinkToHost(short res, int stream = -1, deviceEvent* ev = nullptr, deviceEvent* evList = nullptr, int nEvents = 1) {TransferMemoryResourceToHost(&mMemoryResources[res], stream, ev, evList, nEvents);}
	
	HighResTimer timerTPCtracking[NSLICES][10];
	
protected:
	AliGPUReconstructionCPU(const AliGPUSettingsProcessing& cfg) : AliGPUReconstructionKernels(cfg) {}

private:
	void TransferMemoryResourcesHelper(AliGPUProcessor* proc, int stream, bool all, bool toGPU);
};

#endif