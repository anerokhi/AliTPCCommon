// $Id$
//* This file is property of and copyright by the ALICE HLT Project        *
//* ALICE Experiment at CERN, All rights reserved.                         *
//* See cxx source for full Copyright notice                               *

#ifndef ALIHLTTRDINTERFACES_H
#define ALIHLTTRDINTERFACES_H

/**
 * @class this is an interface header for making the TRD tracking portable between O2, AliRoot, and HLT standalone framework
 */

template <typename T> class trackInterface;
template <typename T> class propagatorInterface;

#ifdef HLTCA_BUILD_ALIROOT_LIB //Interface for AliRoot, build only with AliRoot
#include "AliExternalTrackParam.h"
#include "AliTrackerBase.h"
template <> class trackInterface<AliExternalTrackParam> : public AliExternalTrackParam
{
  typedef double My_Float;

  public:
    trackInterface<AliExternalTrackParam>() : AliExternalTrackParam() {};
    trackInterface<AliExternalTrackParam>(const trackInterface<AliExternalTrackParam> &param) : AliExternalTrackParam(param) {};
    trackInterface<AliExternalTrackParam>(const AliExternalTrackParam &param) : AliExternalTrackParam(param) {};

    // parameter + covariance
    float getX()       const { return GetX(); }
    float getAlpha()   const { return GetAlpha(); }
    float getY()       const { return GetY(); }
    float getZ()       const { return GetZ(); }
    float getSnp()     const { return GetSnp(); }
    float getTgl()     const { return GetTgl(); }
    float getQ2Pt()    const { return GetSigned1Pt(); }
    float getEta()     const { return Eta(); }
    float getPt()      const { return Pt(); }
    float getSigmaY2() const { return GetSigmaY2(); }
    float getSigmaZ2() const { return GetSigmaZ2(); }

    const My_Float *getCov() const { return GetCovariance(); }

    // parameter manipulation
    bool update(const My_Float p[2], const My_Float cov[3])                  { return Update(p, cov); }
    float getPredictedChi2(const My_Float p[2], const My_Float cov[3]) const { return GetPredictedChi2(p, cov); }
    bool rotate(float alpha)                                                 { return Rotate(alpha); }

    void set(float x, float alpha, const float param[5], const float cov[15])  { Set(x, alpha, param, cov); }

    typedef AliExternalTrackParam baseClass;
};

template <> class propagatorInterface<AliTrackerBase> : public AliTrackerBase
{
  public:
    bool PropagateToX(AliExternalTrackParam *trk, float x, float maxSnp, float maxStep) {
      return PropagateTrackToBxByBz(trk, x, 0.13957, maxStep, false, maxSnp);
    }
};
#endif

#ifdef HLTCA_BUILD_O2_LIB //Interface for O2, build only with AliRoot
#endif
#ifndef HLTCA_BUILD_ALIROOT_LIB
#define Error(...)
#define Warning(...)
#define Info(...)
#endif

#include "AliHLTTPCGMTrackParam.h"
#include "AliHLTTPCGMPropagator.h"
#include "AliHLTTPCGMMerger.h"
#include "AliHLTTPCCAParam.h"

template <> class propagatorInterface<AliHLTTPCGMPropagator>;

template <> class trackInterface<AliHLTTPCGMTrackParam> : public AliHLTTPCGMTrackParam
{
  public:
    trackInterface<AliHLTTPCGMTrackParam>() : AliHLTTPCGMTrackParam() {};
    trackInterface<AliHLTTPCGMTrackParam>(const trackInterface<AliHLTTPCGMTrackParam> &param) : AliHLTTPCGMTrackParam() {}; // FIXME set params
    trackInterface<AliHLTTPCGMTrackParam>(const AliHLTTPCGMTrackParam &param) : AliHLTTPCGMTrackParam() {}; // FIXME set params, or is it dummy?

    float getX()       const { return GetX(); }
    float getAlpha()   const;
    float getY()       const { return GetY(); }
    float getZ()       const { return GetZ(); }
    float getSnp()     const { return GetSinPhi(); }
    float getTgl()     const { return GetDzDs(); }
    float getQ2Pt()    const { return GetQPt(); }
    float getEta()     const { return -logf( tanf( 0.5 * (0.5 * M_PI - atanf(getTgl())) ) ); }
    float getPt()      const { return fabs(getQ2Pt() > 0) ? fabs(1./getQ2Pt()) : 99999; }
    float getSigmaY2() const { return GetErr2Y(); }
    float getSigmaZ2() const { return GetErr2Z(); }

    const float *getCov() const { return GetCov(); }

    propagatorInterface<AliHLTTPCGMPropagator> *fProp;

    // parameter manipulation
    AliHLTTPCCAParam fParam;
    bool update(const float p[2], const float cov[3]); // TODO what about the tracklet covariance?
    float getPredictedChi2(const float p[2], const float cov[3]) const { return 99999; } // TODO not available for HLT tracking?
    bool rotate(float alpha);

    void set(float x, float alpha, const float param[5], const float cov[15]);

    typedef AliHLTTPCGMTrackParam baseClass;
};

template <> class propagatorInterface<AliHLTTPCGMPropagator> : public AliHLTTPCGMPropagator
{
  public:
    propagatorInterface<AliHLTTPCGMPropagator>() : AliHLTTPCGMPropagator() {
      static constexpr float kRho = 1.025e-3;
      static constexpr float kRadLen = 29.532;
      static AliHLTTPCGMMerger fMerger;
      this->SetMaterial( kRadLen, kRho );
      this->SetPolynomialField( fMerger.pField() );
      this->SetMaxSinPhi( HLTCA_MAX_SIN_PHI );
      this->SetToyMCEventsFlag(0);
      this->SetFitInProjections(0);
    };
    bool PropagateToX( trackInterface<AliHLTTPCGMTrackParam> *trk, float x, float maxSnp, float maxStep );
};

inline bool trackInterface<AliHLTTPCGMTrackParam>::update(const float p[2], const float cov[3])
{
  AliHLTTPCCAParam param;
  return fProp->Update(p[0], p[1], HLTCA_ROW_COUNT - 1, param, 0, false, false);
}

inline float trackInterface<AliHLTTPCGMTrackParam>::getAlpha() const
{
  return fProp->GetAlpha();
}

inline bool trackInterface<AliHLTTPCGMTrackParam>::rotate(float alpha)
{
  return fProp->RotateToAlpha(alpha);
}

inline void trackInterface<AliHLTTPCGMTrackParam>::set(float x, float alpha, const float param[5], const float cov[15])
{
  SetX(x);
  for (int i=0; i<5; i++) {
    SetPar(i, param[i]);
  }
  for (int j=0; j<15; j++) {
    SetCov(j, cov[j]);
  }
  fProp->SetTrack(this, alpha);
}

inline bool propagatorInterface<AliHLTTPCGMPropagator>::PropagateToX(trackInterface<AliHLTTPCGMTrackParam> *trk, float x, float maxSnp, float maxStep)
{
  return trk->fProp->PropagateToXAlpha( x, GetAlpha(), true); // FIXME determine propagation direction
}

#ifdef HLTCA_BUILD_ALIROOT_LIB
typedef propagatorInterface<AliTrackerBase> HLTTRDPropagator;
#else
typedef propagatorInterface<AliHLTTPCGMPropagator> HLTTRDPropagator;
#endif
#endif
