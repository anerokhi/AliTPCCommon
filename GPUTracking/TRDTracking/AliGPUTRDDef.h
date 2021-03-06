#ifndef ALIGPUTRDDEF_H
#define ALIGPUTRDDEF_H

#ifdef GPUCA_ALIROOT_LIB
typedef double My_Float;
#else
typedef float My_Float;
#endif

#ifdef GPUCA_ALIROOT_LIB
#define TRD_TRACK_TYPE_ALIROOT
#else
#define TRD_TRACK_TYPE_O2
#endif

#if defined (TRD_TRACK_TYPE_ALIROOT)
class AliExternalTrackParam;
typedef AliExternalTrackParam TRDBaseTrack;
//class AliGPUTPCGMTrackParam;
//typedef AliGPUTPCGMTrackParam TRDBaseTrack;
#elif defined (TRD_TRACK_TYPE_O2)
class AliGPUTPCGMTrackParam;
typedef AliGPUTPCGMTrackParam TRDBaseTrack;
#endif

#ifdef GPUCA_ALIROOT_LIB
class AliTrackerBase;
typedef AliTrackerBase TRDBasePropagator;
//class AliGPUTPCGMPropagator;
//typedef AliGPUTPCGMPropagator TRDBasePropagator;
#else
class AliGPUTPCGMPropagator;
typedef AliGPUTPCGMPropagator TRDBasePropagator;
#endif

template <class T> class trackInterface;
template <class T> class propagatorInterface;
template <class T> class AliGPUTRDTrack;
typedef AliGPUTRDTrack<trackInterface<TRDBaseTrack>> GPUTRDTrack;
typedef propagatorInterface<TRDBasePropagator> GPUTRDPropagator;

#if !defined(GPUCA_ALIROOT_LIB) && !defined(__CLING__) && !defined(__ROOTCLING__)
#define Error(...)
#define Warning(...)
#define Info(...)
#endif

#endif
