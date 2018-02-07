/**************************************************************************
* Copyright(c) 1998-1999, ALICE Experiment at CERN, All rights reserved. *
*                                                                        *
* Author: The ALICE Off-line Project.                                    *
* Contributors are mentioned in the code where appropriate.              *
*                                                                        *
* Permission to use, copy, modify and distribute this software and its   *
* documentation strictly for non-commercial purposes is hereby granted   *
* without fee, provided that the above copyright notice appears in all   *
* copies and that both the copyright notice and this permission notice   *
* appear in the supporting documentation. The authors make no claims     *
* about the suitability of this software for any purpose. It is          *
* provided "as is" without express or implied warranty.                  *
**************************************************************************/

// AliAnalysisTaskFlowModes - ALICE Flow framework
//
// ALICE analysis task for universal study of flow.
// Note: So far implemented only for AOD analysis!
// Author: Vojtech Pacik
// Modified by: Naghmeh Mohammadi to include non-linear terms, Nikhef, 2017
// Generic framework by: You Zhou

#include <vector>

#include <TDatabasePDG.h>
#include <TPDGCode.h>

#include "TFile.h"
#include "TChain.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TH3D.h"
#include "TProfile.h"
#include "TProfile2D.h"
#include "TProfile3D.h"
#include "TList.h"
#include "TComplex.h"
#include "TRandom3.h"

#include "AliAnalysisTask.h"
#include "AliAnalysisManager.h"
#include "AliAODInputHandler.h"
#include "AliAnalysisUtils.h"
#include "AliMultSelection.h"
#include "AliPIDResponse.h"
#include "AliPIDCombined.h"
#include "AliAnalysisTaskFlowModes.h"
#include "AliLog.h"
#include "AliAODEvent.h"
#include "AliESDEvent.h"
#include "AliAODTrack.h"
#include "AliAODv0.h"
#include "AliVTrack.h"
#include "AliESDpid.h"
#include "AliFlowBayesianPID.h"


class AliAnalysisTaskFlowModes;

ClassImp(AliAnalysisTaskFlowModes); // classimp: necessary for root

Int_t AliAnalysisTaskFlowModes::fHarmonics[] = {2,3};
Int_t AliAnalysisTaskFlowModes::fMixedHarmonics[] = {422,633,523};// 422: v4{psi2}, 523: v5{psi2, psi3} and 633: v6{psi3}

Double_t AliAnalysisTaskFlowModes::fEtaGap[] = {0.,0.4,0.8};



AliAnalysisTaskFlowModes::AliAnalysisTaskFlowModes() : AliAnalysisTaskSE(),
  fEventAOD(0x0),
  fPIDResponse(0x0),
  fPIDCombined(0x0),
  fFlowWeightsFile(0x0),
  fInit(kFALSE),
  fIndexSampling(0),
  fIndexCentrality(-1),
  fEventCounter(0),
  fNumEventsAnalyse(50),
  fRunNumber(-1),
  fPDGMassPion(TDatabasePDG::Instance()->GetParticle(211)->Mass()),
  fPDGMassKaon(TDatabasePDG::Instance()->GetParticle(321)->Mass()),
  fPDGMassProton(TDatabasePDG::Instance()->GetParticle(2212)->Mass()),

  // FlowPart containers
  fVectorCharged(0x0),
  fVectorPion(0x0),
  fVectorKaon(0x0),
  fVectorProton(0x0),

  // analysis selection
  fRunMode(kFull),
  fAnalType(kAOD),
  fSampling(kFALSE),
  fFillQA(kTRUE),
  //fNumSamples(10),
  fProcessCharged(kFALSE),
  fProcessPID(kFALSE),
  fBayesianResponse(NULL),

  // flow related
  fCutFlowRFPsPtMin(0),
  fCutFlowRFPsPtMax(0),
  fCutFlowDoFourCorrelations(kTRUE),
  fDoOnlyMixedCorrelations(kFALSE),
  fFlowFillWeights(kFALSE),
  fFlowPOIsPtMin(0),
  fFlowPOIsPtMax(20.),
  fFlowCentMin(0),
  fFlowCentMax(150),
  fFlowCentNumBins(150),
  fFlowWeightsPath(),
  fFlowUseWeights(kFALSE),
  fPositivelyChargedRef(kFALSE),
  fNegativelyChargedRef(kFALSE),


  // events selection
  fPVtxCutZ(0.),
  fMultEstimator(),
  fTrigger(0),
  fFullCentralityRange(kTRUE),
  // charged tracks selection
  fCutChargedEtaMax(0),
  fCutChargedPtMax(0),
  fCutChargedPtMin(0),
  fCutChargedDCAzMax(0),
  fCutChargedDCAxyMax(0),
  fCutChargedTrackFilterBit(0),
  fCutChargedNumTPCclsMin(0),

  // PID tracks selection
  fESDpid(),
  fCutPIDUseAntiProtonOnly(kFALSE),
  fPID3sigma(kTRUE),
  fPIDbayesian(kFALSE),
  fCutPIDnSigmaPionMax(3),
  fCutPIDnSigmaKaonMax(3),
  fCutPIDnSigmaProtonMax(3),
  fCutPIDnSigmaTPCRejectElectron(3),
  fCutPIDnSigmaCombinedNoTOFrejection(kFALSE),
  fCurrCentr(0.0),
  fParticleProbability(0.9),

  // output lists
  fQAEvents(0x0),
  fQACharged(0x0),
  fQAPID(0x0),
  fFlowWeights(0x0),
  fFlowRefs(0x0),
  fFlowCharged(0x0),
  fFlowPID(0x0),

  // flow histograms & profiles
  fh3WeightsRefs(0x0),
  fh3WeightsCharged(0x0),
  fh3WeightsPion(0x0),
  fh3WeightsKaon(0x0),
  fh3WeightsProton(0x0),
  fh3AfterWeightsRefs(0x0),
  fh3AfterWeightsCharged(0x0),
  fh3AfterWeightsPion(0x0),
  fh3AfterWeightsKaon(0x0),
  fh3AfterWeightsProton(0x0),
  fh2WeightRefs(0x0),
  fh2WeightCharged(0x0),
  fh2WeightPion(0x0),
  fh2WeightKaon(0x0),
  fh2WeightProton(0x0),


  // event histograms
  fhEventSampling(0x0),
  fhEventCentrality(0x0),
  fh2EventCentralityNumSelCharged(0x0),
  fhEventCounter(0x0),

  // charged histogram
  fhRefsMult(0x0),
  fhRefsPt(0x0),
  fhRefsEta(0x0),
  fhRefsPhi(0x0),
  fhChargedCounter(0x0),

  // PID histogram
  fhPIDPionMult(0x0),
  fhPIDPionPt(0x0),
  fhPIDPionPhi(0x0),
  fhPIDPionEta(0x0),
  fhPIDPionCharge(0x0),
  fhPIDKaonMult(0x0),
  fhPIDKaonPt(0x0),
  fhPIDKaonPhi(0x0),
  fhPIDKaonEta(0x0),
  fhPIDKaonCharge(0x0),
  fhPIDProtonMult(0x0),
  fhPIDProtonPt(0x0),
  fhPIDProtonPhi(0x0),
  fhPIDProtonEta(0x0),
  fhPIDProtonCharge(0x0),
  fh2PIDPionTPCdEdx(0x0),
  fh2PIDPionTOFbeta(0x0),
  fh2PIDKaonTPCdEdx(0x0),
  fh2PIDKaonTOFbeta(0x0),
  fh2PIDProtonTPCdEdx(0x0),
  fh2PIDProtonTOFbeta(0x0),
  fh2PIDPionTPCnSigmaPion(0x0),
  fh2PIDPionTOFnSigmaPion(0x0),
  fh2PIDPionTPCnSigmaKaon(0x0),
  fh2PIDPionTOFnSigmaKaon(0x0),
  fh2PIDPionTPCnSigmaProton(0x0),
  fh2PIDPionTOFnSigmaProton(0x0),
  fh2PIDKaonTPCnSigmaPion(0x0),
  fh2PIDKaonTOFnSigmaPion(0x0),
  fh2PIDKaonTPCnSigmaKaon(0x0),
  fh2PIDKaonTOFnSigmaKaon(0x0),
  fh2PIDKaonTPCnSigmaProton(0x0),
  fh2PIDKaonTOFnSigmaProton(0x0),
  fh2PIDProtonTPCnSigmaPion(0x0),
  fh2PIDProtonTOFnSigmaPion(0x0),
  fh2PIDProtonTPCnSigmaKaon(0x0),
  fh2PIDProtonTOFnSigmaKaon(0x0),
  fh2PIDProtonTPCnSigmaProton(0x0),
  fh2PIDProtonTOFnSigmaProton(0x0)

{
    SetPriors(); //init arrays
    // New PID procedure (Bayesian Combined PID)
    // allocating here is necessary because we don't
    // stream this member
    // TODO: fix streaming problems AliFlowBayesianPID
    fBayesianResponse = new AliFlowBayesianPID();
    fBayesianResponse->SetNewTrackParam();
  // default constructor, don't allocate memory here!
  // this is used by root for IO purposes, it needs to remain empty
}
//_____________________________________________________________________________
AliAnalysisTaskFlowModes::AliAnalysisTaskFlowModes(const char* name) : AliAnalysisTaskSE(name),
  fEventAOD(0x0),
  fPIDResponse(0x0),
  fPIDCombined(0x0),
  fFlowWeightsFile(0x0),
  fInit(kFALSE),
  fIndexSampling(0),
  fIndexCentrality(-1),
  fEventCounter(0),
  fNumEventsAnalyse(50),
  fRunNumber(-1),
  fPDGMassPion(TDatabasePDG::Instance()->GetParticle(211)->Mass()),
  fPDGMassKaon(TDatabasePDG::Instance()->GetParticle(321)->Mass()),
  fPDGMassProton(TDatabasePDG::Instance()->GetParticle(2212)->Mass()),

  // FlowPart containers
  fVectorCharged(0x0),
  fVectorPion(0x0),
  fVectorKaon(0x0),
  fVectorProton(0x0),

  // analysis selection
  fRunMode(kFull),
  fAnalType(kAOD),
  fSampling(kFALSE),
  fFillQA(kTRUE),
  // fNumSamples(10),
  fProcessCharged(kFALSE),
  fProcessPID(kFALSE),
  fBayesianResponse(NULL),

  // flow related
  fCutFlowRFPsPtMin(0),
  fCutFlowRFPsPtMax(0),
  fFlowPOIsPtMin(0),
  fFlowPOIsPtMax(20.),
  fCutFlowDoFourCorrelations(kTRUE),
  fDoOnlyMixedCorrelations(kFALSE),
  fFlowFillWeights(kFALSE),
  fFlowCentMin(0),
  fFlowCentMax(150),
  fFlowCentNumBins(150),
  fFlowWeightsPath(),
  fFlowUseWeights(kFALSE),
  fPositivelyChargedRef(kFALSE),
  fNegativelyChargedRef(kFALSE),

  // events selection
  fPVtxCutZ(0.),
  fMultEstimator(),
  fTrigger(0),
  fFullCentralityRange(kTRUE),
  // charged tracks selection
  fCutChargedEtaMax(0),
  fCutChargedPtMax(0),
  fCutChargedPtMin(0),
  fCutChargedDCAzMax(0),
  fCutChargedDCAxyMax(0),
  fCutChargedTrackFilterBit(0),
  fCutChargedNumTPCclsMin(0),

  // PID tracks selection
  fESDpid(),
  fCutPIDUseAntiProtonOnly(kFALSE),
  fPID3sigma(kTRUE),
  fPIDbayesian(kFALSE),
  fCutPIDnSigmaPionMax(3),
  fCutPIDnSigmaKaonMax(3),
  fCutPIDnSigmaProtonMax(3),
  fCutPIDnSigmaTPCRejectElectron(3),
  fCutPIDnSigmaCombinedNoTOFrejection(kFALSE),
  fCurrCentr(0.0),
  fParticleProbability(0.9),


  // output lists
  fQAEvents(0x0),
  fQACharged(0x0),
  fQAPID(0x0),
  fFlowWeights(0x0),
  fFlowRefs(0x0),
  fFlowCharged(0x0),
  fFlowPID(0x0),

  // flow histograms & profiles
  fh3WeightsRefs(0x0),
  fh3WeightsCharged(0x0),
  fh3WeightsPion(0x0),
  fh3WeightsKaon(0x0),
  fh3WeightsProton(0x0),
  fh3AfterWeightsRefs(0x0),
  fh3AfterWeightsCharged(0x0),
  fh3AfterWeightsPion(0x0),
  fh3AfterWeightsKaon(0x0),
  fh3AfterWeightsProton(0x0),
  fh2WeightRefs(0x0),
  fh2WeightCharged(0x0),
  fh2WeightPion(0x0),
  fh2WeightKaon(0x0),
  fh2WeightProton(0x0),

  // event histograms
  fhEventSampling(0x0),
  fhEventCentrality(0x0),
  fh2EventCentralityNumSelCharged(0x0),
  fhEventCounter(0x0),

  // charged histogram
  fhRefsMult(0x0),
  fhRefsPt(0x0),
  fhRefsEta(0x0),
  fhRefsPhi(0x0),
  fhChargedCounter(0x0),

  // PID histogram
  fhPIDPionMult(0x0),
  fhPIDPionPt(0x0),
  fhPIDPionPhi(0x0),
  fhPIDPionEta(0x0),
  fhPIDPionCharge(0x0),
  fhPIDKaonMult(0x0),
  fhPIDKaonPt(0x0),
  fhPIDKaonPhi(0x0),
  fhPIDKaonEta(0x0),
  fhPIDKaonCharge(0x0),
  fhPIDProtonMult(0x0),
  fhPIDProtonPt(0x0),
  fhPIDProtonPhi(0x0),
  fhPIDProtonEta(0x0),
  fhPIDProtonCharge(0x0),
  fh2PIDPionTPCdEdx(0x0),
  fh2PIDPionTOFbeta(0x0),
  fh2PIDKaonTPCdEdx(0x0),
  fh2PIDKaonTOFbeta(0x0),
  fh2PIDProtonTPCdEdx(0x0),
  fh2PIDProtonTOFbeta(0x0),
  fh2PIDPionTPCnSigmaPion(0x0),
  fh2PIDPionTOFnSigmaPion(0x0),
  fh2PIDPionTPCnSigmaKaon(0x0),
  fh2PIDPionTOFnSigmaKaon(0x0),
  fh2PIDPionTPCnSigmaProton(0x0),
  fh2PIDPionTOFnSigmaProton(0x0),
  fh2PIDKaonTPCnSigmaPion(0x0),
  fh2PIDKaonTOFnSigmaPion(0x0),
  fh2PIDKaonTPCnSigmaKaon(0x0),
  fh2PIDKaonTOFnSigmaKaon(0x0),
  fh2PIDKaonTPCnSigmaProton(0x0),
  fh2PIDKaonTOFnSigmaProton(0x0),
  fh2PIDProtonTPCnSigmaPion(0x0),
  fh2PIDProtonTOFnSigmaPion(0x0),
  fh2PIDProtonTPCnSigmaKaon(0x0),
  fh2PIDProtonTOFnSigmaKaon(0x0),
  fh2PIDProtonTPCnSigmaProton(0x0),
  fh2PIDProtonTOFnSigmaProton(0x0)
{
    
  SetPriors(); //init arrays
  // New PID procedure (Bayesian Combined PID)
  fBayesianResponse = new AliFlowBayesianPID();
  fBayesianResponse->SetNewTrackParam();
  // Flow vectors
  for(Short_t iHarm(0); iHarm < fFlowNumHarmonicsMax; iHarm++)
  {
    for(Short_t iPower(0); iPower < fFlowNumWeightPowersMax; iPower++)
    {
      fFlowVecQpos[iHarm][iPower] = TComplex(0,0,kFALSE);
      fFlowVecQneg[iHarm][iPower] = TComplex(0,0,kFALSE);

      for(Short_t iPt(0); iPt < fFlowPOIsPtNumBins; iPt++)
      {
        fFlowVecPpos[iHarm][iPower][iPt] = TComplex(0,0,kFALSE);
        fFlowVecPneg[iHarm][iPower][iPt] = TComplex(0,0,kFALSE);
        fFlowVecS[iHarm][iPower][iPt] = TComplex(0,0,kFALSE);
      }//endfor(Short_t iPt(0); iPt < fFlowPOIsPtNumBins; iPt++)
    }//endfor(Short_t iPower(0); iPower < fFlowNumWeightPowersMax; iPower++)
  }//endfor(Short_t iHarm(0); iHarm < fFlowNumHarmonicsMax; iHarm++)

  // Flow profiles & histograms
  if(fDoOnlyMixedCorrelations){
      for(Short_t iMixedHarm(0); iMixedHarm < fNumMixedHarmonics; iMixedHarm++)
      {
        for(Short_t iGap(0); iGap < fNumEtaGap; iGap++)
        {
           for(Short_t iSample(0); iSample < fNumSamples; iSample++)
           {
              fpMixedRefsCor4[iSample][iGap][iMixedHarm] = 0x0;
              fpMixedChargedCor3Pos[fIndexSampling][iGap][iMixedHarm] = 0x0;
              fpMixedChargedCor3Neg[fIndexSampling][iGap][iMixedHarm] = 0x0;
               
              fpMixedPionCor3Pos[iSample][iGap][iMixedHarm] = 0x0;
              fpMixedPionCor3Neg[iSample][iGap][iMixedHarm] = 0x0;
              fpMixedKaonCor3Pos[iSample][iGap][iMixedHarm] = 0x0;
              fpMixedKaonCor3Neg[iSample][iGap][iMixedHarm] = 0x0;
              fpMixedProtonCor3Pos[iSample][iGap][iMixedHarm] = 0x0;
              fpMixedProtonCor3Neg[iSample][iGap][iMixedHarm] = 0x0;
               
           }//endfor(Short_t iSample(0); iSample < fNumSamples; iSample++)
        }//endfor(Short_t iGap(0); iGap < fNumEtaGap; iGap++)
      }//endfor(Short_t iMixedHarm(0); iMixedHarm < fNumMixedHarmonics; iMixedHarm++)
  }//endif(fDoOnlyMixedCorrelations)

    if(!fDoOnlyMixedCorrelations){
      for(Short_t iHarm(0); iHarm < fNumHarmonics; iHarm++){
        for(Short_t iSample(0); iSample < fNumSamples; iSample++){
          fpRefsCor4[iSample][iHarm] = 0x0;      
          fp2ChargedCor4[iSample][iHarm] = 0x0;
          fp2PionCor4[iSample][iHarm] = 0x0;
          fp2KaonCor4[iSample][iHarm] = 0x0;
          fp2ProtonCor4[iSample][iHarm] = 0x0;
        }//endfor(Short_t iSample(0); iSample < fNumSamples; iSample++)

       
        for(Short_t iGap(0); iGap < fNumEtaGap; iGap++){

          // mean Qx,Qy
          fpMeanQxRefsPos[iGap][iHarm] = 0x0;
          fpMeanQxRefsNeg[iGap][iHarm] = 0x0;
          fpMeanQyRefsPos[iGap][iHarm] = 0x0;
          fpMeanQyRefsNeg[iGap][iHarm] = 0x0;

          for(Short_t iSample(0); iSample < fNumSamples; iSample++){
            fpRefsCor2[iSample][iGap][iHarm] = 0x0;
            fp2ChargedCor2Pos[iSample][iGap][iHarm] = 0x0;
            fp2ChargedCor2Neg[iSample][iGap][iHarm] = 0x0;
            fp2PionCor2Pos[iSample][iGap][iHarm] = 0x0;
            fp2PionCor2Neg[iSample][iGap][iHarm] = 0x0;
            fp2KaonCor2Pos[iSample][iGap][iHarm] = 0x0;
            fp2KaonCor2Neg[iSample][iGap][iHarm] = 0x0;
            fp2ProtonCor2Pos[iSample][iGap][iHarm] = 0x0;
            fp2ProtonCor2Neg[iSample][iGap][iHarm] = 0x0;
          }//endfor(Short_t iSample(0); iSample < fNumSamples; iSample++)

        }//endfor(Short_t iGap(0); iGap < fNumEtaGap; iGap++)
      }//endfor(Short_t iHarm(0); iHarm < fNumHarmonics; iHarm++)
    }//endif(!fDoOnlyMixedCorrelations)

  // QA histograms
  for(Short_t iQA(0); iQA < fiNumIndexQA; iQA++)
  {
    // Event histograms
    fhQAEventsPVz[iQA] = 0x0;
    fhQAEventsNumContrPV[iQA] = 0x0;
    fhQAEventsNumSPDContrPV[iQA] = 0x0;
    fhQAEventsDistPVSPD[iQA] = 0x0;
    fhQAEventsSPDresol[iQA] = 0x0;
    fhQAEventsPileUp[iQA] = 0x0;
    fhQAEventsCentralityOutliers[iQA] = 0x0;

    // charged
    fhQAChargedMult[iQA] = 0x0;
    fhQAChargedPt[iQA] = 0x0;
    fhQAChargedEta[iQA] = 0x0;
    fhQAChargedPhi[iQA] = 0x0;
    fhQAChargedCharge[iQA] = 0x0;
    fhQAChargedFilterBit[iQA] = 0x0;
    fhQAChargedNumTPCcls[iQA] = 0x0;
    fhQAChargedDCAxy[iQA] = 0x0;
    fhQAChargedDCAz[iQA] = 0x0;

    // PID
    fhQAPIDTPCstatus[iQA] = 0x0;
    fhQAPIDTOFstatus[iQA] = 0x0;
    fhQAPIDTPCdEdx[iQA] = 0x0;
    fhQAPIDTOFbeta[iQA] = 0x0;
      
    fh3PIDPionTPCTOFnSigmaPion[iQA] = 0x0;
    fh3PIDPionTPCTOFnSigmaKaon[iQA] = 0x0;
    fh3PIDPionTPCTOFnSigmaProton[iQA] = 0x0;

    fh3PIDKaonTPCTOFnSigmaPion[iQA] = 0x0;
    fh3PIDKaonTPCTOFnSigmaKaon[iQA] = 0x0;
    fh3PIDKaonTPCTOFnSigmaProton[iQA] = 0x0;
      
    fh3PIDProtonTPCTOFnSigmaPion[iQA] = 0x0;
    fh3PIDProtonTPCTOFnSigmaKaon[iQA] = 0x0;
    fh3PIDProtonTPCTOFnSigmaProton[iQA] = 0x0;


  }//endfor(Short_t iQA(0); iQA < fiNumIndexQA; iQA++)

  // defining input/output
  DefineInput(0, TChain::Class());
  DefineOutput(1, TList::Class());
  DefineOutput(2, TList::Class());
  DefineOutput(3, TList::Class());
  DefineOutput(4, TList::Class());
  DefineOutput(5, TList::Class());
  DefineOutput(6, TList::Class());
  DefineOutput(7, TList::Class());
}
//_____________________________________________________________________________
AliAnalysisTaskFlowModes::~AliAnalysisTaskFlowModes()
{
  // destructor
  // if(fPIDCombined)
  // {
  //   delete fPIDCombined;
  // }

  // deleting FlowPart vectors (containers)
  if(fVectorCharged) delete fVectorCharged;
  if(fVectorPion) delete fVectorPion;
  if(fVectorKaon) delete fVectorKaon;
  if(fVectorProton) delete fVectorProton;

  // deleting output lists
  if(fFlowWeights) delete fFlowWeights;
  if(fFlowRefs) delete fFlowRefs;
  if(fFlowCharged) delete fFlowCharged;
  if(fFlowPID) delete fFlowPID;

  if(fQAEvents) delete fQAEvents;
  if(fQACharged) delete fQACharged;
  if(fQAPID) delete fQAPID;
}
//_____________________________________________________________________________
void AliAnalysisTaskFlowModes::UserCreateOutputObjects()
{
  // create output objects
  // this function is called ONCE at the start of your analysis (RUNTIME)
  // *************************************************************

  // list all parameters used in this analysis
  ListParameters();

  // task initialization
  fInit = InitializeTask();
  if(!fInit) return;

  // creating output lists
  fFlowRefs = new TList();
  fFlowRefs->SetOwner(kTRUE);
  fFlowRefs->SetName("fFlowRefs");
  fFlowCharged = new TList();
  fFlowCharged->SetOwner(kTRUE);
  fFlowCharged->SetName("fFlowCharged");
  fFlowPID = new TList();
  fFlowPID->SetOwner(kTRUE);
  fFlowPID->SetName("fFlowPID");
  fFlowWeights = new TList();
  fFlowWeights->SetOwner(kTRUE);
  fFlowWeights->SetName("fFlowWeights");

  fQAEvents = new TList();
  fQAEvents->SetOwner(kTRUE);
  fQACharged = new TList();
  fQACharged->SetOwner(kTRUE);
  fQAPID = new TList();
  fQAPID->SetOwner(kTRUE);

  // creating particle vectors
  fVectorCharged = new std::vector<FlowPart>;
  fVectorPion = new std::vector<FlowPart>;
  fVectorKaon = new std::vector<FlowPart>;
  fVectorProton = new std::vector<FlowPart>;

  fVectorCharged->reserve(300);
  if(fProcessPID) { fVectorPion->reserve(200); fVectorKaon->reserve(100); fVectorProton->reserve(100); }

  // creating histograms
    // event histogram
    
    fhEventSampling = new TH2D("fhEventSampling","Event sampling; centrality/multiplicity; sample index", fFlowCentNumBins,0,fFlowCentNumBins, fNumSamples,0,fNumSamples);
    fQAEvents->Add(fhEventSampling);
    fhEventCentrality = new TH1D("fhEventCentrality",Form("Event centrality (%s); centrality/multiplicity",fMultEstimator.Data()), fFlowCentNumBins,0,fFlowCentNumBins);
    fQAEvents->Add(fhEventCentrality);
    fh2EventCentralityNumSelCharged = new TH2D("fh2EventCentralityNumSelCharged",Form("Event centrality (%s) vs. N^{sel}_{ch}; N^{sel}_{ch}; centrality/multiplicity",fMultEstimator.Data()), 150,0,150, fFlowCentNumBins,0,fFlowCentNumBins);
    fQAEvents->Add(fh2EventCentralityNumSelCharged);

    const Short_t iEventCounterBins = 10;
    TString sEventCounterLabel[iEventCounterBins] = {"Input","Physics selection OK","Centr. Est. Consis. OK","PV OK","SPD Vtx OK","Pileup MV OK","Vtx Consis. OK","PV #it{z} OK","ESD TPC Mult. Diff. OK","Selected"};
    fhEventCounter = new TH1D("fhEventCounter","Event Counter",iEventCounterBins,0,iEventCounterBins);
    for(Short_t i(0); i < iEventCounterBins; i++) fhEventCounter->GetXaxis()->SetBinLabel(i+1, sEventCounterLabel[i].Data() );
    fQAEvents->Add(fhEventCounter);
  
    // flow histograms & profiles
    // weights
    if(fFlowFillWeights || fRunMode == kFillWeights)
    {
      // for(Short_t iGap(0); iGap < fNumEtaGap; iGap++)
      //   for(Short_t iHarm(0); iHarm < fNumHarmonics; iHarm++)
      //   {
      //     fpMeanQxRefsPos[iGap][iHarm] = new TProfile(Form("fpMeanQxRefs_harm%d_gap%02.2g_Pos",fHarmonics[iHarm],10*fEtaGap[iGap]),Form("<<Qx>>: Refs | Gap %g | n=%d | POS; multiplicity/centrality; <Qx>",fEtaGap[iGap],fHarmonics[iHarm]), fFlowCentNumBins,0,fFlowCentNumBins);
      //     fpMeanQxRefsPos[iGap][iHarm]->Sumw2();
      //     fFlowWeights->Add(fpMeanQxRefsPos[iGap][iHarm]);
      //
      //     fpMeanQxRefsNeg[iGap][iHarm] = new TProfile(Form("fpMeanQxRefs_harm%d_gap%02.2g_Neg",fHarmonics[iHarm],10*fEtaGap[iGap]),Form("<<Qx>>: Refs | Gap %g | n=%d | NEG; multiplicity/centrality; <Qx>",fEtaGap[iGap],fHarmonics[iHarm]), fFlowCentNumBins,0,fFlowCentNumBins);
      //     fpMeanQxRefsNeg[iGap][iHarm]->Sumw2();
      //     fFlowWeights->Add(fpMeanQxRefsNeg[iGap][iHarm]);
      //
      //     fpMeanQyRefsPos[iGap][iHarm] = new TProfile(Form("fpMeanQyRefs_harm%d_gap%02.2g_Pos",fHarmonics[iHarm],10*fEtaGap[iGap]),Form("<<Qy>>: Refs | Gap %g | n=%d | POS; multiplicity/centrality; <Qy>",fEtaGap[iGap],fHarmonics[iHarm]), fFlowCentNumBins,0,fFlowCentNumBins);
      //     fpMeanQyRefsPos[iGap][iHarm]->Sumw2();
      //     fFlowWeights->Add(fpMeanQyRefsPos[iGap][iHarm]);
      //
      //     fpMeanQyRefsNeg[iGap][iHarm] = new TProfile(Form("fpMeanQyRefs_harm%d_gap%02.2g_Neg",fHarmonics[iHarm],10*fEtaGap[iGap]),Form("<<Qy>>: Refs | Gap %g | n=%d | NEG; multiplicity/centrality; <Qy>",fEtaGap[iGap],fHarmonics[iHarm]), fFlowCentNumBins,0,fFlowCentNumBins);
      //     fpMeanQyRefsNeg[iGap][iHarm]->Sumw2();
      //     fFlowWeights->Add(fpMeanQyRefsNeg[iGap][iHarm]);
      //   }

      fh3WeightsRefs = new TH3D("fh3WeightsRefs","Weights: Refs; #varphi; #eta; #it{p}_{T} (GeV/#it{c})", 100,0,TMath::TwoPi(), 151,-1.5,1.5, fFlowPOIsPtNumBins,fFlowPOIsPtMin,fFlowPOIsPtMax);
      fh3WeightsRefs->Sumw2();
      fFlowWeights->Add(fh3WeightsRefs);
      fh3WeightsCharged = new TH3D("fh3WeightsCharged","Weights: Charged; #varphi; #eta; #it{p}_{T} (GeV/#it{c})", 100,0,TMath::TwoPi(), 151,-1.5,1.5, fFlowPOIsPtNumBins,fFlowPOIsPtMin,fFlowPOIsPtMax);
      fh3WeightsCharged->Sumw2();
      fFlowWeights->Add(fh3WeightsCharged);
      fh3WeightsPion = new TH3D("fh3WeightsPion","Weights: #pi; #varphi; #eta; #it{p}_{T} (GeV/#it{c})", 100,0,TMath::TwoPi(), 151,-1.5,1.5, fFlowPOIsPtNumBins,fFlowPOIsPtMin,fFlowPOIsPtMax);
      fh3WeightsPion->Sumw2();
      fFlowWeights->Add(fh3WeightsPion);
      fh3WeightsKaon = new TH3D("fh3WeightsKaon","Weights: K; #varphi; #eta; #it{p}_{T} (GeV/#it{c})", 100,0,TMath::TwoPi(), 151,-1.5,1.5, fFlowPOIsPtNumBins,fFlowPOIsPtMin,fFlowPOIsPtMax);
      fh3WeightsKaon->Sumw2();
      fFlowWeights->Add(fh3WeightsKaon);
      fh3WeightsProton = new TH3D("fh3WeightsProton","Weights: p; #varphi; #eta; #it{p}_{T} (GeV/#it{c})", 100,0,TMath::TwoPi(), 151,-1.5,1.5, fFlowPOIsPtNumBins,fFlowPOIsPtMin,fFlowPOIsPtMax);
      fh3WeightsProton->Sumw2();
      fFlowWeights->Add(fh3WeightsProton);
    }//if(fFlowFillWeights || fRunMode == kFillWeights)

    if(fFlowUseWeights)
    {
      fh3AfterWeightsRefs = new TH3D("fh3AfterWeightsRefs","Weights: Refs; #varphi (After); #eta; #it{p}_{T} (GeV/#it{c})", 100,0,TMath::TwoPi(), 151,-1.5,1.5, fFlowPOIsPtNumBins,fFlowPOIsPtMin,fFlowPOIsPtMax);
      fh3AfterWeightsRefs->Sumw2();
      fFlowWeights->Add(fh3AfterWeightsRefs);
      fh3AfterWeightsCharged = new TH3D("fh3AfterWeightsCharged","Weights: Charged (After); #varphi; #eta; #it{p}_{T} (GeV/#it{c})", 100,0,TMath::TwoPi(), 151,-1.5,1.5, fFlowPOIsPtNumBins,fFlowPOIsPtMin,fFlowPOIsPtMax);
      fh3AfterWeightsCharged->Sumw2();
      fFlowWeights->Add(fh3AfterWeightsCharged);
      fh3AfterWeightsPion = new TH3D("fh3AfterWeightsPion","Weights: #pi (After); #varphi; #eta; #it{p}_{T} (GeV/#it{c})", 100,0,TMath::TwoPi(), 151,-1.5,1.5, fFlowPOIsPtNumBins,fFlowPOIsPtMin,fFlowPOIsPtMax);
      fh3AfterWeightsPion->Sumw2();
      fFlowWeights->Add(fh3AfterWeightsPion);
      fh3AfterWeightsKaon = new TH3D("fh3AfterWeightsKaon","Weights: K (After); #varphi; #eta; #it{p}_{T} (GeV/#it{c})", 100,0,TMath::TwoPi(), 151,-1.5,1.5, fFlowPOIsPtNumBins,fFlowPOIsPtMin,fFlowPOIsPtMax);
      fh3AfterWeightsKaon->Sumw2();
      fFlowWeights->Add(fh3AfterWeightsKaon);
      fh3AfterWeightsProton = new TH3D("fh3AfterWeightsProton","Weights: p (After); #varphi; #eta; #it{p}_{T} (GeV/#it{c})", 100,0,TMath::TwoPi(), 151,-1.5,1.5, fFlowPOIsPtNumBins,fFlowPOIsPtMin,fFlowPOIsPtMax);
      fh3AfterWeightsProton->Sumw2();
      fFlowWeights->Add(fh3AfterWeightsProton);
    }//if(fFlowUseWeights)

    //mixed correlations 
    //
    if(fDoOnlyMixedCorrelations){
        for(Short_t iMixedHarm(0); iMixedHarm < fNumMixedHarmonics; iMixedHarm++)
        {
          for(Short_t iGap(0); iGap < fNumEtaGap; iGap++)//For now only for nonoverlapping subevents...
          {
            for(Short_t iSample(0); iSample < fNumSamples; iSample++)
            {   //reference flow for mixed harmonics 422,633,523
                fpMixedRefsCor4[iSample][iGap][iMixedHarm] = new TProfile(Form("fpRefs_<4>_MixedHarm%d_gap%02.2g_sample%d",fMixedHarmonics[iMixedHarm],10*fEtaGap[iGap],iSample),Form("Ref: <<4>> | Gap %g | v%d | sample %d ; centrality/multiplicity;",fEtaGap[iGap],fMixedHarmonics[iMixedHarm],iSample), fFlowCentNumBins,fFlowCentMin,fFlowCentMax);
                fpMixedRefsCor4[iSample][iGap][iMixedHarm]->Sumw2(kTRUE);
                fFlowRefs->Add(fpMixedRefsCor4[iSample][iGap][iMixedHarm]);

                if(fProcessCharged){
                    fpMixedChargedCor3Pos[iSample][iGap][iMixedHarm] = new TProfile2D(Form("fp2Charged_<3>_MixedHarm%d_gap%02.2g_Pos_sample%d",fMixedHarmonics[iMixedHarm],10*fEtaGap[iGap],iSample),Form("Charged: <<3'>> | Gap %g | v%d | sample %d | POIs pos; centrality/multiplicity; #it{p}_{T} (GeV/c)",fEtaGap[iGap],fMixedHarmonics[iMixedHarm],iSample), fFlowCentNumBins,fFlowCentMin,fFlowCentMax, fFlowPOIsPtNumBins,fFlowPOIsPtMin,fFlowPOIsPtMax);
                    fpMixedChargedCor3Pos[iSample][iGap][iMixedHarm]->Sumw2(kTRUE);
                    fFlowCharged->Add(fpMixedChargedCor3Pos[iSample][iGap][iMixedHarm]);

                    fpMixedChargedCor3Neg[iSample][iGap][iMixedHarm] = new TProfile2D(Form("fp2Charged_<3>_MixedHarm%d_gap%02.2g_Neg_sample%d",fMixedHarmonics[iMixedHarm],10*fEtaGap[iGap],iSample),Form("Charged: <<3'>> | Gap %g | v%d | sample %d | POIs neg; centrality/multiplicity; #it{p}_{T} (GeV/c)",fEtaGap[iGap],fMixedHarmonics[iMixedHarm],iSample), fFlowCentNumBins,fFlowCentMin,fFlowCentMax, fFlowPOIsPtNumBins,fFlowPOIsPtMin,fFlowPOIsPtMax);
                    fpMixedChargedCor3Neg[iSample][iGap][iMixedHarm]->Sumw2(kTRUE);
                    fFlowCharged->Add(fpMixedChargedCor3Neg[iSample][iGap][iMixedHarm]);  
                }//if(fProcessCharged)
                if(fProcessPID){
                    fpMixedPionCor3Pos[iSample][iGap][iMixedHarm] = new TProfile2D(Form("fp2Pion_<3>_MixedHarm%d_gap%02.2g_Pos_sample%d",fMixedHarmonics[iMixedHarm],10*fEtaGap[iGap],iSample),Form("Pion: <<3'>> | Gap %g | v%d | sample %d | POIs pos; centrality/multiplicity; #it{p}_{T} (GeV/c)",fEtaGap[iGap],fMixedHarmonics[iMixedHarm],iSample), fFlowCentNumBins,fFlowCentMin,fFlowCentMax, fFlowPOIsPtNumBins,fFlowPOIsPtMin,fFlowPOIsPtMax);
                    fpMixedPionCor3Pos[iSample][iGap][iMixedHarm]->Sumw2(kTRUE);
                    fFlowPID->Add(fpMixedPionCor3Pos[iSample][iGap][iMixedHarm]);

                    fpMixedPionCor3Neg[iSample][iGap][iMixedHarm] = new TProfile2D(Form("fp2Pion_<3>_MixedHarm%d_gap%02.2g_Neg_sample%d",fMixedHarmonics[iMixedHarm],10*fEtaGap[iGap],iSample),Form("Pion: <<3'>> | Gap %g | v%d | sample %d | POIs neg; centrality/multiplicity; #it{p}_{T} (GeV/c)",fEtaGap[iGap],fMixedHarmonics[iMixedHarm],iSample), fFlowCentNumBins,fFlowCentMin,fFlowCentMax, fFlowPOIsPtNumBins,fFlowPOIsPtMin,fFlowPOIsPtMax);
                    fpMixedPionCor3Neg[iSample][iGap][iMixedHarm]->Sumw2(kTRUE);
                    fFlowPID->Add(fpMixedPionCor3Neg[iSample][iGap][iMixedHarm]);

                    fpMixedKaonCor3Pos[iSample][iGap][iMixedHarm] = new TProfile2D(Form("fp2Kaon_<3>_MixedHarm%d_gap%02.2g_Pos_sample%d",fMixedHarmonics[iMixedHarm],10*fEtaGap[iGap],iSample),Form("Pion: <<3'>> | Gap %g | v%d | sample %d | POIs pos; centrality/multiplicity; #it{p}_{T} (GeV/c)",fEtaGap[iGap],fMixedHarmonics[iMixedHarm],iSample), fFlowCentNumBins,fFlowCentMin,fFlowCentMax, fFlowPOIsPtNumBins,fFlowPOIsPtMin,fFlowPOIsPtMax);
                    fpMixedKaonCor3Pos[iSample][iGap][iMixedHarm]->Sumw2(kTRUE);
                    fFlowPID->Add(fpMixedKaonCor3Pos[iSample][iGap][iMixedHarm]);

                    fpMixedKaonCor3Neg[iSample][iGap][iMixedHarm] = new TProfile2D(Form("fp2Kaon_<3>_MixedHarm%d_gap%02.2g_Neg_sample%d",fMixedHarmonics[iMixedHarm],10*fEtaGap[iGap],iSample),Form("Kaon: <<3'>> | Gap %g | v%d | sample %d | POIs neg; centrality/multiplicity; #it{p}_{T} (GeV/c)",fEtaGap[iGap],fMixedHarmonics[iMixedHarm],iSample), fFlowCentNumBins,fFlowCentMin,fFlowCentMax, fFlowPOIsPtNumBins,fFlowPOIsPtMin,fFlowPOIsPtMax);
                    fpMixedKaonCor3Neg[iSample][iGap][iMixedHarm]->Sumw2(kTRUE);
                    fFlowPID->Add(fpMixedKaonCor3Neg[iSample][iGap][iMixedHarm]);

                    fpMixedProtonCor3Pos[iSample][iGap][iMixedHarm] = new TProfile2D(Form("fp2Proton_<3>_MixedHarm%d_gap%02.2g_Pos_sample%d",fMixedHarmonics[iMixedHarm],10*fEtaGap[iGap],iSample),Form("Proton: <<3'>> | Gap %g | v%d | sample %d | POIs pos; centrality/multiplicity; #it{p}_{T} (GeV/c)",fEtaGap[iGap],fMixedHarmonics[iMixedHarm],iSample), fFlowCentNumBins,fFlowCentMin,fFlowCentMax, fFlowPOIsPtNumBins,fFlowPOIsPtMin,fFlowPOIsPtMax);
                    fpMixedProtonCor3Pos[iSample][iGap][iMixedHarm]->Sumw2(kTRUE);
                    fFlowPID->Add(fpMixedProtonCor3Pos[iSample][iGap][iMixedHarm]);

                    fpMixedProtonCor3Neg[iSample][iGap][iMixedHarm] = new TProfile2D(Form("fp2Proton_<3>_MixedHarm%d_gap%02.2g_Neg_sample%d",fMixedHarmonics[iMixedHarm],10*fEtaGap[iGap],iSample),Form("Proton: <<3'>> | Gap %g | v%d | sample %d | POIs neg; centrality/multiplicity; #it{p}_{T} (GeV/c)",fEtaGap[iGap],fMixedHarmonics[iMixedHarm],iSample), fFlowCentNumBins,fFlowCentMin,fFlowCentMax, fFlowPOIsPtNumBins,fFlowPOIsPtMin,fFlowPOIsPtMax);
                    fpMixedProtonCor3Neg[iSample][iGap][iMixedHarm]->Sumw2(kTRUE);
                    fFlowPID->Add(fpMixedProtonCor3Neg[iSample][iGap][iMixedHarm]);
                }//if(fProcessPID)
            }//for(Short_t iSample(0); iSample < fNumSamples; iSample++)
          }//for(Short_t iGap(0); iGap < fNumEtaGap; iGap++)
        }//for(Short_t iMixedHarm(0); iMixedHarm < fNumMixedHarmonics; iMixedHarm++)
    }//if(fDoOnlyMixedCorrelations)

    if(!fDoOnlyMixedCorrelations){
        // correlations
        for(Short_t iHarm(0); iHarm < fNumHarmonics; iHarm++)
        {
          for(Short_t iGap(0); iGap < fNumEtaGap; iGap++)
          {
            for(Short_t iSample(0); iSample < fNumSamples; iSample++)
            {
              fpRefsCor2[iSample][iGap][iHarm] = new TProfile(Form("fpRefs_<2>_harm%d_gap%02.2g_sample%d",fHarmonics[iHarm],10*fEtaGap[iGap],iSample),Form("Ref: <<2>> | Gap %g | n=%d | sample %d ; centrality/multiplicity;",fEtaGap[iGap],fHarmonics[iHarm],iSample), fFlowCentNumBins,fFlowCentMin,fFlowCentMax);
              fpRefsCor2[iSample][iGap][iHarm]->Sumw2(kTRUE);
              fFlowRefs->Add(fpRefsCor2[iSample][iGap][iHarm]);

              if(fProcessCharged)
              {
                fp2ChargedCor2Pos[iSample][iGap][iHarm] = new TProfile2D(Form("fp2Charged_<2>_harm%d_gap%02.2g_Pos_sample%d",fHarmonics[iHarm],10*fEtaGap[iGap],iSample),Form("Charged: <<2'>> | Gap %g | n=%d | sample %d | POIs pos; centrality/multiplicity; #it{p}_{T} (GeV/c)",fEtaGap[iGap],fHarmonics[iHarm],iSample), fFlowCentNumBins,fFlowCentMin,fFlowCentMax, fFlowPOIsPtNumBins,fFlowPOIsPtMin,fFlowPOIsPtMax);
                fp2ChargedCor2Pos[iSample][iGap][iHarm]->Sumw2(kTRUE);
                fFlowCharged->Add(fp2ChargedCor2Pos[iSample][iGap][iHarm]);


                fp2ChargedCor2Neg[iSample][iGap][iHarm] = new TProfile2D(Form("fp2Charged_<2>_harm%d_gap%02.2g_Neg_sample%d",fHarmonics[iHarm],10*fEtaGap[iGap],iSample),Form("Charged: <<2'>> | Gap %g | n=%d | sample %d | POIs neg; centrality/multiplicity; #it{p}_{T} (GeV/c)",fEtaGap[iGap],fHarmonics[iHarm],iSample), fFlowCentNumBins,fFlowCentMin,fFlowCentMax, fFlowPOIsPtNumBins,fFlowPOIsPtMin,fFlowPOIsPtMax);
                fp2ChargedCor2Neg[iSample][iGap][iHarm]->Sumw2(kTRUE);
                fFlowCharged->Add(fp2ChargedCor2Neg[iSample][iGap][iHarm]);
                
              }//if(fProcessCharged)

              if(fProcessPID)
              {
                fp2PionCor2Pos[iSample][iGap][iHarm] = new TProfile2D(Form("fp2Pion_<2>_harm%d_gap%02.2g_Pos_sample%d",fHarmonics[iHarm],10*fEtaGap[iGap],iSample),Form("PID #pi: <<2'>> | Gap %g | n=%d | sample %d  | POIs pos; centrality/multiplicity; #it{p}_{T} (GeV/c)",fEtaGap[iGap],fHarmonics[iHarm],iSample), fFlowCentNumBins,fFlowCentMin,fFlowCentMax, fFlowPOIsPtNumBins,fFlowPOIsPtMin,fFlowPOIsPtMax);
                fp2PionCor2Pos[iSample][iGap][iHarm]->Sumw2(kTRUE);
                fFlowPID->Add(fp2PionCor2Pos[iSample][iGap][iHarm]);

                fp2KaonCor2Pos[iSample][iGap][iHarm] = new TProfile2D(Form("fp2Kaon_<2>_harm%d_gap%02.2g_Pos_sample%d",fHarmonics[iHarm],10*fEtaGap[iGap],iSample),Form("PID K: <<2'>> | Gap %g | n=%d | sample %d | POIs pos; centrality/multiplicity; #it{p}_{T} (GeV/c)",fEtaGap[iGap],fHarmonics[iHarm],iSample), fFlowCentNumBins,fFlowCentMin,fFlowCentMax, fFlowPOIsPtNumBins,fFlowPOIsPtMin,fFlowPOIsPtMax);
                fp2KaonCor2Pos[iSample][iGap][iHarm]->Sumw2(kTRUE);
                fFlowPID->Add(fp2KaonCor2Pos[iSample][iGap][iHarm]);

                fp2ProtonCor2Pos[iSample][iGap][iHarm] = new TProfile2D(Form("fp2Proton_<2>_harm%d_gap%02.2g_Pos_sample%d",fHarmonics[iHarm],10*fEtaGap[iGap],iSample),Form("PID p: <<2'>> | Gap %g | n=%d | sample %d | POIs pos; centrality/multiplicity; #it{p}_{T} (GeV/c)",fEtaGap[iGap],fHarmonics[iHarm],iSample), fFlowCentNumBins,fFlowCentMin,fFlowCentMax, fFlowPOIsPtNumBins,fFlowPOIsPtMin,fFlowPOIsPtMax);
                fp2ProtonCor2Pos[iSample][iGap][iHarm]->Sumw2(kTRUE);
                fFlowPID->Add(fp2ProtonCor2Pos[iSample][iGap][iHarm]);

                fp2PionCor2Neg[iSample][iGap][iHarm] = new TProfile2D(Form("fp2Pion_<2>_harm%d_gap%02.2g_Neg_sample%d",fHarmonics[iHarm],10*fEtaGap[iGap],iSample),Form("PID #pi: <<2'>> | Gap %g | n=%d | sample %d | POIs neg; centrality/multiplicity; #it{p}_{T} (GeV/c)",fEtaGap[iGap],fHarmonics[iHarm],iSample), fFlowCentNumBins,fFlowCentMin,fFlowCentMax, fFlowPOIsPtNumBins,fFlowPOIsPtMin,fFlowPOIsPtMax);
                fp2PionCor2Neg[iSample][iGap][iHarm]->Sumw2(kTRUE);
                fFlowPID->Add(fp2PionCor2Neg[iSample][iGap][iHarm]);

                fp2KaonCor2Neg[iSample][iGap][iHarm] = new TProfile2D(Form("fp2Kaon_<2>_harm%d_gap%02.2g_Neg_sample%d",fHarmonics[iHarm],10*fEtaGap[iGap],iSample),Form("PID K: <<2'>> | Gap %g | n=%d | sample %d | POIs neg; centrality/multiplicity; #it{p}_{T} (GeV/c)",fEtaGap[iGap],fHarmonics[iHarm],iSample), fFlowCentNumBins,fFlowCentMin,fFlowCentMax, fFlowPOIsPtNumBins,fFlowPOIsPtMin,fFlowPOIsPtMax);
                fp2KaonCor2Neg[iSample][iGap][iHarm]->Sumw2(kTRUE);
                fFlowPID->Add(fp2KaonCor2Neg[iSample][iGap][iHarm]);

                fp2ProtonCor2Neg[iSample][iGap][iHarm] = new TProfile2D(Form("fp2Proton_<2>_harm%d_gap%02.2g_Neg_sample%d",fHarmonics[iHarm],10*fEtaGap[iGap],iSample),Form("PID p: <<2'>> | Gap %g | n=%d | sample %d | POIs neg; centrality/multiplicity; #it{p}_{T} (GeV/c)",fEtaGap[iGap],fHarmonics[iHarm],iSample), fFlowCentNumBins,fFlowCentMin,fFlowCentMax, fFlowPOIsPtNumBins,fFlowPOIsPtMin,fFlowPOIsPtMax);
                fp2ProtonCor2Neg[iSample][iGap][iHarm]->Sumw2(kTRUE);
                fFlowPID->Add(fp2ProtonCor2Neg[iSample][iGap][iHarm]);
            }//endif(fProcessPID)
          }//for(Short_t iSample(0); iSample < fNumSamples; iSample++)
        }//for(Short_t iGap(0); iGap < fNumEtaGap; iGap++)
      }//for(Short_t iHarm(0); iHarm < fNumHarmonics; iHarm++)
    }//endif(!fDoOnlyMixedCorrelations)

    // charged (tracks) histograms
    fhRefsMult = new TH1D("fhRefsMult","RFPs: Multiplicity; multiplicity", 1000,0,1000);
    fQACharged->Add(fhRefsMult);
    fhRefsPt = new TH1D("fhRefsPt","RFPs: #it{p}_{T};  #it{p}_{T} (GeV/#it{c})", 300,0,30);
    fQACharged->Add(fhRefsPt);
    fhRefsEta = new TH1D("fhRefsEta","RFPs: #eta; #eta", 151,-1.5,1.5);
    fQACharged->Add(fhRefsEta);
    fhRefsPhi = new TH1D("fhRefsPhi","RFPs: #varphi; #varphi", 100,0,TMath::TwoPi());
    fQACharged->Add(fhRefsPhi);

    if(fProcessCharged)
    {
      TString sChargedCounterLabel[] = {"Input","FB","#TPC-Cls","DCA-z","DCA-xy","Eta","Selected"};
      const Short_t iNBinsChargedCounter = sizeof(sChargedCounterLabel)/sizeof(sChargedCounterLabel[0]);
      fhChargedCounter = new TH1D("fhChargedCounter","Charged tracks: Counter",iNBinsChargedCounter,0,iNBinsChargedCounter);
      for(Short_t i(0); i < iNBinsChargedCounter; i++) fhChargedCounter->GetXaxis()->SetBinLabel(i+1, sChargedCounterLabel[i].Data() );
      fQACharged->Add(fhChargedCounter);
    } // endif {fProcessCharged}

    // PID tracks histograms
    if(fProcessPID)
    {
      fhPIDPionMult = new TH1D("fhPIDPionMult","PID: #pi: Multiplicity; multiplicity", 200,0,200);
      fQAPID->Add(fhPIDPionMult);
      fhPIDKaonMult = new TH1D("fhPIDKaonMult","PID: K: Multiplicity; multiplicity", 100,0,100);
      fQAPID->Add(fhPIDKaonMult);
      fhPIDProtonMult = new TH1D("fhPIDProtonMult","PID: p: Multiplicity; multiplicity", 100,0,100);
      fQAPID->Add(fhPIDProtonMult);

      if(fFillQA)
      {
        fhPIDPionPt = new TH1D("fhPIDPionPt","PID: #pi: #it{p}_{T}; #it{p}_{T}", 150,0.,30.);
        fQAPID->Add(fhPIDPionPt);
        fhPIDPionPhi = new TH1D("fhPIDPionPhi","PID: #pi: #varphi; #varphi", 100,0,TMath::TwoPi());
        fQAPID->Add(fhPIDPionPhi);
        fhPIDPionEta = new TH1D("fhPIDPionEta","PID: #pi: #eta; #eta", 151,-1.5,1.5);
        fQAPID->Add(fhPIDPionEta);
        fhPIDPionCharge = new TH1D("fhPIDPionCharge","PID: #pi: charge; charge", 3,-1.5,1.5);
        fQAPID->Add(fhPIDPionCharge);
        fhPIDKaonPt = new TH1D("fhPIDKaonPt","PID: K: #it{p}_{T}; #it{p}_{T}", 150,0.,30.);
        fQAPID->Add(fhPIDKaonPt);
        fhPIDKaonPhi = new TH1D("fhPIDKaonPhi","PID: K: #varphi; #varphi", 100,0,TMath::TwoPi());
        fQAPID->Add(fhPIDKaonPhi);
        fhPIDKaonEta = new TH1D("fhPIDKaonEta","PID: K: #eta; #eta", 151,-1.5,1.5);
        fQAPID->Add(fhPIDKaonEta);
        fhPIDKaonCharge = new TH1D("fhPIDKaonCharge","PID: K: charge; charge", 3,-1.5,1.5);
        fQAPID->Add(fhPIDKaonCharge);
        fhPIDProtonPt = new TH1D("fhPIDProtonPt","PID: p: #it{p}_{T}; #it{p}_{T}", 150,0.,30.);
        fQAPID->Add(fhPIDProtonPt);
        fhPIDProtonPhi = new TH1D("fhPIDProtonPhi","PID: p: #varphi; #varphi", 100,0,TMath::TwoPi());
        fQAPID->Add(fhPIDProtonPhi);
        fhPIDProtonEta = new TH1D("fhPIDProtonEta","PID: p: #eta; #eta", 151,-1.5,1.5);
        fQAPID->Add(fhPIDProtonEta);
        fhPIDProtonCharge = new TH1D("fhPIDProtonCharge","PID: p: charge; charge", 3,-1.5,1.5);
        fQAPID->Add(fhPIDProtonCharge);
        fh2PIDPionTPCdEdx = new TH2D("fh2PIDPionTPCdEdx","PID: #pi: TPC dE/dx; #it{p} (GeV/#it{c}); TPC dE/dx", 200,0,20, 131,-10,1000);
        fQAPID->Add(fh2PIDPionTPCdEdx);
        fh2PIDPionTOFbeta = new TH2D("fh2PIDPionTOFbeta","PID: #pi: TOF #beta; #it{p} (GeV/#it{c});TOF #beta", 200,0,20, 101,-0.1,1.5);
        fQAPID->Add(fh2PIDPionTOFbeta);
        fh2PIDKaonTPCdEdx = new TH2D("fh2PIDKaonTPCdEdx","PID: K: TPC dE/dx; #it{p} (GeV/#it{c}); TPC dE/dx", 200,0,20, 131,-10,1000);
        fQAPID->Add(fh2PIDKaonTPCdEdx);
        fh2PIDKaonTOFbeta = new TH2D("fh2PIDKaonTOFbeta","PID: K: TOF #beta; #it{p} (GeV/#it{c});TOF #beta", 200,0,20, 101,-0.1,1.5);
        fQAPID->Add(fh2PIDKaonTOFbeta);
        fh2PIDProtonTPCdEdx = new TH2D("fh2PIDProtonTPCdEdx","PID: p: TPC dE/dx; #it{p} (GeV/#it{c}); TPC dE/dx", 200,0,20, 131,-10,1000);
        fQAPID->Add(fh2PIDProtonTPCdEdx);
        fh2PIDProtonTOFbeta = new TH2D("fh2PIDProtonTOFbeta","PID: p: TOF #beta; #it{p} (GeV/#it{c});TOF #beta", 200,0,20, 101,-0.1,1.5);
        fQAPID->Add(fh2PIDProtonTOFbeta);
        fh2PIDPionTPCnSigmaPion = new TH2D("fh2PIDPionTPCnSigmaPion","PID: #pi: TPC n#sigma (#pi hyp.); #it{p}_{T} (GeV/#it{c}); TPC n#sigma", 200,0,20, 21,-11,10);
        fQAPID->Add(fh2PIDPionTPCnSigmaPion);
        fh2PIDPionTOFnSigmaPion = new TH2D("fh2PIDPionTOFnSigmaPion","PID: #pi: TOF n#sigma (#pi hyp.); #it{p}_{T} (GeV/#it{c}); TOF n#sigma", 200,0,20, 21,-11,10);
        fQAPID->Add(fh2PIDPionTOFnSigmaPion);
        fh2PIDPionTPCnSigmaKaon = new TH2D("fh2PIDPionTPCnSigmaKaon","PID: #pi: TPC n#sigma (K hyp.); #it{p}_{T} (GeV/#it{c}); TPC n#sigma", 200,0,20, 21,-11,10);
        fQAPID->Add(fh2PIDPionTPCnSigmaKaon);
        fh2PIDPionTOFnSigmaKaon = new TH2D("fh2PIDPionTOFnSigmaKaon","PID: #pi: TOF n#sigma (K hyp.); #it{p}_{T} (GeV/#it{c}); TOF n#sigma", 200,0,20, 21,-11,10);
        fQAPID->Add(fh2PIDPionTOFnSigmaKaon);
        fh2PIDPionTPCnSigmaProton = new TH2D("fh2PIDPionTPCnSigmaProton","PID: #pi: TPC n#sigma (p hyp.); #it{p}_{T} (GeV/#it{c}); TPC n#sigma", 200,0,20, 21,-11,10);
        fQAPID->Add(fh2PIDPionTPCnSigmaProton);
        fh2PIDPionTOFnSigmaProton = new TH2D("fh2PIDPionTOFnSigmaProton","PID: #pi: TOF n#sigma (p hyp.); #it{p}_{T} (GeV/#it{c}); TOF n#sigma", 200,0,20, 21,-11,10);
        fQAPID->Add(fh2PIDPionTOFnSigmaProton);
        fh2PIDKaonTPCnSigmaPion = new TH2D("fh2PIDKaonTPCnSigmaPion","PID: K: TPC n#sigma (#pi hyp.); #it{p}_{T} (GeV/#it{c}); TPC n#sigma", 200,0,20, 21,-11,10);
        fQAPID->Add(fh2PIDKaonTPCnSigmaPion);
        fh2PIDKaonTOFnSigmaPion = new TH2D("fh2PIDKaonTOFnSigmaPion","PID: K: TOF n#sigma (#pi hyp.); #it{p}_{T} (GeV/#it{c}); TOF n#sigma", 200,0,20, 21,-11,10);
        fQAPID->Add(fh2PIDKaonTOFnSigmaPion);
        fh2PIDKaonTPCnSigmaKaon = new TH2D("fh2PIDKaonTPCnSigmaKaon","PID: K: TPC n#sigma (K hyp.); #it{p}_{T} (GeV/#it{c}); TPC n#sigma", 200,0,20, 21,-11,10);
        fQAPID->Add(fh2PIDKaonTPCnSigmaKaon);
        fh2PIDKaonTOFnSigmaKaon = new TH2D("fh2PIDKaonTOFnSigmaKaon","PID: K: TOF n#sigma (K hyp.); #it{p}_{T} (GeV/#it{c}); TOF n#sigma", 200,0,20, 21,-11,10);
        fQAPID->Add(fh2PIDKaonTOFnSigmaKaon);
        fh2PIDKaonTPCnSigmaProton = new TH2D("fh2PIDKaonTPCnSigmaProton","PID: K: TPC n#sigma (p hyp.); #it{p}_{T} (GeV/#it{c}); TPC n#sigma", 200,0,20, 21,-11,10);
        fQAPID->Add(fh2PIDKaonTPCnSigmaProton);
        fh2PIDKaonTOFnSigmaProton = new TH2D("fh2PIDKaonTOFnSigmaProton","PID: K: TOF n#sigma (p hyp.); #it{p}_{T} (GeV/#it{c}); TOF n#sigma", 200,0,20, 21,-11,10);
        fQAPID->Add(fh2PIDKaonTOFnSigmaProton);
        fh2PIDProtonTPCnSigmaPion = new TH2D("fh2PIDProtonTPCnSigmaPion","PID: p: TPC n#sigma (#pi hyp.); #it{p}_{T} (GeV/#it{c}); TPC n#sigma", 200,0,20, 21,-11,10);
        fQAPID->Add(fh2PIDProtonTPCnSigmaPion);
        fh2PIDProtonTOFnSigmaPion = new TH2D("fh2PIDProtonTOFnSigmaPion","PID: p: TOF n#sigma (#pi hyp.); #it{p}_{T} (GeV/#it{c}); TOF n#sigma", 200,0,20, 21,-11,10);
        fQAPID->Add(fh2PIDProtonTOFnSigmaPion);
        fh2PIDProtonTPCnSigmaKaon = new TH2D("fh2PIDProtonTPCnSigmaKaon","PID: p: TPC n#sigma (K hyp.); #it{p}_{T} (GeV/#it{c}); TPC n#sigma", 200,0,20, 21,-11,10);
        fQAPID->Add(fh2PIDProtonTPCnSigmaKaon);
        fh2PIDProtonTOFnSigmaKaon = new TH2D("fh2PIDProtonTOFnSigmaKaon","PID: p: TOF n#sigma (K hyp.); #it{p}_{T} (GeV/#it{c}); TOF n#sigma", 200,0,20, 21,-11,10);
        fQAPID->Add(fh2PIDProtonTOFnSigmaKaon);
        fh2PIDProtonTPCnSigmaProton = new TH2D("fh2PIDProtonTPCnSigmaProton","PID: p: TPC n#sigma (p hyp.); #it{p}_{T} (GeV/#it{c}); TPC n#sigma", 200,0,20, 21,-11,10);
        fQAPID->Add(fh2PIDProtonTPCnSigmaProton);
        fh2PIDProtonTOFnSigmaProton = new TH2D("fh2PIDProtonTOFnSigmaProton","PID: p: TOF n#sigma (p hyp.); #it{p}_{T} (GeV/#it{c}); TOF n#sigma", 200,0,20, 21,-11,10);
        fQAPID->Add(fh2PIDProtonTOFnSigmaProton);
      }

    } //endif {fProcessPID}

    const Short_t iNBinsPIDstatus = 4;
    TString sPIDstatus[iNBinsPIDstatus] = {"kDetNoSignal","kDetPidOk","kDetMismatch","kDetNoParams"};
    const Short_t iNFilterMapBinBins = 32;

    // QA histograms
    if(fFillQA)
    {
      TString sQAindex[fiNumIndexQA] = {"Before", "After"};
      for(Short_t iQA(0); iQA < fiNumIndexQA; iQA++)
      {
        // EVENTs QA histograms
        fhQAEventsPVz[iQA] = new TH1D(Form("fhQAEventsPVz_%s",sQAindex[iQA].Data()), "QA Events: PV-#it{z}", 101,-50,50);
        fQAEvents->Add(fhQAEventsPVz[iQA]);
        fhQAEventsNumContrPV[iQA] = new TH1D(Form("fhQAEventsNumContrPV_%s",sQAindex[iQA].Data()), "QA Events: Number of contributors to AOD PV", 20,0,20);
        fQAEvents->Add(fhQAEventsNumContrPV[iQA]);
        fhQAEventsNumSPDContrPV[iQA] = new TH1D(Form("fhQAEventsNumSPDContrPV_%s",sQAindex[iQA].Data()), "QA Events: SPD contributors to PV", 20,0,20);
        fQAEvents->Add(fhQAEventsNumSPDContrPV[iQA]);
        fhQAEventsDistPVSPD[iQA] = new TH1D(Form("fhQAEventsDistPVSPD_%s",sQAindex[iQA].Data()), "QA Events: PV SPD vertex", 50,0,5);
        fQAEvents->Add(fhQAEventsDistPVSPD[iQA]);
        fhQAEventsSPDresol[iQA] = new TH1D(Form("fhQAEventsSPDresol_%s",sQAindex[iQA].Data()), "QA Events: SPD resolution", 150,0,15);
        fQAEvents->Add(fhQAEventsSPDresol[iQA]);
          
        fhQAEventsCentralityOutliers[iQA] = new TH2D(Form("fhQAEventsCentralityOutliers_%s",sQAindex[iQA].Data()),"QA Events: Centrality distribution; centrality percentile (V0M);centrality percentile (CL1)",100,0,100,100,0,100);
          fQAEvents->Add(fhQAEventsCentralityOutliers[iQA]);
          
        fhQAEventsPileUp[iQA] = new TH2D(Form("fhQAEventsPileUp_%s",sQAindex[iQA].Data()),"QA Events: TPC vs. ESD multiplicity; TPC multiplicity; ESD multiplicity",500,0,6000,500,0,6000);
        fQAEvents->Add(fhQAEventsPileUp[iQA]);
          
        // Charged tracks QA
        if(fProcessCharged)
        {
          fhQAChargedMult[iQA] = new TH1D(Form("fhQAChargedMult_%s",sQAindex[iQA].Data()),"QA Charged: Number of Charged in selected events; #it{N}^{Charged}", 1500,0,1500);
          fQACharged->Add(fhQAChargedMult[iQA]);
          fhQAChargedCharge[iQA] = new TH1D(Form("fhQAChargedCharge_%s",sQAindex[iQA].Data()),"QA Charged: Track charge; charge;", 3,-1.5,1.5);
          fQACharged->Add(fhQAChargedCharge[iQA]);
          fhQAChargedPt[iQA] = new TH1D(Form("fhQAChargedPt_%s",sQAindex[iQA].Data()),"QA Charged: Track #it{p}_{T}; #it{p}_{T} (GeV/#it{c})", 300,0.,30.);
          fQACharged->Add(fhQAChargedPt[iQA]);
          fhQAChargedEta[iQA] = new TH1D(Form("fhQAChargedEta_%s",sQAindex[iQA].Data()),"QA Charged: Track #it{#eta}; #it{#eta}", 151,-1.5,1.5);
          fQACharged->Add(fhQAChargedEta[iQA]);
          fhQAChargedPhi[iQA] = new TH1D(Form("fhQAChargedPhi_%s",sQAindex[iQA].Data()),"QA Charged: Track #it{#varphi}; #it{#varphi}", 100,0.,TMath::TwoPi());
          fQACharged->Add(fhQAChargedPhi[iQA]);
          fhQAChargedFilterBit[iQA] = new TH1D(Form("fhQAChargedFilterBit_%s",sQAindex[iQA].Data()), "QA Charged: Filter bit",iNFilterMapBinBins,0,iNFilterMapBinBins);
          for(Int_t j = 0x0; j < iNFilterMapBinBins; j++) fhQAChargedFilterBit[iQA]->GetXaxis()->SetBinLabel(j+1, Form("%g",TMath::Power(2,j)));
          fQACharged->Add(fhQAChargedFilterBit[iQA]);
          fhQAChargedNumTPCcls[iQA] = new TH1D(Form("fhQAChargedNumTPCcls_%s",sQAindex[iQA].Data()),"QA Charged: Track number of TPC clusters; #it{N}^{TPC clusters}", 160,0,160);
          fQACharged->Add(fhQAChargedNumTPCcls[iQA]);
          fhQAChargedDCAxy[iQA] = new TH1D(Form("fhQAChargedDCAxy_%s",sQAindex[iQA].Data()),"QA Charged: Track DCA-xy; DCA_{#it{xy}} (cm)", 100,0.,10);
          fQACharged->Add(fhQAChargedDCAxy[iQA]);
          fhQAChargedDCAz[iQA] = new TH1D(Form("fhQAChargedDCAz_%s",sQAindex[iQA].Data()),"QA Charged: Track DCA-z; DCA_{#it{z}} (cm)", 200,-10.,10.);
          fQACharged->Add(fhQAChargedDCAz[iQA]);
        } // endif {fProcessCharged}

        // PID tracks QA
        if(fProcessPID)
        {
          fhQAPIDTPCstatus[iQA] = new TH1D(Form("fhQAPIDTPCstatus_%s",sQAindex[iQA].Data()),"QA PID: PID status: TPC;", iNBinsPIDstatus,0,iNBinsPIDstatus);
          fQAPID->Add(fhQAPIDTPCstatus[iQA]);
          fhQAPIDTPCdEdx[iQA] = new TH2D(Form("fhQAPIDTPCdEdx_%s",sQAindex[iQA].Data()),"QA PID: TPC PID information; #it{p} (GeV/#it{c}); TPC dEdx (au)", 100,0,10, 131,-10,1000);
          fQAPID->Add(fhQAPIDTPCdEdx[iQA]);
          fhQAPIDTOFstatus[iQA] = new TH1D(Form("fhQAPIDTOFstatus_%s",sQAindex[iQA].Data()),"QA PID: PID status: TOF;", iNBinsPIDstatus,0,iNBinsPIDstatus);
          fQAPID->Add(fhQAPIDTOFstatus[iQA]);
          fhQAPIDTOFbeta[iQA] = new TH2D(Form("fhQAPIDTOFbeta_%s",sQAindex[iQA].Data()),"QA PID: TOF #beta information; #it{p} (GeV/#it{c}); TOF #beta", 100,0,10, 101,-0.1,1.5);
          fQAPID->Add(fhQAPIDTOFbeta[iQA]);
            
          fh3PIDPionTPCTOFnSigmaPion[iQA] = new TH3D(Form("fh3PIDPionTPCTOFnSigmaPion_%s",sQAindex[iQA].Data()),"QA PID: #pi: TPC-TOF n#sigma (#pi hyp.); n#sigma^{TPC}; n#sigma^{TOF}; #it{p}_{T}", 22,-11,10, 22,-11,10, 100,0,10);
          fQAPID->Add(fh3PIDPionTPCTOFnSigmaPion[iQA]);
          fh3PIDPionTPCTOFnSigmaKaon[iQA] = new TH3D(Form("fh3PIDPionTPCTOFnSigmaKaon_%s",sQAindex[iQA].Data()),"QA PID: #pi: TPC-TOF n#sigma (K hyp.); n#sigma^{TPC}; n#sigma^{TOF}; #it{p}_{T}", 22,-11,10, 22,-11,10, 100,0,10);
          fQAPID->Add(fh3PIDPionTPCTOFnSigmaKaon[iQA]);
          fh3PIDPionTPCTOFnSigmaProton[iQA] = new TH3D(Form("fh3PIDPionTPCTOFnSigmaProton_%s",sQAindex[iQA].Data()),"QA PID: #pi: TPC-TOF n#sigma (p hyp.); n#sigma^{TPC}; n#sigma^{TOF}; #it{p}_{T}", 22,-11,10, 22,-11,10, 100,0,10);
          fQAPID->Add(fh3PIDPionTPCTOFnSigmaProton[iQA]);
            
          fh3PIDKaonTPCTOFnSigmaPion[iQA] = new TH3D(Form("fh3PIDKaonTPCTOFnSigmaPion_%s",sQAindex[iQA].Data()),"QA PID: K: TPC-TOF n#sigma (#pi hyp.); n#sigma^{TPC}; n#sigma^{TOF}; #it{p}_{T}", 22,-11,10, 22,-11,10, 100,0,10);
          fQAPID->Add(fh3PIDKaonTPCTOFnSigmaPion[iQA]);
          fh3PIDKaonTPCTOFnSigmaKaon[iQA] = new TH3D(Form("fh3PIDKaonTPCTOFnSigmaKaon_%s",sQAindex[iQA].Data()),"QA PID: K: TPC-TOF n#sigma (K hyp.); n#sigma^{TPC}; n#sigma^{TOF}; #it{p}_{T}", 22,-11,10, 22,-11,10, 100,0,10);
          fQAPID->Add(fh3PIDKaonTPCTOFnSigmaKaon[iQA]);
          fh3PIDKaonTPCTOFnSigmaProton[iQA] = new TH3D(Form("fh3PIDKaonTPCTOFnSigmaProton_%s",sQAindex[iQA].Data()),"QA PID: K: TPC-TOF n#sigma (p hyp.); n#sigma^{TPC}; n#sigma^{TOF}; #it{p}_{T}", 22,-11,10, 22,-11,10, 100,0,10);
          fQAPID->Add(fh3PIDKaonTPCTOFnSigmaProton[iQA]);
  
          fh3PIDProtonTPCTOFnSigmaPion[iQA] = new TH3D(Form("fh3PIDProtonTPCTOFnSigmaPion_%s",sQAindex[iQA].Data()),"QA PID: p: TPC-TOF n#sigma (#pi hyp.); n#sigma^{TPC}; n#sigma^{TOF}; #it{p}_{T}", 22,-11,10, 22,-11,10, 100,0,10);
          fQAPID->Add(fh3PIDProtonTPCTOFnSigmaPion[iQA]);
          fh3PIDProtonTPCTOFnSigmaKaon[iQA] = new TH3D(Form("fh3PIDProtonTPCTOFnSigmaKaon_%s",sQAindex[iQA].Data()),"QA PID: p: TPC-TOF n#sigma (K hyp.); n#sigma^{TPC}; n#sigma^{TOF}; #it{p}_{T}", 22,-11,10, 22,-11,10, 100,0,10);
          fQAPID->Add(fh3PIDProtonTPCTOFnSigmaKaon[iQA]);
          fh3PIDProtonTPCTOFnSigmaProton[iQA] = new TH3D(Form("fh3PIDProtonTPCTOFnSigmaProton_%s",sQAindex[iQA].Data()),"QA PID: p: TPC-TOF n#sigma (p hyp.); n#sigma^{TPC}; n#sigma^{TOF}; #it{p}_{T}", 22,-11,10, 22,-11,10, 100,0,10);
          fQAPID->Add(fh3PIDProtonTPCTOFnSigmaProton[iQA]);
        
  

          for(Int_t j = 0x0; j < iNBinsPIDstatus; j++)
          {
            fhQAPIDTOFstatus[iQA]->GetXaxis()->SetBinLabel(j+1, sPIDstatus[j].Data() );
            fhQAPIDTPCstatus[iQA]->GetXaxis()->SetBinLabel(j+1, sPIDstatus[j].Data() );
          }
        } // endif {fProcessPID}
      }
    }

  // posting data (mandatory)
  PostData(1, fFlowRefs);
  PostData(2, fFlowCharged);
  PostData(3, fFlowPID);
  PostData(4, fQAEvents);
  PostData(5, fQACharged);
  PostData(6, fQAPID);
  PostData(7, fFlowWeights);

  return;
}//AliAnalysisTaskFlowModes::UserCreateOutputObjects()
//_____________________________________________________________________________
void AliAnalysisTaskFlowModes::ListParameters()
{
  // lists all task parameters
  // *************************************************************
  printf("\n======= List of parameters ========================================\n");
  printf("   -------- Analysis task ---------------------------------------\n");
  printf("      fRunMode: (RunMode) %d\n",    fRunMode);
  printf("      fAnalType: (AnalType) %d\n",    fAnalType);
  printf("      fSampling: (Bool_t) %s\n",    fSampling ? "kTRUE" : "kFALSE");
  printf("      fFillQA: (Bool_t) %s\n",    fFillQA ? "kTRUE" : "kFALSE");
  printf("      fProcessCharged: (Bool_t) %s\n",    fProcessCharged ? "kTRUE" : "kFALSE");
  printf("      fProcessPID: (Bool_t) %s\n",    fProcessPID ? "kTRUE" : "kFALSE");
  printf("   -------- Flow related ----------------------------------------\n");
  printf("      fCutFlowDoFourCorrelations: (Bool_t) %s\n",    fCutFlowDoFourCorrelations ? "kTRUE" : "kFALSE");
  printf("      fCutFlowRFPsPtMin: (Float_t) %g (GeV/c)\n",    fCutFlowRFPsPtMin);
  printf("      fCutFlowRFPsPtMax: (Float_t) %g (GeV/c)\n",    fCutFlowRFPsPtMax);
  printf("      fFlowPOIsPtMin: (Float_t) %g (GeV/c)\n",    fFlowPOIsPtMin);
  printf("      fFlowPOIsPtMax: (Float_t) %g (GeV/c)\n",    fFlowPOIsPtMax);
  printf("      fFlowCentNumBins: (Int_t) %d (GeV/c)\n",    fFlowCentNumBins);
  printf("      fFlowCentMin: (Int_t) %d (GeV/c)\n",    fFlowCentMin);
  printf("      fFlowCentMax: (Int_t) %d (GeV/c)\n",    fFlowCentMax);
  printf("      fFlowUseWeights: (Bool_t) %s\n",    fFlowUseWeights ? "kTRUE" : "kFALSE");
  printf("      fPositivelyChargedRef: (Bool_t) %s\n", fPositivelyChargedRef ? "kTRUE" : "kFALSE");
  printf("      fNegativelyChargedRef: (Bool_t) %s\n", fNegativelyChargedRef ? "kTRUE" : "kFALSE");
  printf("      fFlowWeightsPath: (TString) '%s' \n",    fFlowWeightsPath.Data());
  printf("   -------- Events ----------------------------------------------\n");
  printf("      fTrigger: (Short_t) %d\n",    fTrigger);
  printf("      fMultEstimator: (TString) '%s'\n",    fMultEstimator.Data());
  printf("      fPVtxCutZ: (Double_t) %g (cm)\n",    fPVtxCutZ);
  printf("      fFullCentralityRange: (Bool_t) runs over %s centrality range \n",    fFullCentralityRange ? "0-100%" : "50-100%");
  printf("   -------- Charge tracks ---------------------------------------\n");
  printf("      fCutChargedTrackFilterBit: (UInt) %d\n",    fCutChargedTrackFilterBit);
  printf("      fCutChargedNumTPCclsMin: (UShort_t) %d\n",    fCutChargedNumTPCclsMin);
  printf("      fCutChargedEtaMax: (Float_t) %g\n",    fCutChargedEtaMax);
  printf("      fCutChargedPtMin: (Float_t) %g (GeV/c)\n",    fCutChargedPtMin);
  printf("      fCutChargedPtMax: (Float_t) %g (GeV/c)\n",    fCutChargedPtMax);
  printf("      fCutChargedDCAzMax: (Float_t) %g (cm)\n",    fCutChargedDCAzMax);
  printf("      fCutChargedDCAxyMax: (Float_t) %g (cm)\n",    fCutChargedDCAxyMax);
  printf("   -------- PID (pi,K,p) tracks ---------------------------------\n");
  printf("      fCutPIDUseAntiProtonOnly: (Bool_t) %s\n",  fCutPIDUseAntiProtonOnly ? "kTRUE" : "kFALSE");
  printf("      fCutPIDnSigmaCombinedNoTOFrejection: (Bool_t) %s\n",  fCutPIDnSigmaCombinedNoTOFrejection ? "kTRUE" : "kFALSE");
  printf("      fCutPIDnSigmaPionMax: (Double_t) %g\n",    fCutPIDnSigmaPionMax);
  printf("      fCutPIDnSigmaKaonMax: (Double_t) %g\n",    fCutPIDnSigmaKaonMax);
  printf("      fCutPIDnSigmaProtonMax: (Double_t) %g\n",    fCutPIDnSigmaProtonMax);
  printf("=====================================================================\n\n");

  return;
}
//_____________________________________________________________________________
Bool_t AliAnalysisTaskFlowModes::InitializeTask()
{
  // called once on beginning of task (within UserCreateOutputObjects method)
  // check if task parameters are specified and valid
  // returns kTRUE if succesfull
  // *************************************************************

  printf("====== InitializeTask AliAnalysisTaskFlowModes =========================\n");

  if(fAnalType != kESD && fAnalType != kAOD)
  {
    ::Error("InitializeTask","Analysis type not specified! Terminating!");
    return kFALSE;
  }

  if(fAnalType == kESD)
  {
    ::Error("InitializeTask","Analysis type: ESD not implemented! Terminating!");
    return kFALSE;
  }

  // TODO check if period corresponds to selected collisional system


  // checking PID response
  AliAnalysisManager* mgr = AliAnalysisManager::GetAnalysisManager();
  AliInputEventHandler* inputHandler = (AliInputEventHandler*)mgr->GetInputEventHandler();
  fPIDResponse = inputHandler->GetPIDResponse();
  if(!fPIDResponse)
  {
    ::Error("InitializeTask","AliPIDResponse object not found! Terminating!");
    return kFALSE;
  }

  fPIDCombined = new AliPIDCombined();
  if(!fPIDCombined)
  {
    ::Error("InitializeTask","AliPIDCombined object not found! Terminating!");
    return kFALSE;
  }
  fPIDCombined->SetDefaultTPCPriors();
  fPIDCombined->SetSelectedSpecies(5); // all particle species
  fPIDCombined->SetDetectorMask(AliPIDResponse::kDetTPC+AliPIDResponse::kDetTOF); // setting TPC + TOF mask


  if(fSampling && fNumSamples == 0)
  {
    ::Error("InitializeTask","Sampling used, but number of samples is 0! Terminating!");
    return kFALSE;
  }

  if(!fSampling)
  //fNumSamples = 1;

  // checking cut setting
  ::Info("InitializeTask","Checking task parameters setting conflicts (ranges, etc)");
  if(fCutFlowRFPsPtMin > 0. && fCutFlowRFPsPtMax > 0. && fCutFlowRFPsPtMin > fCutFlowRFPsPtMax)
  {
    ::Error("InitializeTask","Cut: RFPs Pt range wrong!");
    return kFALSE;
  }
 
  // upper-case for multiplicity estimator
  fMultEstimator.ToUpper();

  // checking for weights source file
  if(fFlowUseWeights && !fFlowWeightsPath.EqualTo(""))
  {
    fFlowWeightsFile = TFile::Open(Form("alien:///%s",fFlowWeightsPath.Data()));
    if(!fFlowWeightsFile)
    {
      ::Error("InitializeTask","Flow weights file not found");
      return kFALSE;
    }
  }

  ::Info("InitializeTask","Initialization succesfull!");
  printf("======================================================================\n\n");
  return kTRUE;
}
//_____________________________________________________________________________
void AliAnalysisTaskFlowModes::UserExec(Option_t *)
{
  // main method called for each event (event loop)
  // *************************************************************

  if(!fInit) return; // check if initialization succesfull

  // local event counter check: if running in test mode, it runs until the 50 events are succesfully processed
  if(fRunMode == kTest && fEventCounter >= fNumEventsAnalyse) return;

  // event selection
  fEventAOD = dynamic_cast<AliAODEvent*>(InputEvent());
  if(!EventSelection()) return;

  // fIndexCentrality = GetCentralityIndex();

  // processing of selected event
  if(!ProcessEvent()) return;

  // posting data (mandatory)
  PostData(1, fFlowRefs);
  PostData(2, fFlowCharged);
  PostData(3, fFlowPID);
  PostData(4, fQAEvents);
  PostData(5, fQACharged);
  PostData(6, fQAPID);
  PostData(7, fFlowWeights);

  return;
}
//_____________________________________________________________________________
Bool_t AliAnalysisTaskFlowModes::EventSelection()
{
  // main (envelope) method for event selection
  // Specific event selection methods are called from here
  // returns kTRUE if event pass all selection criteria
  // *************************************************************

  Bool_t eventSelected = kFALSE;

  if(!fEventAOD) return kFALSE;
    
  // Fill event QA BEFORE cuts
  if(fFillQA) FillEventsQA(0);

  // event selection for PbPb

  eventSelected = IsEventSelected();

  return eventSelected;
}
//_____________________________________________________________________________
Bool_t AliAnalysisTaskFlowModes::IsEventSelected()
{
  // Event selection for PbPb collisions recorded in Run 2 year 2015
  // PbPb (LHC15o)
  // return kTRUE if event passes all criteria, kFALSE otherwise
  // *************************************************************
  fhEventCounter->Fill("Input",1);

  // Physics selection (trigger)
  AliAnalysisManager* mgr = AliAnalysisManager::GetAnalysisManager();
  AliInputEventHandler* inputHandler = (AliInputEventHandler*) mgr->GetInputEventHandler();
  UInt_t fSelectMask = inputHandler->IsEventSelected();
  
  Bool_t isTriggerSelected = kFALSE;
  switch(fTrigger) // check for high multiplicity trigger
  {
    case 0:
      isTriggerSelected = fSelectMask& AliVEvent::kINT7;
      break;

    case 1:
      isTriggerSelected = fSelectMask& AliVEvent::kHighMultV0;
      break;

    case 2:
      isTriggerSelected = fSelectMask& AliVEvent::kHighMultSPD;
      break;

    default: isTriggerSelected = kFALSE;
  }
  if(!isTriggerSelected){  return kFALSE;}

  // events passing physics selection
  fhEventCounter->Fill("Physics selection OK",1);

  // get centrality from AliMultSelection
  
   AliMultSelection* fMultSelection = 0x0;
   fMultSelection = (AliMultSelection*) fEventAOD->FindListObject("MultSelection");
   Double_t centrV0M = fMultSelection->GetMultiplicityPercentile("V0M");
   Double_t centrCL1 = fMultSelection->GetMultiplicityPercentile("CL1");
    
  // cut on consistency between centrality estimators: VOM vs CL1
  if(fabs(centrV0M-centrCL1)>7.5) return kFALSE;
  fhEventCounter->Fill("Centr. Est. Consis. OK",1);

  
  // primary vertex selection
  const AliAODVertex* vtx = dynamic_cast<const AliAODVertex*>(fEventAOD->GetPrimaryVertex());
  if(!vtx || vtx->GetNContributors() < 1){ return kFALSE;}
  fhEventCounter->Fill("PV OK",1);

  // SPD vertex selection
  const AliAODVertex* vtxSPD = dynamic_cast<const AliAODVertex*>(fEventAOD->GetPrimaryVertexSPD());
    
    
    if (((AliAODHeader*)fEventAOD->GetHeader())->GetRefMultiplicityComb08() < 0) return kFALSE;
    
    if (fEventAOD->IsIncompleteDAQ()) return kFALSE;
    
    if (vtx->GetNContributors() < 2 || vtxSPD->GetNContributors()<1) return kFALSE;
    double cov[6]={0}; double covSPD[6]={0};
    vtx->GetCovarianceMatrix(cov);
    vtxSPD->GetCovarianceMatrix(covSPD);
    
    //Special selections for SPD vertex
    double zRes = TMath::Sqrt(covSPD[5]);
    double fMaxResol=0.25;
    if ( vtxSPD->IsFromVertexerZ() && (zRes>fMaxResol)) return kFALSE;
    fhEventCounter->Fill("SPD Vtx OK",1);
 
 
  // check for multi-vertexer pile-up
  const int    kMinPileUpContrib = 5;
  const double kMaxPileUpChi2 = 5.0;
  const double kMinWDist = 15;
  const AliAODVertex* vtxPileUp = 0;
  int nPileUp = 0;
  if (nPileUp=fEventAOD->GetNumberOfPileupVerticesTracks()) {
    if (vtx == vtxSPD) return kTRUE; // there are pile-up vertices but no primary
    for (int iPileUp=0;iPileUp<nPileUp;iPileUp++) {
        vtxPileUp = (const AliAODVertex*)fEventAOD->GetPileupVertexTracks(iPileUp);
        if (vtxPileUp->GetNContributors() < kMinPileUpContrib) continue;
        if (vtxPileUp->GetChi2perNDF() > kMaxPileUpChi2) continue;
        //  int bcPlp = vtxPileUp->GetBC();
        //  if (bcPlp!=AliVTrack::kTOFBCNA && TMath::Abs(bcPlp-bcPrim)>2) return kTRUE; // pile-up from other BC
        double wDst = GetWDist(vtx,vtxPileUp);
        if (wDst<kMinWDist) continue;
        return kTRUE; // pile-up: well separated vertices
    }
  }

  fhEventCounter->Fill("Pileup MV OK",1);
    
  // check vertex consistency
  double dz = vtx->GetZ() - vtxSPD->GetZ();
  double errTot = TMath::Sqrt(cov[5]+covSPD[5]);
  double err = TMath::Sqrt(cov[5]);
  double nsigTot = dz/errTot;
  double nsig = dz/err;
  if (TMath::Abs(dz)>0.2 || TMath::Abs(nsigTot)>10 || TMath::Abs(nsig)>20) return kFALSE;
  fhEventCounter->Fill("Vtx Consis. OK",1);

 
  const Double_t aodVtxZ = vtx->GetZ();
  if( TMath::Abs(aodVtxZ) > fPVtxCutZ ){return kFALSE;}
  fhEventCounter->Fill("PV #it{z} OK",1);

  // cut on # ESD tracks vs # TPC only tracks
  const Int_t nTracks = fEventAOD->GetNumberOfTracks();
  Int_t multEsd = ((AliAODHeader*)fEventAOD->GetHeader())->GetNumberOfESDTracks();
  Int_t multTrk = 0;
  Int_t multTrkBefC = 0;
  Int_t multTrkTOFBefC = 0;
  Int_t multTPC = 0;
  for (Int_t it = 0; it < nTracks; it++) {
     AliAODTrack* AODTrk = (AliAODTrack*)fEventAOD->GetTrack(it);
     if (!AODTrk){ delete AODTrk; continue; }
     if (AODTrk->TestFilterBit(128)) {multTPC++;}
  } // end of for (Int_t it = 0; it < nTracks; it++)
  Double_t multTPCn = multTPC;
  Double_t multEsdn = multEsd;
  Double_t multESDTPCDif = multEsdn - multTPCn*3.38;
  if (multESDTPCDif > 15000.) return kFALSE;
  fhEventCounter->Fill("ESD TPC Mult. Diff. OK",1);

    
  fhEventCounter->Fill("Selected",1);
    
  // Fill event QA AFTER cuts
  if(fFillQA) FillEventsQA(1);

  return kTRUE;

}
//_____________________________________________________________________________
void AliAnalysisTaskFlowModes::FillEventsQA(const Short_t iQAindex)
{
  // Filling various QA plots related with event selection
  // *************************************************************

  const AliAODVertex* aodVtx = fEventAOD->GetPrimaryVertex();
  const Double_t dVtxZ = aodVtx->GetZ();
  const Int_t iNumContr = aodVtx->GetNContributors();
  const AliAODVertex* spdVtx = fEventAOD->GetPrimaryVertexSPD();
  const Int_t iNumContrSPD = spdVtx->GetNContributors();
  const Double_t spdVtxZ = spdVtx->GetZ();

  fhQAEventsPVz[iQAindex]->Fill(dVtxZ);
  fhQAEventsNumContrPV[iQAindex]->Fill(iNumContr);
  fhQAEventsNumSPDContrPV[iQAindex]->Fill(iNumContrSPD);
  fhQAEventsDistPVSPD[iQAindex]->Fill(TMath::Abs(dVtxZ - spdVtxZ));

  // SPD vertexer resolution
  Double_t cov[6] = {0};
  spdVtx->GetCovarianceMatrix(cov);
  Double_t zRes = TMath::Sqrt(cov[5]);
  fhQAEventsSPDresol[iQAindex]->Fill(zRes);
    
  AliMultSelection* fMultSelection = 0x0;
  fMultSelection = (AliMultSelection*) fEventAOD->FindListObject("MultSelection");
  Double_t centrV0M = fMultSelection->GetMultiplicityPercentile("V0M");
  Double_t centrCL1 = fMultSelection->GetMultiplicityPercentile("CL1");
    
  fhQAEventsCentralityOutliers[iQAindex]->Fill(centrV0M,centrCL1);
    
  const Int_t nTracks = fEventAOD->GetNumberOfTracks();
  Int_t multEsd = ((AliAODHeader*)fEventAOD->GetHeader())->GetNumberOfESDTracks();
  Int_t multTPC = 0;
  for (Int_t it = 0; it < nTracks; it++) {
      AliAODTrack* AODTrk = (AliAODTrack*)fEventAOD->GetTrack(it);
      if (!AODTrk){ delete AODTrk; continue; }
      if (AODTrk->TestFilterBit(128)) {multTPC++;}
  }// end of for (Int_t it = 0; it < nTracks; it++)
  fhQAEventsPileUp[iQAindex]->Fill(multTPC,multEsd);

  return;
}
//_____________________________________________________________________________
void AliAnalysisTaskFlowModes::Filtering()
{

  // main (envelope) method for filtering all particles of interests (POIs) in selected events
  // All POIs passing selection criteria are saved to relevant TClonesArray for further processing
  // return kTRUE if succesfull (no errors in process)
  // *************************************************************

  if(!fProcessCharged && !fProcessPID) // if neither is ON, filtering is skipped
    return;

  fVectorCharged->clear();
  FilterCharged();

  // estimate centrality & assign indexes (centrality/percentile, sampling, ...)
  fIndexCentrality = GetCentralityIndex();

  if(fIndexCentrality < 0) return; // not succesfull estimation
  fhEventCentrality->Fill(fIndexCentrality);
  fh2EventCentralityNumSelCharged->Fill(fVectorCharged->size(),fIndexCentrality);

  fIndexSampling = GetSamplingIndex();
  fhEventSampling->Fill(fIndexCentrality,fIndexSampling);

  if(fProcessPID)
  {
    fVectorPion->clear();
    fVectorKaon->clear();
    fVectorProton->clear();
    FilterPID();
  }
  return;
}
//_____________________________________________________________________________
void AliAnalysisTaskFlowModes::FilterCharged()
{
  // Filtering input charged tracks
  // If track passes all requirements as defined in IsChargedSelected(),
  // the relevant properties (pT, eta, phi) are stored in FlowPart struct
  // and pushed to relevant vector container.
  // return kFALSE if any complications occurs
  // *************************************************************

  const Short_t iNumTracks = fEventAOD->GetNumberOfTracks();
  if(iNumTracks < 1) return;

  AliAODTrack* track = 0x0;
  Int_t iNumRefs = 0;
  Double_t weight = 0;

  for(Short_t iTrack(0); iTrack < iNumTracks; iTrack++)
  {
    track = static_cast<AliAODTrack*>(fEventAOD->GetTrack(iTrack));
    if(!track) continue;

    if(fFillQA) FillQACharged(0,track); // QA before selection

    if(IsChargedSelected(track))
    {
      fVectorCharged->emplace_back( FlowPart(track->Pt(),track->Phi(),track->Eta(), track->Charge(), kCharged) );
      if(fRunMode == kFillWeights || fFlowFillWeights) fh3WeightsCharged->Fill(track->Phi(),track->Eta(),track->Pt());
      if(fFlowUseWeights)
      {
        weight = fh2WeightCharged->GetBinContent( fh2WeightCharged->FindBin(track->Eta(),track->Phi()) );
        fh3AfterWeightsCharged->Fill(track->Phi(),track->Eta(),track->Pt(),weight);
      }
      if(fFillQA) FillQACharged(1,track); // QA after selection
      //printf("pt %g | phi %g | eta %g\n",track->Pt(),track->Phi(),track->Eta());

      // Filling refs QA plots
      if(fCutFlowRFPsPtMin > 0. && track->Pt() >= fCutFlowRFPsPtMin && fCutFlowRFPsPtMax > 0. && track->Pt() <= fCutFlowRFPsPtMax)
      {
        iNumRefs++;
        if(fRunMode == kFillWeights || fFlowFillWeights) fh3WeightsRefs->Fill(track->Phi(),track->Eta(),track->Pt());
        if(fFlowUseWeights)
        {
          weight = fh2WeightRefs->GetBinContent( fh2WeightRefs->FindBin(track->Eta(),track->Phi()) );
          fh3AfterWeightsRefs->Fill(track->Phi(),track->Eta(),track->Pt(),weight);
        }
        FillQARefs(1,track);
      }
    }
  }

  // fill QA charged multiplicity
  fhRefsMult->Fill(iNumRefs);
  if(fFillQA)
  {
    fhQAChargedMult[0]->Fill(fEventAOD->GetNumberOfTracks());
    fhQAChargedMult[1]->Fill(fVectorCharged->size());
  }

  return;
}
//_____________________________________________________________________________
Bool_t AliAnalysisTaskFlowModes::IsChargedSelected(const AliAODTrack* track)
{
  // Selection of charged track
  // returns kTRUE if track pass all requirements, kFALSE otherwise
  // *************************************************************
  if(!track) return kFALSE;
  fhChargedCounter->Fill("Input",1);

  // filter bit
  if( !track->TestFilterBit(fCutChargedTrackFilterBit) ) return kFALSE;
  fhChargedCounter->Fill("FB",1);

  // number of TPC clusters (additional check for not ITS-standalone tracks)
  if( track->GetTPCNcls() < fCutChargedNumTPCclsMin && fCutChargedTrackFilterBit != 2) return kFALSE;
  fhChargedCounter->Fill("#TPC-Cls",1);

  // track DCA coordinates
  // note AliAODTrack::XYZAtDCA() works only for constrained tracks
  Double_t dTrackXYZ[3] = {0};
  Double_t dVertexXYZ[3] = {0.};
  Double_t dDCAXYZ[3] = {0.};
  if( fCutChargedDCAzMax > 0. || fCutChargedDCAxyMax > 0.)
  {
    const AliAODVertex* vertex = fEventAOD->GetPrimaryVertex();
    if(!vertex) return kFALSE; // event does not have a PV

    track->GetXYZ(dTrackXYZ);
    vertex->GetXYZ(dVertexXYZ);

    for(Short_t i(0); i < 3; i++)
      dDCAXYZ[i] = dTrackXYZ[i] - dVertexXYZ[i];
  }

  if(fCutChargedDCAzMax > 0. && TMath::Abs(dDCAXYZ[2]) > fCutChargedDCAzMax) return kFALSE;
  fhChargedCounter->Fill("DCA-z",1);

  if(fCutChargedDCAxyMax > 0. && TMath::Sqrt(dDCAXYZ[0]*dDCAXYZ[0] + dDCAXYZ[1]*dDCAXYZ[1]) > fCutChargedDCAxyMax) return kFALSE;
  fhChargedCounter->Fill("DCA-xy",1);

  // pseudorapidity (eta)
  if(fCutChargedEtaMax > 0. && TMath::Abs(track->Eta()) > fCutChargedEtaMax) return kFALSE;
  fhChargedCounter->Fill("Eta",1);

  // track passing all criteria
  fhChargedCounter->Fill("Selected",1);
  return kTRUE;
}
//_____________________________________________________________________________
void AliAnalysisTaskFlowModes::FillQARefs(const Short_t iQAindex, const AliAODTrack* track)
{
  // Filling various QA plots related to RFPs subset of charged track selection
  // *************************************************************

  if(!track) return;
  if(iQAindex == 0) return; // NOTE implemented only for selected RFPs

  fhRefsPt->Fill(track->Pt());
  fhRefsEta->Fill(track->Eta());
  fhRefsPhi->Fill(track->Phi());

  return;
}
//_____________________________________________________________________________
void AliAnalysisTaskFlowModes::FillQACharged(const Short_t iQAindex, const AliAODTrack* track)
{
  // Filling various QA plots related to charged track selection
  // *************************************************************
  if(!track) return;

  // filter bit testing
  for(Short_t i(0); i < 32; i++)
  {
    if(track->TestFilterBit(TMath::Power(2.,i)))
      fhQAChargedFilterBit[iQAindex]->Fill(i);
  }

  // track charge
  fhQAChargedCharge[iQAindex]->Fill(track->Charge());

  // number of TPC clusters
  fhQAChargedNumTPCcls[iQAindex]->Fill(track->GetTPCNcls());

  // track DCA
  Double_t dDCAXYZ[3] = {-999., -999., -999.};
  const AliAODVertex* vertex = fEventAOD->GetPrimaryVertex();
  if(vertex)
  {
    Double_t dTrackXYZ[3] = {-999., -999., -999.};
    Double_t dVertexXYZ[3] = {-999., -999., -999.};

    track->GetXYZ(dTrackXYZ);
    vertex->GetXYZ(dVertexXYZ);

    for(Short_t i(0); i < 3; i++)
      dDCAXYZ[i] = dTrackXYZ[i] - dVertexXYZ[i];
  }
  fhQAChargedDCAxy[iQAindex]->Fill(TMath::Sqrt(dDCAXYZ[0]*dDCAXYZ[0] + dDCAXYZ[1]*dDCAXYZ[1]));
  fhQAChargedDCAz[iQAindex]->Fill(dDCAXYZ[2]);

  // kinematics
  fhQAChargedPt[iQAindex]->Fill(track->Pt());
  fhQAChargedPhi[iQAindex]->Fill(track->Phi());
  fhQAChargedEta[iQAindex]->Fill(track->Eta());

  return;
}
//_____________________________________________________________________________
void AliAnalysisTaskFlowModes::FilterPID()
{
  // If track passes all requirements as defined in IsPIDSelected() (and species dependent),
  // the relevant properties (pT, eta, phi) are stored in FlowPart struct
  // and pushed to relevant vector container.
  // return kFALSE if any complications occurs
  // *************************************************************

  const Short_t iNumTracks = fEventAOD->GetNumberOfTracks();
  if(iNumTracks < 1) return;

  PartSpecies species = kUnknown;
  AliAODTrack* track = 0x0;
  Double_t weight = 0;

  for(Short_t iTrack(0); iTrack < iNumTracks; iTrack++)
  {
    track = static_cast<AliAODTrack*>(fEventAOD->GetTrack(iTrack));
    if(!track) continue;

    // PID tracks are subset of selected charged tracks (same quality requirements)
    if(!IsChargedSelected(track)) continue;

    if(fFillQA) FillPIDQA(0,track,kUnknown);   // filling QA for tracks before selection (but after charged criteria applied)

    // PID track selection (return most favourable species)
    PartSpecies species = IsPIDSelected(track);
    // check if only protons should be used
    if(fCutPIDUseAntiProtonOnly && species == kProton && track->Charge() == 1) species = kUnknown;

    // selection of PID tracks
    switch (species)
    {
      case kPion:
        fVectorPion->emplace_back( FlowPart(track->Pt(),track->Phi(),track->Eta(), track->Charge(), kPion, fPDGMassPion, track->Px(), track->Py(), track->Pz()) );
        if(fRunMode == kFillWeights || fFlowFillWeights) fh3WeightsPion->Fill(track->Phi(), track->Eta(), track->Pt());
        if(fFlowUseWeights)
        {
          weight = fh2WeightPion->GetBinContent( fh2WeightPion->FindBin(track->Eta(),track->Phi()) );
          fh3AfterWeightsPion->Fill(track->Phi(),track->Eta(),track->Pt(),weight);
        }
        break;
      case kKaon:
        fVectorKaon->emplace_back( FlowPart(track->Pt(),track->Phi(),track->Eta(), track->Charge(), kKaon, fPDGMassKaon, track->Px(), track->Py(), track->Pz()) );
        if(fRunMode == kFillWeights || fFlowFillWeights) fh3WeightsKaon->Fill(track->Phi(), track->Eta(), track->Pt());
        if(fFlowUseWeights)
        {
          weight = fh2WeightKaon->GetBinContent( fh2WeightKaon->FindBin(track->Eta(),track->Phi()) );
          fh3AfterWeightsKaon->Fill(track->Phi(),track->Eta(),track->Pt(),weight);
        }
        break;
      case kProton:
        fVectorProton->emplace_back( FlowPart(track->Pt(),track->Phi(),track->Eta(), track->Charge(), kProton, fPDGMassProton, track->Px(), track->Py(), track->Pz()) );
        if(fRunMode == kFillWeights || fFlowFillWeights) fh3WeightsProton->Fill(track->Phi(), track->Eta(), track->Pt());
        if(fFlowUseWeights)
        {
          weight = fh2WeightProton->GetBinContent( fh2WeightProton->FindBin(track->Eta(),track->Phi()) );
          fh3AfterWeightsProton->Fill(track->Phi(),track->Eta(),track->Pt(),weight);
        }
        break;
      default:
        continue;
    }

    if(fFillQA) FillPIDQA(1,track,species); // filling QA for tracks AFTER selection
  }

  fhPIDPionMult->Fill(fVectorPion->size());
  fhPIDKaonMult->Fill(fVectorKaon->size());
  fhPIDProtonMult->Fill(fVectorProton->size());

  return;
}
//_____________________________________________________________________________
AliAnalysisTaskFlowModes::PartSpecies AliAnalysisTaskFlowModes::IsPIDSelected(const AliAODTrack* track)
{
  // Selection of PID tracks (pi,K,p) - track identification
  // nSigma cutting is used
  // returns AliAnalysisTaskFlowModes::PartSpecies enum : kPion, kKaon, kProton if any of this passed kUnknown otherwise
  // *************************************************************

  // checking detector states
  AliPIDResponse::EDetPidStatus pidStatusTPC = fPIDResponse->CheckPIDStatus(AliPIDResponse::kTPC, track);
  AliPIDResponse::EDetPidStatus pidStatusTOF = fPIDResponse->CheckPIDStatus(AliPIDResponse::kTOF, track);

  Bool_t bIsTPCok = (pidStatusTPC == AliPIDResponse::kDetPidOk);
  Bool_t bIsTOFok = ((pidStatusTOF == AliPIDResponse::kDetPidOk) && (track->GetStatus()& AliVTrack::kTOFout) && (track->GetStatus()& AliVTrack::kTIME) && (track->GetTOFsignal() > 12000) && (track->GetTOFsignal() < 100000)); // checking TOF
  //if(!bIsTPCok) return kUnknown;
    
  const Double_t dPt = track->Pt();

  // use nSigma cuts (based on combination of TPC / TOF nSigma cuts)

  Double_t dNumSigmaTPC[5] = {-99,-99,-99,-99,-99}; // TPC nSigma array: 0: electron / 1: muon / 2: pion / 3: kaon / 4: proton
  Double_t dNumSigmaTOF[5] = {-99,-99,-99,-99,-99}; // TOF nSigma array: 0: electron / 1: muon / 2: pion / 3: kaon / 4: proton
  
  Float_t *probabilities;
  Float_t mismProb;
  Float_t ProbBayes[5] = {0,0,0,0,0}; //0=el, 1=mu, 2=pi, 3=ka, 4=pr, 5=deuteron, 6=triton, 7=He3

  // filling nSigma arrays
  if(bIsTPCok) // should be anyway
  {
      dNumSigmaTPC[0] = TMath::Abs(fPIDResponse->NumberOfSigmasTPC(track, AliPID::kElectron));
      dNumSigmaTPC[1] = TMath::Abs(fPIDResponse->NumberOfSigmasTPC(track, AliPID::kMuon));
      dNumSigmaTPC[2] = TMath::Abs(fPIDResponse->NumberOfSigmasTPC(track, AliPID::kPion));
      dNumSigmaTPC[3] = TMath::Abs(fPIDResponse->NumberOfSigmasTPC(track, AliPID::kKaon));
      dNumSigmaTPC[4] = TMath::Abs(fPIDResponse->NumberOfSigmasTPC(track, AliPID::kProton));
  }

  if(bIsTOFok) // should be anyway
  {
      dNumSigmaTOF[0] = TMath::Abs(fPIDResponse->NumberOfSigmasTOF(track, AliPID::kElectron));
      dNumSigmaTOF[1] = TMath::Abs(fPIDResponse->NumberOfSigmasTOF(track, AliPID::kMuon));
      dNumSigmaTOF[2] = TMath::Abs(fPIDResponse->NumberOfSigmasTOF(track, AliPID::kPion));
      dNumSigmaTOF[3] = TMath::Abs(fPIDResponse->NumberOfSigmasTOF(track, AliPID::kKaon));
      dNumSigmaTOF[4] = TMath::Abs(fPIDResponse->NumberOfSigmasTOF(track, AliPID::kProton));
  }
  
    
  if(fPIDbayesian){
      if(!TPCTOFagree(track)){return kUnknown;}

      fBayesianResponse->SetDetResponse(fEventAOD, fCurrCentr,AliESDpid::kTOF_T0); // centrality = PbPb centrality class (0-100%) or -1 for pp collisions
      if(fEventAOD->GetTOFHeader()){
          fESDpid.SetTOFResponse(fEventAOD,AliESDpid::kTOF_T0);
      }
      fBayesianResponse->SetDetAND(1);
  }
    
  // TPC nSigma cuts
  if(dPt <= 0.4)
  {
      if(fPID3sigma){

          Double_t dMinSigmasTPC = TMath::MinElement(5,dNumSigmaTPC);
          // electron rejection
          if(dMinSigmasTPC == dNumSigmaTPC[0] && TMath::Abs(dNumSigmaTPC[0]) <= fCutPIDnSigmaTPCRejectElectron) return kUnknown;
          if(dMinSigmasTPC == dNumSigmaTPC[2] && TMath::Abs(dNumSigmaTPC[2]) <= fCutPIDnSigmaPionMax) return kPion;
          if(dMinSigmasTPC == dNumSigmaTPC[3] && TMath::Abs(dNumSigmaTPC[3]) <= fCutPIDnSigmaKaonMax) return kKaon;
          if(dMinSigmasTPC == dNumSigmaTPC[4] && TMath::Abs(dNumSigmaTPC[4]) <= fCutPIDnSigmaProtonMax) return kProton;
      }
      if(fPIDbayesian){

          fBayesianResponse->ComputeProb(track,track->GetAODEvent()); // fCurrCentr is needed for mismatch fraction
          probabilities = fBayesianResponse->GetProb(); // Bayesian Probability (from 0 to 4) (Combined TPC || TOF) including a tuning of priors and TOF mism$
          ProbBayes[0] = probabilities[0];
          ProbBayes[1] = probabilities[1];
          ProbBayes[2] = probabilities[2];
          ProbBayes[3] = probabilities[3];
          ProbBayes[4] = probabilities[4];
          
          mismProb = fBayesianResponse->GetTOFMismProb(); // mismatch Bayesian probabilities
          
          Double_t dMaxBayesianProb = TMath::MaxElement(5,ProbBayes);
          if(dMaxBayesianProb > fParticleProbability && mismProb < 0.5){
              // electron rejection
              if(dMaxBayesianProb == ProbBayes[0] && TMath::Abs(dNumSigmaTPC[0]) <= fCutPIDnSigmaTPCRejectElectron) return kUnknown;
              if(dMaxBayesianProb == ProbBayes[2] && TMath::Abs(dNumSigmaTPC[2]) <= fCutPIDnSigmaPionMax){Printf("Pion found\n"); return kPion;}
              if(dMaxBayesianProb == ProbBayes[3] && TMath::Abs(dNumSigmaTPC[3]) <= fCutPIDnSigmaKaonMax){Printf("Kaon found\n"); return kKaon;}
              if(dMaxBayesianProb == ProbBayes[4] && TMath::Abs(dNumSigmaTPC[4]) <= fCutPIDnSigmaProtonMax){Printf("Proton found\n"); return kProton;}
          }
      }
  }

  // combined TPC + TOF nSigma cuts
  if(dPt > 0.4) // && < 4 GeV TODO once TPC dEdx parametrisation is available
  {
      Double_t dNumSigmaCombined[5] = {-99,-99,-99,-99,-99};

      // discard candidates if no TOF is available if cut is on
      if(fCutPIDnSigmaCombinedNoTOFrejection && !bIsTOFok) return kUnknown;

      // calculating combined nSigmas
      for(Short_t i(0); i < 5; i++)
      {
        if(bIsTOFok) { dNumSigmaCombined[i] = TMath::Sqrt(dNumSigmaTPC[i]*dNumSigmaTPC[i] + dNumSigmaTOF[i]*dNumSigmaTOF[i]); }
        else { dNumSigmaCombined[i] = dNumSigmaTPC[i]; }
      }

      if(fPID3sigma){
          Double_t dMinSigmasCombined = TMath::MinElement(5,dNumSigmaCombined);

          // electron rejection
          if(dMinSigmasCombined == dNumSigmaCombined[0] && TMath::Abs(dNumSigmaCombined[0]) <= fCutPIDnSigmaPionMax) return kUnknown;
          if(dMinSigmasCombined == dNumSigmaCombined[2] && TMath::Abs(dNumSigmaCombined[2]) <= fCutPIDnSigmaPionMax){Printf("Pion found\n"); return kPion;}
          if(dMinSigmasCombined == dNumSigmaCombined[3] && TMath::Abs(dNumSigmaCombined[3]) <= fCutPIDnSigmaKaonMax){Printf("Kaon found\n"); return kKaon;}
          if(dMinSigmasCombined == dNumSigmaCombined[4] && TMath::Abs(dNumSigmaCombined[4]) <= fCutPIDnSigmaProtonMax){Printf("Proton found\n"); return kProton;}
      }
      if(fPIDbayesian){

          fBayesianResponse->ComputeProb(track,track->GetAODEvent()); // fCurrCentr is needed for mismatch fraction
          probabilities = fBayesianResponse->GetProb(); // Bayesian Probability (from 0 to 4) (Combined TPC || TOF) including a tuning of priors and TOF mism$
          ProbBayes[0] = probabilities[0];
          ProbBayes[1] = probabilities[1];
          ProbBayes[2] = probabilities[2];
          ProbBayes[3] = probabilities[3];
          ProbBayes[4] = probabilities[4];
          
          mismProb = fBayesianResponse->GetTOFMismProb(); // mismatch Bayesian probabilities
          
          Double_t dMaxBayesianProb = TMath::MaxElement(5,ProbBayes);
          if(dMaxBayesianProb > fParticleProbability && mismProb < 0.5){
              // electron rejection
              if(dMaxBayesianProb == ProbBayes[0] && TMath::Abs(dNumSigmaCombined[0]) <= fCutPIDnSigmaPionMax) return kUnknown;
              if(dMaxBayesianProb == ProbBayes[2] && TMath::Abs(dNumSigmaCombined[2]) <= fCutPIDnSigmaPionMax){Printf("Pion found\n"); return kPion;}
              if(dMaxBayesianProb == ProbBayes[3] && TMath::Abs(dNumSigmaCombined[3]) <= fCutPIDnSigmaKaonMax){Printf("Kaon found\n"); return kKaon;}
              if(dMaxBayesianProb == ProbBayes[4] && TMath::Abs(dNumSigmaCombined[4]) <= fCutPIDnSigmaProtonMax){Printf("Proton found\n"); return kProton;}
          }
      }
  }

    // TPC dEdx parametrisation (dEdx - <dEdx>)
    // TODO: TPC dEdx parametrisation cuts
    // if(dPt > 3.)
    // {
    //
    // }

  return kUnknown;
}
//_____________________________________________________________________________
void AliAnalysisTaskFlowModes::FillPIDQA(const Short_t iQAindex, const AliAODTrack* track, const PartSpecies species)
{
  // Filling various QA plots related to PID (pi,K,p) track selection
  // *************************************************************
  if(!track) return;

  if(!fPIDResponse || !fPIDCombined)
  {
    ::Error("FillPIDQA","AliPIDResponse or AliPIDCombined object not found!");
    return;
  }

  // TPC & TOF statuses & measures
  AliPIDResponse::EDetPidStatus pidStatusTPC = fPIDResponse->CheckPIDStatus(AliPIDResponse::kTPC, track);
  AliPIDResponse::EDetPidStatus pidStatusTOF = fPIDResponse->CheckPIDStatus(AliPIDResponse::kTOF, track);

  fhQAPIDTOFstatus[iQAindex]->Fill((Int_t) pidStatusTOF );
  fhQAPIDTPCstatus[iQAindex]->Fill((Int_t) pidStatusTPC );

  Bool_t bIsTPCok = (pidStatusTPC == AliPIDResponse::kDetPidOk);
  //Bool_t bIsTOFok = ((pidStatusTOF == AliPIDResponse::kDetPidOk) && (track->GetStatus()& AliVTrack::kTOFout) && (track->GetStatus()& AliVTrack::kTIME));
    
  Bool_t bIsTOFok = ((pidStatusTOF == AliPIDResponse::kDetPidOk) && (track->GetStatus()& AliVTrack::kTOFout) && (track->GetStatus()& AliVTrack::kTIME) && (track->GetTOFsignal() > 12000) && (track->GetTOFsignal() < 100000)); // checking TOF

  Double_t dNumSigmaTPC[5] = {-11}; // array: 0: electron / 1: muon / 2: pion / 3: kaon / 4: proton
  Double_t dNumSigmaTOF[5] = {-11}; // array: 0: electron / 1: muon / 2: pion / 3: kaon / 4: proton

  Double_t dTPCdEdx = -5; // TPC dEdx for selected particle
  Double_t dTOFbeta = -0.05; //TOF beta for selected particle

  Double_t dP = track->P();
  Double_t dPt = track->Pt();

  // detector status dependent
  if(bIsTPCok)
  {
    dNumSigmaTPC[0] = fPIDResponse->NumberOfSigmasTPC(track, AliPID::kElectron);
    dNumSigmaTPC[1] = fPIDResponse->NumberOfSigmasTPC(track, AliPID::kMuon);
    dNumSigmaTPC[2] = fPIDResponse->NumberOfSigmasTPC(track, AliPID::kPion);
    dNumSigmaTPC[3] = fPIDResponse->NumberOfSigmasTPC(track, AliPID::kKaon);
    dNumSigmaTPC[4] = fPIDResponse->NumberOfSigmasTPC(track, AliPID::kProton);

    dTPCdEdx = track->GetTPCsignal();
    fhQAPIDTPCdEdx[iQAindex]->Fill(track->P(), dTPCdEdx);
  }
  else // TPC status not OK
  {
    dNumSigmaTPC[0] = -11.;
    dNumSigmaTPC[1] = -11.;
    dNumSigmaTPC[2] = -11.;
    dNumSigmaTPC[3] = -11.;
    dNumSigmaTPC[4] = -11.;

    fhQAPIDTPCdEdx[iQAindex]->Fill(track->P(), -5.);
  }

  if(bIsTOFok)
  {
    dNumSigmaTOF[0] = fPIDResponse->NumberOfSigmasTOF(track, AliPID::kElectron);
    dNumSigmaTOF[1] = fPIDResponse->NumberOfSigmasTOF(track, AliPID::kMuon);
    dNumSigmaTOF[2] = fPIDResponse->NumberOfSigmasTOF(track, AliPID::kPion);
    dNumSigmaTOF[3] = fPIDResponse->NumberOfSigmasTOF(track, AliPID::kKaon);
    dNumSigmaTOF[4] = fPIDResponse->NumberOfSigmasTOF(track, AliPID::kProton);

    Double_t dTOF[5];
    track->GetIntegratedTimes(dTOF);
    dTOFbeta = dTOF[0] / track->GetTOFsignal();
    fhQAPIDTOFbeta[iQAindex]->Fill(dP,dTOFbeta);
  }
  else // TOF status not OK
  {
    dNumSigmaTOF[0] = -11.;
    dNumSigmaTOF[1] = -11.;
    dNumSigmaTOF[2] = -11.;
    dNumSigmaTOF[3] = -11.;
    dNumSigmaTOF[4] = -11.;

    fhQAPIDTOFbeta[iQAindex]->Fill(track->P(),-0.05);
  }


  // species dependent QA
  switch (species)
  {
    case kPion:
      fhPIDPionPt->Fill(track->Pt());
      fhPIDPionPhi->Fill(track->Phi());
      fhPIDPionEta->Fill(track->Eta());
      fhPIDPionCharge->Fill(track->Charge());
      fh2PIDPionTPCdEdx->Fill(dPt,dTPCdEdx);
      fh2PIDPionTOFbeta->Fill(dPt,dTOFbeta);
      fh2PIDPionTPCnSigmaPion->Fill(dPt,dNumSigmaTPC[2]);
      fh2PIDPionTOFnSigmaPion->Fill(dPt,dNumSigmaTOF[2]);
      fh2PIDPionTPCnSigmaKaon->Fill(dPt,dNumSigmaTPC[3]);
      fh2PIDPionTOFnSigmaKaon->Fill(dPt,dNumSigmaTOF[3]);
      fh2PIDPionTPCnSigmaProton->Fill(dPt,dNumSigmaTPC[4]);
      fh2PIDPionTOFnSigmaProton->Fill(dPt,dNumSigmaTOF[4]);
          
      fh3PIDPionTPCTOFnSigmaPion[iQAindex]->Fill(dNumSigmaTPC[2],dNumSigmaTOF[2],dPt);
      fh3PIDPionTPCTOFnSigmaKaon[iQAindex]->Fill(dNumSigmaTPC[3],dNumSigmaTOF[3],dPt);
      fh3PIDPionTPCTOFnSigmaProton[iQAindex]->Fill(dNumSigmaTPC[4],dNumSigmaTOF[4],dPt);
    
      break;

    case kKaon:
      fhPIDKaonPt->Fill(track->Pt());
      fhPIDKaonPhi->Fill(track->Phi());
      fhPIDKaonEta->Fill(track->Eta());
      fhPIDKaonCharge->Fill(track->Charge());
      fh2PIDKaonTPCdEdx->Fill(dP,dTPCdEdx);
      fh2PIDKaonTOFbeta->Fill(dP,dTOFbeta);
      fh2PIDKaonTPCnSigmaPion->Fill(dPt,dNumSigmaTPC[2]);
      fh2PIDKaonTOFnSigmaPion->Fill(dPt,dNumSigmaTOF[2]);
      fh2PIDKaonTPCnSigmaKaon->Fill(dPt,dNumSigmaTPC[3]);
      fh2PIDKaonTOFnSigmaKaon->Fill(dPt,dNumSigmaTOF[3]);
      fh2PIDKaonTPCnSigmaProton->Fill(dPt,dNumSigmaTPC[4]);
      fh2PIDKaonTOFnSigmaProton->Fill(dPt,dNumSigmaTOF[4]);
          
      fh3PIDKaonTPCTOFnSigmaPion[iQAindex]->Fill(dNumSigmaTPC[2],dNumSigmaTOF[2],dPt);
      fh3PIDKaonTPCTOFnSigmaKaon[iQAindex]->Fill(dNumSigmaTPC[3],dNumSigmaTOF[3],dPt);
      fh3PIDKaonTPCTOFnSigmaProton[iQAindex]->Fill(dNumSigmaTPC[4],dNumSigmaTOF[4],dPt);

      break;

    case kProton:
      fhPIDProtonPt->Fill(track->Pt());
      fhPIDProtonPhi->Fill(track->Phi());
      fhPIDProtonEta->Fill(track->Eta());
      fhPIDProtonCharge->Fill(track->Charge());
      fh2PIDProtonTPCdEdx->Fill(dP,dTPCdEdx);
      fh2PIDProtonTOFbeta->Fill(dP,dTOFbeta);
      fh2PIDProtonTPCnSigmaPion->Fill(dPt,dNumSigmaTPC[2]);
      fh2PIDProtonTOFnSigmaPion->Fill(dPt,dNumSigmaTOF[2]);
      fh2PIDProtonTPCnSigmaKaon->Fill(dPt,dNumSigmaTPC[3]);
      fh2PIDProtonTOFnSigmaKaon->Fill(dPt,dNumSigmaTOF[3]);
      fh2PIDProtonTPCnSigmaProton->Fill(dPt,dNumSigmaTPC[4]);
      fh2PIDProtonTOFnSigmaProton->Fill(dPt,dNumSigmaTOF[4]);
          
      fh3PIDProtonTPCTOFnSigmaPion[iQAindex]->Fill(dNumSigmaTPC[2],dNumSigmaTOF[2],dPt);
      fh3PIDProtonTPCTOFnSigmaKaon[iQAindex]->Fill(dNumSigmaTPC[3],dNumSigmaTOF[3],dPt);
      fh3PIDProtonTPCTOFnSigmaProton[iQAindex]->Fill(dNumSigmaTPC[4],dNumSigmaTOF[4],dPt);

      break;

    default:
      break;
  }
    
    
  //


  return;
}
//_____________________________________________________________________________
Bool_t AliAnalysisTaskFlowModes::ProcessEvent()
{

  // main method for processing of (selected) event:
  // - Filtering of tracks / particles for flow calculations
  // - Phi,eta,pt weights for generic framework are calculated if specified
  // - Flow calculations
  // returns kTRUE if succesfull
  // *************************************************************

  // printf("======= EVENT ================\n");

  // checking the run number for aplying weights & loading TList with weights
  // if(fFlowUseWeights && fRunNumber < 0)
  if(fFlowUseWeights && (fRunNumber < 0 || fRunNumber != fEventAOD->GetRunNumber()) )
  { 
    fRunNumber = fEventAOD->GetRunNumber();
    if(fFlowWeightsFile)
    {
      TList* listFlowWeights = (TList*) fFlowWeightsFile->Get(Form("%d",fRunNumber));
      if(!listFlowWeights) {::Error("ProcessEvent","TList from flow weights not found."); return kFALSE; }
      fh2WeightRefs = (TH2D*) listFlowWeights->FindObject("Refs"); if(!fh2WeightRefs) { ::Error("ProcessEvent","Refs weights not found"); return kFALSE; }

      fh2WeightCharged = (TH2D*) listFlowWeights->FindObject("Charged"); if(!fh2WeightCharged) { ::Error("ProcessEvent","Charged weights not found"); return kFALSE; }
      fh2WeightPion = (TH2D*) listFlowWeights->FindObject("Pion"); if(!fh2WeightPion) { ::Error("ProcessEvent","Pion weights not found"); return kFALSE; }
      fh2WeightKaon = (TH2D*) listFlowWeights->FindObject("Kaon"); if(!fh2WeightKaon) { ::Error("ProcessEvent","Kaon weights not found"); return kFALSE; }
      fh2WeightProton = (TH2D*) listFlowWeights->FindObject("Proton"); if(!fh2WeightProton) { ::Error("ProcessEvent","Proton weights not found"); return kFALSE; }
    }
  }

  // filtering particles
  Filtering();
  // at this point, centrality index (percentile) should be properly estimated, if not, skip event
  if(fIndexCentrality < 0) return kFALSE;


  // if running in kFillWeights mode, skip the remaining part
  if(fRunMode == kFillWeights) { fEventCounter++; return kTRUE; }

  // checking if there is at least one charged track selected;
  // if not, event is skipped: unable to compute Reference flow (and thus any differential flow)
  if(fVectorCharged->size() < 1)
    return kFALSE;

  // at this point, all particles fullfiling relevant POIs (and REFs) criteria are filled in TClonesArrays

  // >>>> flow starts here <<<<
  // >>>> Flow a la General Framework <<<<
  for(Short_t iGap(0); iGap < fNumEtaGap; iGap++)
  {

    // Reference (pT integrated) flow
    DoFlowRefs(iGap);

    // pT differential
    if(fProcessCharged)
    {
      // charged track flow
      DoFlowCharged(iGap);
    }

    if(fProcessPID)
    {
      const Int_t iSizePion = fVectorPion->size();
      const Int_t iSizeKaon = fVectorKaon->size();
      const Int_t iSizeProton = fVectorProton->size();

      // pi,K,p flow
      if(iSizePion > 0) DoFlowPID(iGap,kPion);
      if(iSizeKaon > 0) DoFlowPID(iGap,kKaon);
      if(iSizeProton > 0) DoFlowPID(iGap,kProton);
    }
  } // endfor {iGap} eta gaps

  fEventCounter++; // counter of processed events
  //printf("event %d\n",fEventCounter);

  return kTRUE;
}
//_____________________________________________________________________________
void AliAnalysisTaskFlowModes::DoFlowRefs(const Short_t iEtaGapIndex)
{

  // Estimate <2> for reference flow for all harmonics based on relevant flow vectors
  // *************************************************************

  Float_t dEtaGap = fEtaGap[iEtaGapIndex];
  Short_t iHarmonics = 0;
  Double_t Cn2 = 0;
  Double_t Dn4GapP = 0;
  TComplex vector = TComplex(0,0,kFALSE);
  Double_t dValue = 999;

  FillRefsVectors(iEtaGapIndex); // filling RFPs (Q) flow vectors
    if(!fDoOnlyMixedCorrelations){
        // estimating <2>
        Cn2 = TwoGap(0,0).Re();
        if(Cn2 != 0)
        {
          for(Short_t iHarm(0); iHarm < fNumHarmonics; iHarm++)
          {
            iHarmonics = fHarmonics[iHarm];
            vector = TwoGap(iHarmonics,-iHarmonics);
            dValue = vector.Re()/Cn2;
            // printf("Gap (RFPs): %g Harm %d | Dn2: %g | fFlowVecQpos[0][0]: %g | fFlowVecQneg[0][0]: %g | fIndexCentrality %d\n\n", dEtaGap,iHarmonics,Cn2,fFlowVecQpos[0][0].Re(),fFlowVecQneg[0][0].Re(),fIndexCentrality);
            if( TMath::Abs(dValue < 1) )
              fpRefsCor2[fIndexSampling][iEtaGapIndex][iHarm]->Fill(fIndexCentrality, dValue, Cn2);

          }
        }
      }
    //estimating <4>
    //for mixed harmonics
    if(fDoOnlyMixedCorrelations){
        Dn4GapP = FourGapPos(0,0,0,0).Re();
        if(Dn4GapP != 0)
        {
                // (2,2 | 2,2)_gap , referece flow for v4/psi2
            TComplex Four_2222_GapP = FourGapPos(2, 2, -2, -2);
            double c4_2222_GapP = Four_2222_GapP.Re()/Dn4GapP;
            fpMixedRefsCor4[fIndexSampling][iEtaGapIndex][0]->Fill(fIndexCentrality, c4_2222_GapP, Dn4GapP);
            // (3,3 | 3,3)_gap, referece flow for v6/psi3
            TComplex Four_3333_GapP = FourGapPos(3, 3, -3, -3);
            double c4_3333_GapP = Four_3333_GapP.Re()/Dn4GapP;
            fpMixedRefsCor4[fIndexSampling][iEtaGapIndex][1]->Fill(fIndexCentrality, c4_3333_GapP, Dn4GapP);
            // (3,2 | 3,2)_gap, reference flow for v5/psi23
            TComplex Four_3232_GapP = FourGapPos(3, 2, -3, -2);
            double c4_3232_GapP = Four_3232_GapP.Re()/Dn4GapP;
            fpMixedRefsCor4[fIndexSampling][iEtaGapIndex][2]->Fill(fIndexCentrality, c4_3232_GapP, Dn4GapP);
        }
    }
  return;
}
//_____________________________________________________________________________
void AliAnalysisTaskFlowModes::DoFlowCharged(const Short_t iEtaGapIndex)
{
  // Estimate <2> for pT diff flow of charged tracks for all harmonics based on relevant flow vectors
  // *************************************************************

  FillPOIsVectors(iEtaGapIndex,kCharged);  // filling POIs (P,S) flow vectors

  const Double_t dPtBinWidth = (fFlowPOIsPtMax - fFlowPOIsPtMin) / fFlowPOIsPtNumBins;

  Float_t dEtaGap = fEtaGap[iEtaGapIndex];
  Short_t iHarmonics = 0;
  Double_t Dn2 = 0;
  Double_t DDn3GapP=0;
  TComplex vector = TComplex(0,0,kFALSE);
  Double_t dValue = 999;


  for(Short_t iPt(0); iPt < fFlowPOIsPtNumBins; iPt++)
  {
    if(!fDoOnlyMixedCorrelations){
          // estimating <2'>
          // POIs in positive eta
          Dn2 = TwoDiffGapPos(0,0,iPt).Re();
          if(Dn2 != 0)
          {
            for(Short_t iHarm(0); iHarm < fNumHarmonics; iHarm++)
            {
              iHarmonics = fHarmonics[iHarm];
              vector = TwoDiffGapPos(iHarmonics,-iHarmonics,iPt);
              dValue = vector.Re()/Dn2;
              if( TMath::Abs(dValue < 1) )
                fp2ChargedCor2Pos[fIndexSampling][iEtaGapIndex][iHarm]->Fill(fIndexCentrality, iPt*dPtBinWidth, dValue, Dn2);
            }
          }

          // POIs in negative eta
          Dn2 = TwoDiffGapNeg(0,0,iPt).Re();
          if(Dn2 != 0)
          {
            for(Short_t iHarm(0); iHarm < fNumHarmonics; iHarm++)
            {
              iHarmonics = fHarmonics[iHarm];
              vector = TwoDiffGapNeg(iHarmonics,-iHarmonics,iPt);
              dValue = vector.Re()/Dn2;
              if( TMath::Abs(dValue < 1) )
                fp2ChargedCor2Neg[fIndexSampling][iEtaGapIndex][iHarm]->Fill(fIndexCentrality, iPt*dPtBinWidth, dValue, Dn2);
            }
          }
    }
    if(fDoOnlyMixedCorrelations){
          // estimating <3'>
          // POIs in positive eta
          DDn3GapP = ThreeDiffGapPos(0,0,0,iPt).Re();
          if(DDn3GapP!=0)
          {
             for(Short_t iMixedHarm(0); iMixedHarm < fNumMixedHarmonics; iMixedHarm++)
         {
                 if(iMixedHarm==0){ vector = ThreeDiffGapPos(4,-2,-2,iPt);}
                 if(iMixedHarm==1){ vector = ThreeDiffGapPos(6,-3,-3,iPt);}   
                 if(iMixedHarm==2){ vector = ThreeDiffGapPos(5,-3,-2,iPt);} 
                 dValue = vector.Re()/DDn3GapP;
                 if( TMath::Abs(dValue < 1) ){
            fpMixedChargedCor3Pos[fIndexSampling][iEtaGapIndex][iMixedHarm]->Fill(fIndexCentrality,iPt*dPtBinWidth, dValue, DDn3GapP);
                 }
             }
          }     
          // POIs in negative eta
          DDn3GapP = ThreeDiffGapNeg(0,0,0,iPt).Re();
          if(DDn3GapP!=0)
          {
             for(Short_t iMixedHarm(0); iMixedHarm < fNumMixedHarmonics; iMixedHarm++)
             {
                 if(iMixedHarm==0){ vector = ThreeDiffGapNeg(4,-2,-2,iPt);}
                 if(iMixedHarm==1){ vector = ThreeDiffGapNeg(6,-3,-3,iPt);}
                 if(iMixedHarm==2){ vector = ThreeDiffGapNeg(5,-2,-3,iPt);}
                 dValue = vector.Re()/DDn3GapP;
                 if( TMath::Abs(dValue < 1) ){
                    fpMixedChargedCor3Neg[fIndexSampling][iEtaGapIndex][iMixedHarm]->Fill(fIndexCentrality,iPt*dPtBinWidth, dValue, DDn3GapP);
                 }
             }
          }
    }
  } // endfor {iPt}
  return;
}
//_____________________________________________________________________________
void AliAnalysisTaskFlowModes::DoFlowPID(const Short_t iEtaGapIndex, const PartSpecies species)
{
  // Estimate <2> for pT diff flow of pi/K/p tracks for all harmonics based on relevant flow vectors
  // *************************************************************
 
    TProfile2D** profile2Pos = 0x0;
    TProfile2D** profile2Neg = 0x0;
    TProfile2D** profile4 = 0x0;
    TProfile2D** profile3Pos = 0x0;
    TProfile2D** profile3Neg = 0x0;
  
  if(!fDoOnlyMixedCorrelations){

      switch (species)
      {
          case kPion:
              profile2Pos = fp2PionCor2Pos[fIndexSampling][iEtaGapIndex];
              profile2Neg = fp2PionCor2Neg[fIndexSampling][iEtaGapIndex];
              profile4 = fp2PionCor4[fIndexSampling];
              break;
              
          case kKaon:
              profile2Pos = fp2KaonCor2Pos[fIndexSampling][iEtaGapIndex];
              profile2Neg = fp2KaonCor2Neg[fIndexSampling][iEtaGapIndex];
              profile4 = fp2KaonCor4[fIndexSampling];
              break;
              
          case kProton:
              profile2Pos = fp2ProtonCor2Pos[fIndexSampling][iEtaGapIndex];
              profile2Neg = fp2ProtonCor2Neg[fIndexSampling][iEtaGapIndex];
              profile4 = fp2ProtonCor4[fIndexSampling];
              break;
              
          default:
              ::Error("DoFlowPID","Unexpected species! Terminating!");
              return;
      }
  }
  
  if(fDoOnlyMixedCorrelations){

    switch (species)
    {
        case kPion:
            profile3Pos = fpMixedPionCor3Pos[fIndexSampling][iEtaGapIndex];
            profile3Neg = fpMixedPionCor3Neg[fIndexSampling][iEtaGapIndex];
            break;
                
        case kKaon:
            profile3Pos = fpMixedKaonCor3Pos[fIndexSampling][iEtaGapIndex];
            profile3Neg = fpMixedKaonCor3Neg[fIndexSampling][iEtaGapIndex];
            break;
                
        case kProton:
            profile3Pos = fpMixedProtonCor3Pos[fIndexSampling][iEtaGapIndex];
            profile3Neg = fpMixedProtonCor3Neg[fIndexSampling][iEtaGapIndex];
            break;
                
        default:
            ::Error("DoFlowPID with mixed harmonics","Unexpected species! Terminating!");
            return;
    }
  }

  FillPOIsVectors(iEtaGapIndex,species); // Filling POIs vectors

  const Double_t dPtBinWidth = (fFlowPOIsPtMax - fFlowPOIsPtMin) / fFlowPOIsPtNumBins;

  Float_t dEtaGap = fEtaGap[iEtaGapIndex];
  Short_t iHarmonics = 0;
  Double_t Dn2 = 0;
  TComplex vector = TComplex(0,0,kFALSE);
  Double_t dValue = 999;
  Double_t DDn3GapP=0;

  // filling POIs (P,S) flow vectors

  for(Short_t iPt(0); iPt < fFlowPOIsPtNumBins; iPt++)
  {
    if(!fDoOnlyMixedCorrelations){
        // estimating <2'>
        // POIs in positive eta
        Dn2 = TwoDiffGapPos(0,0,iPt).Re();
        if(Dn2 != 0)
        {
            for(Short_t iHarm(0); iHarm < fNumHarmonics; iHarm++)
            {
                iHarmonics = fHarmonics[iHarm];
                vector = TwoDiffGapPos(iHarmonics,-iHarmonics,iPt);
                dValue = vector.Re()/Dn2;
                if( TMath::Abs(dValue < 1) )
                    profile2Pos[iHarm]->Fill(fIndexCentrality, iPt*dPtBinWidth, dValue, Dn2);
            }
        }
        // POIs in negative eta
        Dn2 = TwoDiffGapNeg(0,0,iPt).Re();
        if(Dn2 != 0)
        {
            for(Short_t iHarm(0); iHarm < fNumHarmonics; iHarm++)
            {
                iHarmonics = fHarmonics[iHarm];
                vector = TwoDiffGapNeg(iHarmonics,-iHarmonics,iPt);
                dValue = vector.Re()/Dn2;
                if( TMath::Abs(dValue < 1) )
                    profile2Neg[iHarm]->Fill(fIndexCentrality, iPt*dPtBinWidth, dValue, Dn2);
            }
        }
    }
    if(fDoOnlyMixedCorrelations){
        // estimating <3'>
        // POIs in positive eta
        DDn3GapP = ThreeDiffGapPos(0,0,0,iPt).Re();
        if(DDn3GapP!=0)
        {
            for(Short_t iMixedHarm(0); iMixedHarm < fNumMixedHarmonics; iMixedHarm++)
            {
                if(iMixedHarm==0){ vector = ThreeDiffGapPos(4,-2,-2,iPt);}
                if(iMixedHarm==1){ vector = ThreeDiffGapPos(6,-3,-3,iPt);}
                if(iMixedHarm==2){ vector = ThreeDiffGapPos(5,-3,-2,iPt);}
                dValue = vector.Re()/DDn3GapP;
                if( TMath::Abs(dValue < 1) ){
                    profile3Pos[iMixedHarm]->Fill(fIndexCentrality,iPt*dPtBinWidth, dValue, DDn3GapP);
                }
            }
        }
        // POIs in negative eta
        DDn3GapP = ThreeDiffGapNeg(0,0,0,iPt).Re();
        if(DDn3GapP!=0)
        {
            for(Short_t iMixedHarm(0); iMixedHarm < fNumMixedHarmonics; iMixedHarm++)
            {
                if(iMixedHarm==0){ vector = ThreeDiffGapNeg(4,-2,-2,iPt);}
                if(iMixedHarm==1){ vector = ThreeDiffGapNeg(6,-3,-3,iPt);}
                if(iMixedHarm==2){ vector = ThreeDiffGapNeg(5,-2,-3,iPt);}
                dValue = vector.Re()/DDn3GapP;
                if( TMath::Abs(dValue < 1) ){
                    profile3Neg[iMixedHarm]->Fill(fIndexCentrality,iPt*dPtBinWidth, dValue, DDn3GapP);
                }
            }
        }
    }
  } // endfor {iPt}
  return;
}
//_____________________________________________________________________________
void AliAnalysisTaskFlowModes::FillRefsVectors(const Short_t iEtaGapIndex)
{
  // Filling Q flow vector with RFPs
  // return kTRUE if succesfull (i.e. no error occurs), kFALSE otherwise
  // *************************************************************
  const Float_t dEtaGap = fEtaGap[iEtaGapIndex];
  TH2D* h2Weights = 0x0;
  Double_t dWeight = 1.;
  if(fFlowUseWeights)
  {
    h2Weights = fh2WeightRefs;
    if(!h2Weights) { ::Error("FillRefsVectors","Histogtram with weights not found."); return; }
  }

  // clearing output (global) flow vectors
  ResetRFPsVector(fFlowVecQpos);
  ResetRFPsVector(fFlowVecQneg);

  Double_t dQcosPos, dQcosNeg, dQsinPos, dQsinNeg;

  // Double_t dQcosPos[fFlowNumHarmonicsMax][fFlowNumWeightPowersMax] = {0};
  // Double_t dQcosNeg[fFlowNumHarmonicsMax][fFlowNumWeightPowersMax] = {0};
  // Double_t dQsinPos[fFlowNumHarmonicsMax][fFlowNumWeightPowersMax] = {0};
  // Double_t dQsinNeg[fFlowNumHarmonicsMax][fFlowNumWeightPowersMax] = {0};

  for (auto part = fVectorCharged->begin(); part != fVectorCharged->end(); part++)
  {
    // checking species of used particles (just for double checking purpose)
    if( part->species != kCharged)
    {
      ::Warning("FillRefsVectors","Unexpected part. species (%d) in selected sample (expected %d)",part->species,kCharged);
      continue;
    }

    if(fNegativelyChargedRef==kTRUE && part->charge>0) continue;
    if(fPositivelyChargedRef==kTRUE && part->charge<0) continue;

    
    // RFPs pT check
    if(fCutFlowRFPsPtMin > 0. && part->pt < fCutFlowRFPsPtMin)
      continue;

    if(fCutFlowRFPsPtMax > 0. && part->pt > fCutFlowRFPsPtMax)
      continue;

    // 0-ing variables
    dQcosPos = 0;
    dQcosNeg = 0;
    dQsinPos = 0;
    dQsinNeg = 0;

    // loading weights if needed
    if(fFlowUseWeights && h2Weights)
    {
      dWeight = h2Weights->GetBinContent(h2Weights->FindBin(part->eta,part->phi));
      if(dWeight <= 0) dWeight = 1.;
      // if(iEtaGapIndex == 0) fh3AfterWeightsRefs->Fill(part->phi,part->eta,part->pt, dWeight);
    }

    // RPF candidate passing all criteria: start filling flow vectors

    if(part->eta > dEtaGap / 2)
    {
      // RFP in positive eta acceptance
      for(Short_t iHarm(0); iHarm < fFlowNumHarmonicsMax; iHarm++){
        for(Short_t iPower(0); iPower < fFlowNumWeightPowersMax; iPower++)
        {
            dQcosPos = TMath::Power(dWeight,iPower) * TMath::Cos(iHarm * part->phi);
            dQsinPos = TMath::Power(dWeight,iPower) * TMath::Sin(iHarm * part->phi);
            fFlowVecQpos[iHarm][iPower] += TComplex(dQcosPos,dQsinPos,kFALSE);
        }
      }
    }
    if(part->eta < -dEtaGap / 2 )
    {
      // RFP in negative eta acceptance
      for(Short_t iHarm(0); iHarm < fFlowNumHarmonicsMax; iHarm++){
        for(Short_t iPower(0); iPower < fFlowNumWeightPowersMax; iPower++)
        {
          dQcosNeg = TMath::Power(dWeight,iPower) * TMath::Cos(iHarm * part->phi);
          dQsinNeg = TMath::Power(dWeight,iPower) * TMath::Sin(iHarm * part->phi);
          fFlowVecQneg[iHarm][iPower] += TComplex(dQcosNeg,dQsinNeg,kFALSE);
        }
      }
    }
  } // endfor {tracks} particle loop

  // // filling local flow vectors to global flow vector arrays
  // for(Short_t iHarm(0); iHarm < fFlowNumHarmonicsMax; iHarm++)
  //   for(Short_t iPower(0); iPower < fFlowNumWeightPowersMax; iPower++)
  //   {
  //     fFlowVecQpos[iHarm][iPower] = TComplex(dQcosPos[iHarm][iPower],dQsinPos[iHarm][iPower],kFALSE);
  //     if(dEtaGap > -1)
  //       fFlowVecQneg[iHarm][iPower] = TComplex(dQcosNeg[iHarm][iPower],dQsinNeg[iHarm][iPower],kFALSE);
  //   }

  // printf("RFPs EtaGap %g : number %g (pos) %g (neg) \n", dEtaGap,fFlowVecQpos[0][0].Re(),fFlowVecQneg[0][0].Re());
  return;
}
//_____________________________________________________________________________
void AliAnalysisTaskFlowModes::FillPOIsVectors(const Short_t iEtaGapIndex, const PartSpecies species, const Short_t iMassIndex)
{
  // Filling p,q and s flow vectors with POIs (given by species) for differential flow calculation
  // *************************************************************

  if(species == kUnknown) return;

  Double_t dWeight = 1.;  // for generic framework != 1

  // clearing output (global) flow vectors
  ResetPOIsVector(fFlowVecPpos);
  ResetPOIsVector(fFlowVecPneg);
  ResetPOIsVector(fFlowVecS);

  std::vector<FlowPart>* vector = 0x0;
  TH3D* hist = 0x0;
  Double_t dMassLow = 0, dMassHigh = 0;
  TH2D* h2Weights = 0x0;

  // swich based on species
  switch (species)
  {
    case kCharged:
      vector = fVectorCharged;
      if(fFlowUseWeights) { h2Weights = fh2WeightCharged; }
      break;

    case kPion:
      vector = fVectorPion;
      if(fFlowUseWeights) { h2Weights = fh2WeightPion; }
      break;

    case kKaon:
      vector = fVectorKaon;
      if(fFlowUseWeights) { h2Weights = fh2WeightKaon; }

      break;

    case kProton:
      vector = fVectorProton;
      if(fFlowUseWeights) { h2Weights = fh2WeightProton; }
      break;

    default:
      ::Error("FillPOIsVectors","Selected species unknown.");
      return;
  }//switch(species)

  if(fFlowUseWeights && !h2Weights) { ::Error("FillPOIsVectors","Histogtram with weights not found."); return; }

  const Double_t dEtaGap = fEtaGap[iEtaGapIndex];
  const Double_t dMass = (dMassLow+dMassHigh)/2;

  Short_t iPtBin = 0;
  Double_t dCos = 0, dSin = 0;

  for (auto part = vector->begin(); part != vector->end(); part++)
  {
    // checking species of used particles (just for double checking purpose)
    if( part->species != species)
    {
      ::Warning("FillPOIsVectors","Unexpected part. species (%d) in selected sample (expected %d)",part->species,species);
      continue;
    }//endif{part->species != species}

    // assign iPtBin based on particle momenta
    iPtBin = GetPOIsPtBinIndex(part->pt);

    // 0-ing variables
    dCos = 0;
    dSin = 0;

    // POIs candidate passing all criteria: start filling flow vectors

    // loading weights if needed
    if(fFlowUseWeights && h2Weights)
    {
      dWeight = h2Weights->GetBinContent(h2Weights->FindBin(part->eta,part->phi));
      if(dWeight <= 0) dWeight = 1.;
    }//endif{fFlowUseWeights && h2Weights}

    if(part->eta > dEtaGap / 2 )
    {
        // particle in positive eta acceptance
        for(Short_t iHarm(0); iHarm < fFlowNumHarmonicsMax; iHarm++){
          for(Short_t iPower(0); iPower < fFlowNumWeightPowersMax; iPower++)
          {
            dCos = TMath::Power(dWeight,iPower) * TMath::Cos(iHarm * part->phi);
            dSin = TMath::Power(dWeight,iPower) * TMath::Sin(iHarm * part->phi);
            fFlowVecPpos[iHarm][iPower][iPtBin] += TComplex(dCos,dSin,kFALSE);
          }//endfor{Short_t iPower(0); iPower < fFlowNumWeightPowersMax; iPower++}
       }//endfor{Short_t iHarm(0); iHarm < fFlowNumHarmonicsMax; iHarm++}
    }//endif{part->eta > dEtaGap / 2 }   
    if(part->eta < -dEtaGap / 2 ){
         // particle in negative eta acceptance
         for(Short_t iHarm(0); iHarm < fFlowNumHarmonicsMax; iHarm++){
           for(Short_t iPower(0); iPower < fFlowNumWeightPowersMax; iPower++)
           {
             dCos = TMath::Power(dWeight,iPower) * TMath::Cos(iHarm * part->phi);
             dSin = TMath::Power(dWeight,iPower) * TMath::Sin(iHarm * part->phi);
             fFlowVecPneg[iHarm][iPower][iPtBin] += TComplex(dCos,dSin,kFALSE);
           }//endfor{Short_t iPower(0); iPower < fFlowNumWeightPowersMax; iPower++}
         }//endfor{Short_t iHarm(0); iHarm < fFlowNumHarmonicsMax; iHarm++}
     }//endif{part->eta < -dEtaGap / 2 }
   } // endfor {tracks}
   return;
}
//_____________________________________________________________________________
Short_t AliAnalysisTaskFlowModes::GetPOIsPtBinIndex(const Double_t pt)
{
  // Return POIs pT bin index based on pT value
  // *************************************************************
  const Double_t dPtBinWidth = (fFlowPOIsPtMax - fFlowPOIsPtMin) / fFlowPOIsPtNumBins;
  // printf("Pt %g | index %d\n",pt,(Short_t) (pt / dPtBinWidth) );
  return (Short_t) (pt / dPtBinWidth);
}
//_____________________________________________________________________________
void AliAnalysisTaskFlowModes::ResetRFPsVector(TComplex (&array)[fFlowNumHarmonicsMax][fFlowNumWeightPowersMax])
{
  // Reset RFPs (Q) array values to TComplex(0,0,kFALSE) for given array
  // *************************************************************
  for(Short_t iHarm(0); iHarm < fFlowNumHarmonicsMax; iHarm++)
    for(Short_t iPower(0); iPower < fFlowNumWeightPowersMax; iPower++)
      array[iHarm][iPower] = TComplex(0,0,kFALSE);
  return;
}
//_____________________________________________________________________________
void AliAnalysisTaskFlowModes::ResetPOIsVector(TComplex (&array)[fFlowNumHarmonicsMax][fFlowNumWeightPowersMax][fFlowPOIsPtNumBins])
{
  for(Short_t iHarm(0); iHarm < fFlowNumHarmonicsMax; iHarm++)
    for(Short_t iPower(0); iPower < fFlowNumWeightPowersMax; iPower++)
      for(Short_t iPt(0); iPt < fFlowPOIsPtNumBins; iPt++)
        array[iHarm][iPower][iPt] = TComplex(0,0,kFALSE);
  return;
}
//_____________________________________________________________________________
void AliAnalysisTaskFlowModes::ListFlowVector(TComplex (&array)[fFlowNumHarmonicsMax][fFlowNumWeightPowersMax])
{
  // List all values of given flow vector TComplex array
  // *************************************************************
  printf(" ### Listing (TComplex) flow vector array ###########################\n");
  for(Short_t iHarm(0); iHarm < fFlowNumHarmonicsMax; iHarm++)
  {
    printf("Harm %d (power):",iHarm);
    for(Short_t iPower(0); iPower < fFlowNumWeightPowersMax; iPower++)
    {
        printf("|(%d) %g+%g(i)",iPower, array[iHarm][iPower].Re(), array[iHarm][iPower].Im());
    }
    printf("\n");
  }
  return;
}
//_____________________________________________________________________________
Short_t AliAnalysisTaskFlowModes::GetSamplingIndex()
{
  // Assessing sampling index based on generated random number
  // returns centrality index
  // *************************************************************

  Short_t index = 0x0;

  if(fSampling && fNumSamples > 1)
  {
    TRandom3 rr(0);
    Double_t ranNum = rr.Rndm(); // getting random number in (0,1)
    Double_t generated = ranNum * fNumSamples; // getting random number in range (0, fNumSamples)

    // finding right index for sampling based on generated number and total number of samples
    for(Short_t i(0); i < fNumSamples; i++)
    {
      if(generated < (i+1) )
      {
        index = i;
        break;
      }
    }
  }

  return index;
}
//_____________________________________________________________________________
Short_t AliAnalysisTaskFlowModes::GetCentralityIndex()
{
  // Estimating centrality percentile based on selected estimator.
  // (Default) If no multiplicity estimator is specified (fMultEstimator == '' || Charged), percentile is estimated as number of selected / filtered charged tracks.
  // If a valid multiplicity estimator is specified, centrality percentile is estimated via AliMultSelection
  // otherwise -1 is returned (and event is skipped)
  // *************************************************************
  fMultEstimator.ToUpper();
  //cout<<"++++++++++++++++++GetCentralityIndex+++++++++++++++++"<<endl;
  if(
      fMultEstimator.EqualTo("V0A") || fMultEstimator.EqualTo("V0C") || fMultEstimator.EqualTo("V0M") ||
      fMultEstimator.EqualTo("CL0") || fMultEstimator.EqualTo("CL1") || fMultEstimator.EqualTo("ZNA") || 
      fMultEstimator.EqualTo("ZNC") || fMultEstimator.EqualTo("TRK") ){
   //cout<<"++++++++++++++++++CentralityIndex+++++++++++++++++"<<endl;

    // some of supported AliMultSelection estimators (listed above)
    Float_t dPercentile = 300;

    // checking AliMultSelection
    AliMultSelection* multSelection = 0x0;
    multSelection = (AliMultSelection*) fEventAOD->FindListObject("MultSelection");
    if(!multSelection) { AliError("AliMultSelection object not found! Returning -1"); return -1;}

    dPercentile = multSelection->GetMultiplicityPercentile(fMultEstimator.Data());
    //cout<<"fMultEstimator.Data()"<<fMultEstimator.Data()<<endl;
    //cout<<"dPercentile = "<<dPercentile<<endl;
    if(fFullCentralityRange){
       if(dPercentile > 100 || dPercentile < 0) { AliWarning("Centrality percentile estimated not within 0-100 range. Returning -1"); return -1;}
       else { return dPercentile;}
    }
    if(!fFullCentralityRange){
       if(dPercentile > 100 || dPercentile <50) { AliWarning("Centrality percentile estimated not within 50-100 range. Returning -1"); return -1;}
       else { return dPercentile;}
    }
  }
  else if(fMultEstimator.EqualTo("") || fMultEstimator.EqualTo("CHARGED"))
  {
    // assigning centrality based on number of selected charged tracks
    return fVectorCharged->size();
  }
  else
  {
    AliWarning(Form("Multiplicity estimator '%s' not supported. Returning -1\n",fMultEstimator.Data()));
    return -1;
  }


  return -1;
}
//____________________________
Double_t AliAnalysisTaskFlowModes::GetWDist(const AliAODVertex* v0, const AliAODVertex* v1) {
// calculate sqrt of weighted distance to other vertex
  if (!v0 || !v1) {
    printf("One of vertices is not valid\n");
    return kFALSE;
  }
  static TMatrixDSym vVb(3);
  double dist = -1;
  double dx = v0->GetX()-v1->GetX();
  double dy = v0->GetY()-v1->GetY();
  double dz = v0->GetZ()-v1->GetZ();
  double cov0[6],cov1[6];
  v0->GetCovarianceMatrix(cov0);
  v1->GetCovarianceMatrix(cov1);
  vVb(0,0) = cov0[0]+cov1[0];
  vVb(1,1) = cov0[2]+cov1[2];
  vVb(2,2) = cov0[5]+cov1[5];
  vVb(1,0) = vVb(0,1) = cov0[1]+cov1[1];
  vVb(0,2) = vVb(1,2) = vVb(2,0) = vVb(2,1) = 0.;
  vVb.InvertFast();
  if (!vVb.IsValid()) {printf("Singular Matrix\n"); return dist;}
  dist = vVb(0,0)*dx*dx + vVb(1,1)*dy*dy + vVb(2,2)*dz*dz+2*vVb(0,1)*dx*dy + 2*vVb(0,2)*dx*dz + 2*vVb(1,2)*dy*dz;
  return dist>0 ? TMath::Sqrt(dist) : -1;
}
//_________________________________________________
void AliAnalysisTaskFlowModes::Terminate(Option_t* option)
{
  // called on end of task, after all events are processed
  // *************************************************************

  return;
}
//_____________________________________________________________________________
// Set of methods returning given complex flow vector based on flow harmonics (n) and weight power indexes (p)
// a la General Framework implementation.
// Q: flow vector of RFPs (with/out eta gap)
// P: flow vector of POIs (with/out eta gap) (in usual notation p)
// S: flow vector of overlaping RFPs and POIs (in usual notation q)

TComplex AliAnalysisTaskFlowModes::Q(const Short_t n, const Short_t p)
{
  if (n < 0) return TComplex::Conjugate(fFlowVecQpos[-n][p]);
  else return fFlowVecQpos[n][p];
}
//____________________________________________________________________
TComplex AliAnalysisTaskFlowModes::QGapPos(const Short_t n, const Short_t p)
{
  if (n < 0) return TComplex::Conjugate(fFlowVecQpos[-n][p]);
  else return fFlowVecQpos[n][p];
}
//____________________________________________________________________
TComplex AliAnalysisTaskFlowModes::QGapNeg(const Short_t n, const Short_t p)
{
  if(n < 0) return TComplex::Conjugate(fFlowVecQneg[-n][p]);
  else return fFlowVecQneg[n][p];
}
//____________________________________________________________________
TComplex AliAnalysisTaskFlowModes::P(const Short_t n, const Short_t p, const Short_t pt)
{
  if(n < 0) return TComplex::Conjugate(fFlowVecPpos[-n][p][pt]);
  else return fFlowVecPpos[n][p][pt];
}
//____________________________________________________________________
TComplex AliAnalysisTaskFlowModes::PGapPos(const Short_t n, const Short_t p, const Short_t pt)
{
  if(n < 0) return TComplex::Conjugate(fFlowVecPpos[-n][p][pt]);
  else return fFlowVecPpos[n][p][pt];
}
//____________________________________________________________________
TComplex AliAnalysisTaskFlowModes::PGapNeg(const Short_t n, const Short_t p, const Short_t pt)
{
  if(n < 0) return TComplex::Conjugate(fFlowVecPneg[-n][p][pt]);
  else return fFlowVecPneg[n][p][pt];
}
//____________________________________________________________________
TComplex AliAnalysisTaskFlowModes::S(const Short_t n, const Short_t p, const Short_t pt)
{
  if(n < 0) return TComplex::Conjugate(fFlowVecS[-n][p][pt]);
  else return fFlowVecS[n][p][pt];
}
//____________________________________________________________________

// Set of flow calculation methods for cumulants of different orders with/out eta gap

TComplex AliAnalysisTaskFlowModes::Two(const Short_t n1, const Short_t n2)
{
  TComplex formula = Q(n1,1)*Q(n2,1) - Q(n1+n2,2);
  return formula;
}
//____________________________________________________________________
TComplex AliAnalysisTaskFlowModes::TwoGap(const Short_t n1, const Short_t n2)
{
  TComplex formula = QGapPos(n1,1)*QGapNeg(n2,1);
  return formula;
}
//____________________________________________________________________
TComplex AliAnalysisTaskFlowModes::TwoDiff(const Short_t n1, const Short_t n2, const Short_t pt)
{
  TComplex formula = P(n1,1,pt)*Q(n2,1) - S(n1+n2,1,pt);
  return formula;
}
//____________________________________________________________________
TComplex AliAnalysisTaskFlowModes::TwoDiffGapPos(const Short_t n1, const Short_t n2, const Short_t pt)
{
  TComplex formula = PGapPos(n1,1,pt)*QGapNeg(n2,1);
  return formula;
}
//____________________________________________________________________
TComplex AliAnalysisTaskFlowModes::TwoDiffGapNeg(const Short_t n1, const Short_t n2, const Short_t pt)
{
  TComplex formula = PGapNeg(n1,1,pt)*QGapPos(n2,1);
  return formula;
}
//____________________________________________________________________
TComplex AliAnalysisTaskFlowModes::ThreeDiffGapPos(const Short_t n1, const Short_t n2, const Short_t n3,const Short_t pt)
{
  TComplex formula = PGapPos(n1,1,pt)*QGapNeg(n2,1)*QGapNeg(n3,1)- PGapPos(n1,1,pt)*QGapNeg(n2+n3,2);
  return formula;
}
//____________________________________________________________________
TComplex AliAnalysisTaskFlowModes::ThreeDiffGapNeg(const Short_t n1, const Short_t n2, const Short_t n3,const Short_t pt)
{
  TComplex formula = PGapNeg(n1,1,pt)*QGapPos(n2,1)*QGapPos(n3,1)- PGapNeg(n1,1,pt)*QGapPos(n2+n3,2);
  return formula;
}
//____________________________________________________________________
TComplex AliAnalysisTaskFlowModes::Four(const Short_t n1, const Short_t n2, const Short_t n3, const Short_t n4)
{
  TComplex formula = Q(n1,1)*Q(n2,1)*Q(n3,1)*Q(n4,1)-Q(n1+n2,2)*Q(n3,1)*Q(n4,1)-Q(n2,1)*Q(n1+n3,2)*Q(n4,1)
                    - Q(n1,1)*Q(n2+n3,2)*Q(n4,1)+2.*Q(n1+n2+n3,3)*Q(n4,1)-Q(n2,1)*Q(n3,1)*Q(n1+n4,2)
                    + Q(n2+n3,2)*Q(n1+n4,2)-Q(n1,1)*Q(n3,1)*Q(n2+n4,2)+Q(n1+n3,2)*Q(n2+n4,2)
                    + 2.*Q(n3,1)*Q(n1+n2+n4,3)-Q(n1,1)*Q(n2,1)*Q(n3+n4,2)+Q(n1+n2,2)*Q(n3+n4,2)
                    + 2.*Q(n2,1)*Q(n1+n3+n4,3)+2.*Q(n1,1)*Q(n2+n3+n4,3)-6.*Q(n1+n2+n3+n4,4);
  return formula;
}
// //____________________________________________________________________
TComplex AliAnalysisTaskFlowModes::FourDiff(const Short_t n1, const Short_t n2, const Short_t n3, const Short_t n4, const Short_t pt)
{
  TComplex formula = P(n1,1,pt)*Q(n2,1)*Q(n3,1)*Q(n4,1)-S(n1+n2,2,pt)*Q(n3,1)*Q(n4,1)-Q(n2,1)*S(n1+n3,2,pt)*Q(n4,1)
                    - P(n1,1,pt)*Q(n2+n3,2)*Q(n4,1)+2.*S(n1+n2+n3,3,pt)*Q(n4,1)-Q(n2,1)*Q(n3,1)*S(n1+n4,2,pt)
                    + Q(n2+n3,2)*S(n1+n4,2,pt)-P(n1,1,pt)*Q(n3,1)*Q(n2+n4,2)+S(n1+n3,2,pt)*Q(n2+n4,2)
                    + 2.*Q(n3,1)*S(n1+n2+n4,3,pt)-P(n1,1,pt)*Q(n2,1)*Q(n3+n4,2)+S(n1+n2,2,pt)*Q(n3+n4,2)
                    + 2.*Q(n2,1)*S(n1+n3+n4,3,pt)+2.*P(n1,1,pt)*Q(n2+n3+n4,3)-6.*S(n1+n2+n3+n4,4,pt);
    return formula;
}
//____________________________________________________________________
TComplex AliAnalysisTaskFlowModes::FourGapPos(const Short_t n1, const Short_t n2, const Short_t n3, const Short_t n4) //  n1+n2 = n3+n4;  n1, n2 from P & n3, n4 from M
{
    TComplex formula = QGapPos(n1,1)*QGapPos(n2,1)*QGapNeg(n3,1)*QGapNeg(n4,1)-QGapPos(n1+n2,2)*QGapNeg(n3,1)*QGapNeg(n4,1)-QGapPos(n1,1)*QGapPos(n2,1)*QGapNeg(n3+n4,2)+QGapPos(n1+n2,2)*QGapNeg(n3+n4,2);
    //TComplex *out = (TComplex*) &formula;
    return formula;
}
//____________________________________________________________________
TComplex AliAnalysisTaskFlowModes::FourGapNeg(const Short_t n1, const Short_t n2, const Short_t n3, const Short_t n4) //  n1+n2 = n3+n4;  n1, n2 from M & n3, n4 from P
{
     TComplex formula = QGapNeg(n1,1)*QGapNeg(n2,1)*QGapPos(n3,1)*QGapPos(n4,1)-QGapNeg(n1+n2,2)*QGapPos(n3,1)*QGapPos(n4,1)-QGapNeg(n1,1)*QGapNeg(n2,1)*QGapPos(n3+n4,2)+QGapNeg(n1+n2,2)*QGapPos(n3+n4,2);
     //TComplex *out = (TComplex*) &formula;
     return formula;
}

//-----------------------------------------------------------------------
void AliAnalysisTaskFlowModes::SetPriors(Float_t centrCur){
    //set priors for the bayesian pid selection
    fCurrCentr = centrCur;
    
    fBinLimitPID[0] = 0.300000;
    fBinLimitPID[1] = 0.400000;
    fBinLimitPID[2] = 0.500000;
    fBinLimitPID[3] = 0.600000;
    fBinLimitPID[4] = 0.700000;
    fBinLimitPID[5] = 0.800000;
    fBinLimitPID[6] = 0.900000;
    fBinLimitPID[7] = 1.000000;
    fBinLimitPID[8] = 1.200000;
    fBinLimitPID[9] = 1.400000;
    fBinLimitPID[10] = 1.600000;
    fBinLimitPID[11] = 1.800000;
    fBinLimitPID[12] = 2.000000;
    fBinLimitPID[13] = 2.200000;
    fBinLimitPID[14] = 2.400000;
    fBinLimitPID[15] = 2.600000;
    fBinLimitPID[16] = 2.800000;
    fBinLimitPID[17] = 3.000000;
    
    // 0-10%
    if(centrCur < 10){
        fC[0][0] = 0.005;
        fC[0][1] = 0.005;
        fC[0][2] = 1.0000;
        fC[0][3] = 0.0010;
        fC[0][4] = 0.0010;
        
        fC[1][0] = 0.005;
        fC[1][1] = 0.005;
        fC[1][2] = 1.0000;
        fC[1][3] = 0.0168;
        fC[1][4] = 0.0122;
        
        fC[2][0] = 0.005;
        fC[2][1] = 0.005;
        fC[2][2] = 1.0000;
        fC[2][3] = 0.0272;
        fC[2][4] = 0.0070;
        
        fC[3][0] = 0.005;
        fC[3][1] = 0.005;
        fC[3][2] = 1.0000;
        fC[3][3] = 0.0562;
        fC[3][4] = 0.0258;
        
        fC[4][0] = 0.005;
        fC[4][1] = 0.005;
        fC[4][2] = 1.0000;
        fC[4][3] = 0.0861;
        fC[4][4] = 0.0496;
        
        fC[5][0] = 0.005;
        fC[5][1] = 0.005;
        fC[5][2] = 1.0000;
        fC[5][3] = 0.1168;
        fC[5][4] = 0.0740;
        
        fC[6][0] = 0.005;
        fC[6][1] = 0.005;
        fC[6][2] = 1.0000;
        fC[6][3] = 0.1476;
        fC[6][4] = 0.0998;
        
        fC[7][0] = 0.005;
        fC[7][1] = 0.005;
        fC[7][2] = 1.0000;
        fC[7][3] = 0.1810;
        fC[7][4] = 0.1296;
        
        fC[8][0] = 0.005;
        fC[8][1] = 0.005;
        fC[8][2] = 1.0000;
        fC[8][3] = 0.2240;
        fC[8][4] = 0.1827;
        
        fC[9][0] = 0.005;
        fC[9][1] = 0.005;
        fC[9][2] = 1.0000;
        fC[9][3] = 0.2812;
        fC[9][4] = 0.2699;
        
        fC[10][0] = 0.005;
        fC[10][1] = 0.005;
        fC[10][2] = 1.0000;
        fC[10][3] = 0.3328;
        fC[10][4] = 0.3714;
        
        fC[11][0] = 0.005;
        fC[11][1] = 0.005;
        fC[11][2] = 1.0000;
        fC[11][3] = 0.3780;
        fC[11][4] = 0.4810;
        
        fC[12][0] = 0.005;
        fC[12][1] = 0.005;
        fC[12][2] = 1.0000;
        fC[12][3] = 0.4125;
        fC[12][4] = 0.5771;
        
        fC[13][0] = 0.005;
        fC[13][1] = 0.005;
        fC[13][2] = 1.0000;
        fC[13][3] = 0.4486;
        fC[13][4] = 0.6799;
        
        fC[14][0] = 0.005;
        fC[14][1] = 0.005;
        fC[14][2] = 1.0000;
        fC[14][3] = 0.4840;
        fC[14][4] = 0.7668;
        
        fC[15][0] = 0.005;
        fC[15][1] = 0.005;
        fC[15][2] = 1.0000;
        fC[15][3] = 0.4971;
        fC[15][4] = 0.8288;
        
        fC[16][0] = 0.005;
        fC[16][1] = 0.005;
        fC[16][2] = 1.0000;
        fC[16][3] = 0.4956;
        fC[16][4] = 0.8653;
        
        fC[17][0] = 0.005;
        fC[17][1] = 0.005;
        fC[17][2] = 1.0000;
        fC[17][3] = 0.5173;
        fC[17][4] = 0.9059;   
    }
    // 10-20%
    else if(centrCur < 20){
        fC[0][0] = 0.005;
        fC[0][1] = 0.005;
        fC[0][2] = 1.0000;
        fC[0][3] = 0.0010;
        fC[0][4] = 0.0010;
        
        fC[1][0] = 0.005;
        fC[1][1] = 0.005;
        fC[1][2] = 1.0000;
        fC[1][3] = 0.0132;
        fC[1][4] = 0.0088;
        
        fC[2][0] = 0.005;
        fC[2][1] = 0.005;
        fC[2][2] = 1.0000;
        fC[2][3] = 0.0283;
        fC[2][4] = 0.0068;
        
        fC[3][0] = 0.005;
        fC[3][1] = 0.005;
        fC[3][2] = 1.0000;
        fC[3][3] = 0.0577;
        fC[3][4] = 0.0279;
        
        fC[4][0] = 0.005;
        fC[4][1] = 0.005;
        fC[4][2] = 1.0000;
        fC[4][3] = 0.0884;
        fC[4][4] = 0.0534;
        
        fC[5][0] = 0.005;
        fC[5][1] = 0.005;
        fC[5][2] = 1.0000;
        fC[5][3] = 0.1179;
        fC[5][4] = 0.0794;
        
        fC[6][0] = 0.005;
        fC[6][1] = 0.005;
        fC[6][2] = 1.0000;
        fC[6][3] = 0.1480;
        fC[6][4] = 0.1058;
        
        fC[7][0] = 0.005;
        fC[7][1] = 0.005;
        fC[7][2] = 1.0000;
        fC[7][3] = 0.1807;
        fC[7][4] = 0.1366;
        
        fC[8][0] = 0.005;
        fC[8][1] = 0.005;
        fC[8][2] = 1.0000;
        fC[8][3] = 0.2219;
        fC[8][4] = 0.1891;
        
        fC[9][0] = 0.005;
        fC[9][1] = 0.005;
        fC[9][2] = 1.0000;
        fC[9][3] = 0.2804;
        fC[9][4] = 0.2730;
        
        fC[10][0] = 0.005;
        fC[10][1] = 0.005;
        fC[10][2] = 1.0000;
        fC[10][3] = 0.3283;
        fC[10][4] = 0.3660;
        
        fC[11][0] = 0.005;
        fC[11][1] = 0.005;
        fC[11][2] = 1.0000;
        fC[11][3] = 0.3710;
        fC[11][4] = 0.4647;
        
        fC[12][0] = 0.005;
        fC[12][1] = 0.005;
        fC[12][2] = 1.0000;
        fC[12][3] = 0.4093;
        fC[12][4] = 0.5566;
        
        fC[13][0] = 0.005;
        fC[13][1] = 0.005;
        fC[13][2] = 1.0000;
        fC[13][3] = 0.4302;
        fC[13][4] = 0.6410;
        
        fC[14][0] = 0.005;
        fC[14][1] = 0.005;
        fC[14][2] = 1.0000;
        fC[14][3] = 0.4649;
        fC[14][4] = 0.7055;
        
        fC[15][0] = 0.005;
        fC[15][1] = 0.005;
        fC[15][2] = 1.0000;
        fC[15][3] = 0.4523;
        fC[15][4] = 0.7440;
        
        fC[16][0] = 0.005;
        fC[16][1] = 0.005;
        fC[16][2] = 1.0000;
        fC[16][3] = 0.4591;
        fC[16][4] = 0.7799;
        
        fC[17][0] = 0.005;
        fC[17][1] = 0.005;
        fC[17][2] = 1.0000;
        fC[17][3] = 0.4804;
        fC[17][4] = 0.8218;
    }
    // 20-30%
    else if(centrCur < 30){
        fC[0][0] = 0.005;
        fC[0][1] = 0.005;
        fC[0][2] = 1.0000;
        fC[0][3] = 0.0010;
        fC[0][4] = 0.0010;
        
        fC[1][0] = 0.005;
        fC[1][1] = 0.005;
        fC[1][2] = 1.0000;
        fC[1][3] = 0.0102;
        fC[1][4] = 0.0064;
        
        fC[2][0] = 0.005;
        fC[2][1] = 0.005;
        fC[2][2] = 1.0000;
        fC[2][3] = 0.0292;
        fC[2][4] = 0.0066;
        
        fC[3][0] = 0.005;
        fC[3][1] = 0.005;
        fC[3][2] = 1.0000;
        fC[3][3] = 0.0597;
        fC[3][4] = 0.0296;
        
        fC[4][0] = 0.005;
        fC[4][1] = 0.005;
        fC[4][2] = 1.0000;
        fC[4][3] = 0.0900;
        fC[4][4] = 0.0589;
        
        fC[5][0] = 0.005;
        fC[5][1] = 0.005;
        fC[5][2] = 1.0000;
        fC[5][3] = 0.1199;
        fC[5][4] = 0.0859;
        
        fC[6][0] = 0.005;
        fC[6][1] = 0.005;
        fC[6][2] = 1.0000;
        fC[6][3] = 0.1505;
        fC[6][4] = 0.1141;
        
        fC[7][0] = 0.005;
        fC[7][1] = 0.005;
        fC[7][2] = 1.0000;
        fC[7][3] = 0.1805;
        fC[7][4] = 0.1454;
        
        fC[8][0] = 0.005;
        fC[8][1] = 0.005;
        fC[8][2] = 1.0000;
        fC[8][3] = 0.2221;
        fC[8][4] = 0.2004;
        
        fC[9][0] = 0.005;
        fC[9][1] = 0.005;
        fC[9][2] = 1.0000;
        fC[9][3] = 0.2796;
        fC[9][4] = 0.2838;
        
        fC[10][0] = 0.005;
        fC[10][1] = 0.005;
        fC[10][2] = 1.0000;
        fC[10][3] = 0.3271;
        fC[10][4] = 0.3682;
        
        fC[11][0] = 0.005;
        fC[11][1] = 0.005;
        fC[11][2] = 1.0000;
        fC[11][3] = 0.3648;
        fC[11][4] = 0.4509;
        
        fC[12][0] = 0.005;
        fC[12][1] = 0.005;
        fC[12][2] = 1.0000;
        fC[12][3] = 0.3988;
        fC[12][4] = 0.5339;
        
        fC[13][0] = 0.005;
        fC[13][1] = 0.005;
        fC[13][2] = 1.0000;
        fC[13][3] = 0.4315;
        fC[13][4] = 0.5995;
        
        fC[14][0] = 0.005;
        fC[14][1] = 0.005;
        fC[14][2] = 1.0000;
        fC[14][3] = 0.4548;
        fC[14][4] = 0.6612;
        
        fC[15][0] = 0.005;
        fC[15][1] = 0.005;
        fC[15][2] = 1.0000;
        fC[15][3] = 0.4744;
        fC[15][4] = 0.7060;
        
        fC[16][0] = 0.005;
        fC[16][1] = 0.005;
        fC[16][2] = 1.0000;
        fC[16][3] = 0.4899;
        fC[16][4] = 0.7388;
        
        fC[17][0] = 0.005;
        fC[17][1] = 0.005;
        fC[17][2] = 1.0000;
        fC[17][3] = 0.4411;
        fC[17][4] = 0.7293;
    }
    // 30-40%
    else if(centrCur < 40){
        fC[0][0] = 0.005;
        fC[0][1] = 0.005;
        fC[0][2] = 1.0000;
        fC[0][3] = 0.0010;
        fC[0][4] = 0.0010;
        
        fC[1][0] = 0.005;
        fC[1][1] = 0.005;
        fC[1][2] = 1.0000;
        fC[1][3] = 0.0102;
        fC[1][4] = 0.0048;
        
        fC[2][0] = 0.005;
        fC[2][1] = 0.005;
        fC[2][2] = 1.0000;
        fC[2][3] = 0.0306;
        fC[2][4] = 0.0079;
        
        fC[3][0] = 0.005;
        fC[3][1] = 0.005;
        fC[3][2] = 1.0000;
        fC[3][3] = 0.0617;
        fC[3][4] = 0.0338;
        
        fC[4][0] = 0.005;
        fC[4][1] = 0.005;
        fC[4][2] = 1.0000;
        fC[4][3] = 0.0920;
        fC[4][4] = 0.0652;
        
        fC[5][0] = 0.005;
        fC[5][1] = 0.005;
        fC[5][2] = 1.0000;
        fC[5][3] = 0.1211;
        fC[5][4] = 0.0955;
        
        fC[6][0] = 0.005;
        fC[6][1] = 0.005;
        fC[6][2] = 1.0000;
        fC[6][3] = 0.1496;
        fC[6][4] = 0.1242;
        
        fC[7][0] = 0.005;
        fC[7][1] = 0.005;
        fC[7][2] = 1.0000;
        fC[7][3] = 0.1807;
        fC[7][4] = 0.1576;
        
        fC[8][0] = 0.005;
        fC[8][1] = 0.005;
        fC[8][2] = 1.0000;
        fC[8][3] = 0.2195;
        fC[8][4] = 0.2097;
        
        fC[9][0] = 0.005;
        fC[9][1] = 0.005;
        fC[9][2] = 1.0000;
        fC[9][3] = 0.2732;
        fC[9][4] = 0.2884;
        
        fC[10][0] = 0.005;
        fC[10][1] = 0.005;
        fC[10][2] = 1.0000;
        fC[10][3] = 0.3204;
        fC[10][4] = 0.3679;
        
        fC[11][0] = 0.005;
        fC[11][1] = 0.005;
        fC[11][2] = 1.0000;
        fC[11][3] = 0.3564;
        fC[11][4] = 0.4449;
        
        fC[12][0] = 0.005;
        fC[12][1] = 0.005;
        fC[12][2] = 1.0000;
        fC[12][3] = 0.3791;
        fC[12][4] = 0.5052;
        
        fC[13][0] = 0.005;
        fC[13][1] = 0.005;
        fC[13][2] = 1.0000;
        fC[13][3] = 0.4062;
        fC[13][4] = 0.5647;
        
        fC[14][0] = 0.005;
        fC[14][1] = 0.005;
        fC[14][2] = 1.0000;
        fC[14][3] = 0.4234;
        fC[14][4] = 0.6203;
        
        fC[15][0] = 0.005;
        fC[15][1] = 0.005;
        fC[15][2] = 1.0000;
        fC[15][3] = 0.4441;
        fC[15][4] = 0.6381;
        
        fC[16][0] = 0.005;
        fC[16][1] = 0.005;
        fC[16][2] = 1.0000;
        fC[16][3] = 0.4629;
        fC[16][4] = 0.6496;
        
        fC[17][0] = 0.005;
        fC[17][1] = 0.005;
        fC[17][2] = 1.0000;
        fC[17][3] = 0.4293;
        fC[17][4] = 0.6491;
    }
    // 40-50%
    else if(centrCur < 50){
        fC[0][0] = 0.005;
        fC[0][1] = 0.005;
        fC[0][2] = 1.0000;
        fC[0][3] = 0.0010;
        fC[0][4] = 0.0010;
        
        fC[1][0] = 0.005;
        fC[1][1] = 0.005;
        fC[1][2] = 1.0000;
        fC[1][3] = 0.0093;
        fC[1][4] = 0.0057;
        
        fC[2][0] = 0.005;
        fC[2][1] = 0.005;
        fC[2][2] = 1.0000;
        fC[2][3] = 0.0319;
        fC[2][4] = 0.0075;
        
        fC[3][0] = 0.005;
        fC[3][1] = 0.005;
        fC[3][2] = 1.0000;
        fC[3][3] = 0.0639;
        fC[3][4] = 0.0371;
        
        fC[4][0] = 0.005;
        fC[4][1] = 0.005;
        fC[4][2] = 1.0000;
        fC[4][3] = 0.0939;
        fC[4][4] = 0.0725;
        
        fC[5][0] = 0.005;
        fC[5][1] = 0.005;
        fC[5][2] = 1.0000;
        fC[5][3] = 0.1224;
        fC[5][4] = 0.1045;
        
        fC[6][0] = 0.005;
        fC[6][1] = 0.005;
        fC[6][2] = 1.0000;
        fC[6][3] = 0.1520;
        fC[6][4] = 0.1387;
        
        fC[7][0] = 0.005;
        fC[7][1] = 0.005;
        fC[7][2] = 1.0000;
        fC[7][3] = 0.1783;
        fC[7][4] = 0.1711;
        
        fC[8][0] = 0.005;
        fC[8][1] = 0.005;
        fC[8][2] = 1.0000;
        fC[8][3] = 0.2202;
        fC[8][4] = 0.2269;
        
        fC[9][0] = 0.005;
        fC[9][1] = 0.005;
        fC[9][2] = 1.0000;
        fC[9][3] = 0.2672;
        fC[9][4] = 0.2955;
        
        fC[10][0] = 0.005;
        fC[10][1] = 0.005;
        fC[10][2] = 1.0000;
        fC[10][3] = 0.3191;
        fC[10][4] = 0.3676;
        
        fC[11][0] = 0.005;
        fC[11][1] = 0.005;
        fC[11][2] = 1.0000;
        fC[11][3] = 0.3434;
        fC[11][4] = 0.4321;
        
        fC[12][0] = 0.005;
        fC[12][1] = 0.005;
        fC[12][2] = 1.0000;
        fC[12][3] = 0.3692;
        fC[12][4] = 0.4879;
        
        fC[13][0] = 0.005;
        fC[13][1] = 0.005;
        fC[13][2] = 1.0000;
        fC[13][3] = 0.3993;
        fC[13][4] = 0.5377;
        
        fC[14][0] = 0.005;
        fC[14][1] = 0.005;
        fC[14][2] = 1.0000;
        fC[14][3] = 0.3818;
        fC[14][4] = 0.5547;
        
        fC[15][0] = 0.005;
        fC[15][1] = 0.005;
        fC[15][2] = 1.0000;
        fC[15][3] = 0.4003;
        fC[15][4] = 0.5484;
        
        fC[16][0] = 0.005;
        fC[16][1] = 0.005;
        fC[16][2] = 1.0000;
        fC[16][3] = 0.4281;
        fC[16][4] = 0.5383;
        
        fC[17][0] = 0.005;
        fC[17][1] = 0.005;
        fC[17][2] = 1.0000;
        fC[17][3] = 0.3960;
        fC[17][4] = 0.5374;
    }
    // 50-60%
    else if(centrCur < 60){
        fC[0][0] = 0.005;
        fC[0][1] = 0.005;
        fC[0][2] = 1.0000;
        fC[0][3] = 0.0010;
        fC[0][4] = 0.0010;
        
        fC[1][0] = 0.005;
        fC[1][1] = 0.005;
        fC[1][2] = 1.0000;
        fC[1][3] = 0.0076;
        fC[1][4] = 0.0032;
        
        fC[2][0] = 0.005;
        fC[2][1] = 0.005;
        fC[2][2] = 1.0000;
        fC[2][3] = 0.0329;
        fC[2][4] = 0.0085;
        
        fC[3][0] = 0.005;
        fC[3][1] = 0.005;
        fC[3][2] = 1.0000;
        fC[3][3] = 0.0653;
        fC[3][4] = 0.0423;
        
        fC[4][0] = 0.005;
        fC[4][1] = 0.005;
        fC[4][2] = 1.0000;
        fC[4][3] = 0.0923;
        fC[4][4] = 0.0813;
        
        fC[5][0] = 0.005;
        fC[5][1] = 0.005;
        fC[5][2] = 1.0000;
        fC[5][3] = 0.1219;
        fC[5][4] = 0.1161;
        
        fC[6][0] = 0.005;
        fC[6][1] = 0.005;
        fC[6][2] = 1.0000;
        fC[6][3] = 0.1519;
        fC[6][4] = 0.1520;
        
        fC[7][0] = 0.005;
        fC[7][1] = 0.005;
        fC[7][2] = 1.0000;
        fC[7][3] = 0.1763;
        fC[7][4] = 0.1858;
        
        fC[8][0] = 0.005;
        fC[8][1] = 0.005;
        fC[8][2] = 1.0000;
        fC[8][3] = 0.2178;
        fC[8][4] = 0.2385;
        
        fC[9][0] = 0.005;
        fC[9][1] = 0.005;
        fC[9][2] = 1.0000;
        fC[9][3] = 0.2618;
        fC[9][4] = 0.3070;
        
        fC[10][0] = 0.005;
        fC[10][1] = 0.005;
        fC[10][2] = 1.0000;
        fC[10][3] = 0.3067;
        fC[10][4] = 0.3625;
        
        fC[11][0] = 0.005;
        fC[11][1] = 0.005;
        fC[11][2] = 1.0000;
        fC[11][3] = 0.3336;
        fC[11][4] = 0.4188;
        
        fC[12][0] = 0.005;
        fC[12][1] = 0.005;
        fC[12][2] = 1.0000;
        fC[12][3] = 0.3706;
        fC[12][4] = 0.4511;
        
        fC[13][0] = 0.005;
        fC[13][1] = 0.005;
        fC[13][2] = 1.0000;
        fC[13][3] = 0.3765;
        fC[13][4] = 0.4729;
        
        fC[14][0] = 0.005;
        fC[14][1] = 0.005;
        fC[14][2] = 1.0000;
        fC[14][3] = 0.3942;
        fC[14][4] = 0.4855;
        
        fC[15][0] = 0.005;
        fC[15][1] = 0.005;
        fC[15][2] = 1.0000;
        fC[15][3] = 0.4051;
        fC[15][4] = 0.4762;
        
        fC[16][0] = 0.005;
        fC[16][1] = 0.005;
        fC[16][2] = 1.0000;
        fC[16][3] = 0.3843;
        fC[16][4] = 0.4763;
        
        fC[17][0] = 0.005;
        fC[17][1] = 0.005;
        fC[17][2] = 1.0000;
        fC[17][3] = 0.4237;
        fC[17][4] = 0.4773;
    }
    // 60-70%
    else if(centrCur < 70){
        fC[0][0] = 0.005;
        fC[0][1] = 0.005;
        fC[0][2] = 1.0000;
        fC[0][3] = 0.0010;
        fC[0][4] = 0.0010;
        
        fC[1][0] = 0.005;
        fC[1][1] = 0.005;
        fC[1][2] = 1.0000;
        fC[1][3] = 0.0071;
        fC[1][4] = 0.0012;
        
        fC[2][0] = 0.005;
        fC[2][1] = 0.005;
        fC[2][2] = 1.0000;
        fC[2][3] = 0.0336;
        fC[2][4] = 0.0097;
        
        fC[3][0] = 0.005;
        fC[3][1] = 0.005;
        fC[3][2] = 1.0000;
        fC[3][3] = 0.0662;
        fC[3][4] = 0.0460;
        
        fC[4][0] = 0.005;
        fC[4][1] = 0.005;
        fC[4][2] = 1.0000;
        fC[4][3] = 0.0954;
        fC[4][4] = 0.0902;
        
        fC[5][0] = 0.005;
        fC[5][1] = 0.005;
        fC[5][2] = 1.0000;
        fC[5][3] = 0.1181;
        fC[5][4] = 0.1306;
        
        fC[6][0] = 0.005;
        fC[6][1] = 0.005;
        fC[6][2] = 1.0000;
        fC[6][3] = 0.1481;
        fC[6][4] = 0.1662;
        
        fC[7][0] = 0.005;
        fC[7][1] = 0.005;
        fC[7][2] = 1.0000;
        fC[7][3] = 0.1765;
        fC[7][4] = 0.1963;
        
        fC[8][0] = 0.005;
        fC[8][1] = 0.005;
        fC[8][2] = 1.0000;
        fC[8][3] = 0.2155;
        fC[8][4] = 0.2433;
        
        fC[9][0] = 0.005;
        fC[9][1] = 0.005;
        fC[9][2] = 1.0000;
        fC[9][3] = 0.2580;
        fC[9][4] = 0.3022;
        
        fC[10][0] = 0.005;
        fC[10][1] = 0.005;
        fC[10][2] = 1.0000;
        fC[10][3] = 0.2872;
        fC[10][4] = 0.3481;
        
        fC[11][0] = 0.005;
        fC[11][1] = 0.005;
        fC[11][2] = 1.0000;
        fC[11][3] = 0.3170;
        fC[11][4] = 0.3847;
        
        fC[12][0] = 0.005;
        fC[12][1] = 0.005;
        fC[12][2] = 1.0000;
        fC[12][3] = 0.3454;
        fC[12][4] = 0.4258;
        
        fC[13][0] = 0.005;
        fC[13][1] = 0.005;
        fC[13][2] = 1.0000;
        fC[13][3] = 0.3580;
        fC[13][4] = 0.4299;
        
        fC[14][0] = 0.005;
        fC[14][1] = 0.005;
        fC[14][2] = 1.0000;
        fC[14][3] = 0.3903;
        fC[14][4] = 0.4326;
        
        fC[15][0] = 0.005;
        fC[15][1] = 0.005;
        fC[15][2] = 1.0000;
        fC[15][3] = 0.3690;
        fC[15][4] = 0.4491;
        
        fC[16][0] = 0.005;
        fC[16][1] = 0.005;
        fC[16][2] = 1.0000;
        fC[16][3] = 0.4716;
        fC[16][4] = 0.4298;
        
        fC[17][0] = 0.005;
        fC[17][1] = 0.005;
        fC[17][2] = 1.0000;
        fC[17][3] = 0.3875;
        fC[17][4] = 0.4083;
    }
    // 70-80%
    else if(centrCur < 80){
        fC[0][0] = 0.005;
        fC[0][1] = 0.005;
        fC[0][2] = 1.0000;
        fC[0][3] = 0.0010;
        fC[0][4] = 0.0010;
        
        fC[1][0] = 0.005;
        fC[1][1] = 0.005;
        fC[1][2] = 1.0000;
        fC[1][3] = 0.0075;
        fC[1][4] = 0.0007;
        
        fC[2][0] = 0.005;
        fC[2][1] = 0.005;
        fC[2][2] = 1.0000;
        fC[2][3] = 0.0313;
        fC[2][4] = 0.0124;
        
        fC[3][0] = 0.005;
        fC[3][1] = 0.005;
        fC[3][2] = 1.0000;
        fC[3][3] = 0.0640;
        fC[3][4] = 0.0539;
        
        fC[4][0] = 0.005;
        fC[4][1] = 0.005;
        fC[4][2] = 1.0000;
        fC[4][3] = 0.0923;
        fC[4][4] = 0.0992;
        
        fC[5][0] = 0.005;
        fC[5][1] = 0.005;
        fC[5][2] = 1.0000;
        fC[5][3] = 0.1202;
        fC[5][4] = 0.1417;
        
        fC[6][0] = 0.005;
        fC[6][1] = 0.005;
        fC[6][2] = 1.0000;
        fC[6][3] = 0.1413;
        fC[6][4] = 0.1729;
        
        fC[7][0] = 0.005;
        fC[7][1] = 0.005;
        fC[7][2] = 1.0000;
        fC[7][3] = 0.1705;
        fC[7][4] = 0.1999;
        
        fC[8][0] = 0.005;
        fC[8][1] = 0.005;
        fC[8][2] = 1.0000;
        fC[8][3] = 0.2103;
        fC[8][4] = 0.2472;
        
        fC[9][0] = 0.005;
        fC[9][1] = 0.005;
        fC[9][2] = 1.0000;
        fC[9][3] = 0.2373;
        fC[9][4] = 0.2916;
        
        fC[10][0] = 0.005;
        fC[10][1] = 0.005;
        fC[10][2] = 1.0000;
        fC[10][3] = 0.2824;
        fC[10][4] = 0.3323;
        
        fC[11][0] = 0.005;
        fC[11][1] = 0.005;
        fC[11][2] = 1.0000;
        fC[11][3] = 0.3046;
        fC[11][4] = 0.3576;
        
        fC[12][0] = 0.005;
        fC[12][1] = 0.005;
        fC[12][2] = 1.0000;
        fC[12][3] = 0.3585;
        fC[12][4] = 0.4003;
        
        fC[13][0] = 0.005;
        fC[13][1] = 0.005;
        fC[13][2] = 1.0000;
        fC[13][3] = 0.3461;
        fC[13][4] = 0.3982;
        
        fC[14][0] = 0.005;
        fC[14][1] = 0.005;
        fC[14][2] = 1.0000;
        fC[14][3] = 0.3362;
        fC[14][4] = 0.3776;
        
        fC[15][0] = 0.005;
        fC[15][1] = 0.005;
        fC[15][2] = 1.0000;
        fC[15][3] = 0.3071;
        fC[15][4] = 0.3500;
        
        fC[16][0] = 0.005;
        fC[16][1] = 0.005;
        fC[16][2] = 1.0000;
        fC[16][3] = 0.2914;
        fC[16][4] = 0.3937;
        
        fC[17][0] = 0.005;
        fC[17][1] = 0.005;
        fC[17][2] = 1.0000;
        fC[17][3] = 0.3727;
        fC[17][4] = 0.3877;
    }
    // 80-100%
    else{
        fC[0][0] = 0.005;
        fC[0][1] = 0.005;
        fC[0][2] = 1.0000;
        fC[0][3] = 0.0010;
        fC[0][4] = 0.0010;
        
        fC[1][0] = 0.005;
        fC[1][1] = 0.005;
        fC[1][2] = 1.0000;
        fC[1][3] = 0.0060;
        fC[1][4] = 0.0035;
        
        fC[2][0] = 0.005;
        fC[2][1] = 0.005;
        fC[2][2] = 1.0000;
        fC[2][3] = 0.0323;
        fC[2][4] = 0.0113;
        
        fC[3][0] = 0.005;
        fC[3][1] = 0.005;
        fC[3][2] = 1.0000;
        fC[3][3] = 0.0609;
        fC[3][4] = 0.0653;
        
        fC[4][0] = 0.005;
        fC[4][1] = 0.005;
        fC[4][2] = 1.0000;
        fC[4][3] = 0.0922;
        fC[4][4] = 0.1076;
        
        fC[5][0] = 0.005;
        fC[5][1] = 0.005;
        fC[5][2] = 1.0000;
        fC[5][3] = 0.1096;
        fC[5][4] = 0.1328;
        
        fC[6][0] = 0.005;
        fC[6][1] = 0.005;
        fC[6][2] = 1.0000;
        fC[6][3] = 0.1495;
        fC[6][4] = 0.1779;
        
        fC[7][0] = 0.005;
        fC[7][1] = 0.005;
        fC[7][2] = 1.0000;
        fC[7][3] = 0.1519;
        fC[7][4] = 0.1989;
        
        fC[8][0] = 0.005;
        fC[8][1] = 0.005;
        fC[8][2] = 1.0000;
        fC[8][3] = 0.1817;
        fC[8][4] = 0.2472;
        
        fC[9][0] = 0.005;
        fC[9][1] = 0.005;
        fC[9][2] = 1.0000;
        fC[9][3] = 0.2429;
        fC[9][4] = 0.2684;
        
        fC[10][0] = 0.005;
        fC[10][1] = 0.005;
        fC[10][2] = 1.0000;
        fC[10][3] = 0.2760;
        fC[10][4] = 0.3098;
        
        fC[11][0] = 0.005;
        fC[11][1] = 0.005;
        fC[11][2] = 1.0000;
        fC[11][3] = 0.2673;
        fC[11][4] = 0.3198;
        
        fC[12][0] = 0.005;
        fC[12][1] = 0.005;
        fC[12][2] = 1.0000;
        fC[12][3] = 0.3165;
        fC[12][4] = 0.3564;
        
        fC[13][0] = 0.005;
        fC[13][1] = 0.005;
        fC[13][2] = 1.0000;
        fC[13][3] = 0.3526;
        fC[13][4] = 0.3011;
        
        fC[14][0] = 0.005;
        fC[14][1] = 0.005;
        fC[14][2] = 1.0000;
        fC[14][3] = 0.3788;
        fC[14][4] = 0.3011;
        
        fC[15][0] = 0.005;
        fC[15][1] = 0.005;
        fC[15][2] = 1.0000;
        fC[15][3] = 0.3788;
        fC[15][4] = 0.3011;
        
        fC[16][0] = 0.005;
        fC[16][1] = 0.005;
        fC[16][2] = 1.0000;
        fC[16][3] = 0.3788;
        fC[16][4] = 0.3011;
        
        fC[17][0] = 0.005;
        fC[17][1] = 0.005;
        fC[17][2] = 1.0000;
        fC[17][3] = 0.3788;
        fC[17][4] = 0.3011;
    }
    
    for(Int_t i=18;i<fgkPIDptBin;i++){
        fBinLimitPID[i] = 3.0 + 0.2 * (i-18);
        fC[i][0] = fC[17][0];
        fC[i][1] = fC[17][1];
        fC[i][2] = fC[17][2];
        fC[i][3] = fC[17][3];
        fC[i][4] = fC[17][4];
    }  
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////
Bool_t AliAnalysisTaskFlowModes::TPCTOFagree(const AliVTrack *track)
{
    //check pid agreement between TPC and TOF
    Bool_t status = kFALSE;
    
    const Float_t c = 2.99792457999999984e-02;
    
    Float_t mass[5] = {5.10998909999999971e-04,1.05658000000000002e-01,1.39570000000000000e-01,4.93676999999999977e-01,9.38271999999999995e-01};
    
    
    Double_t exptimes[9];
    track->GetIntegratedTimes(exptimes);
    
    Float_t dedx = track->GetTPCsignal();
    
    Float_t p = track->P();
    Float_t time = track->GetTOFsignal()- fESDpid.GetTOFResponse().GetStartTime(p);
    Float_t tl = exptimes[0]*c; // to work both for ESD and AOD
    
    Float_t betagammares =  fESDpid.GetTOFResponse().GetExpectedSigma(p, exptimes[4], mass[4]);
    
    Float_t betagamma1 = tl/(time-5 *betagammares) * 33.3564095198152043;
    
    //  printf("betagamma1 = %f\n",betagamma1);
    
    if(betagamma1 < 0.1) betagamma1 = 0.1;
    
    if(betagamma1 < 0.99999) betagamma1 /= TMath::Sqrt(1-betagamma1*betagamma1);
    else betagamma1 = 100;
    
    Float_t betagamma2 = tl/(time+5 *betagammares) * 33.3564095198152043;
    //  printf("betagamma2 = %f\n",betagamma2);
    
    if(betagamma2 < 0.1) betagamma2 = 0.1;
    
    if(betagamma2 < 0.99999) betagamma2 /= TMath::Sqrt(1-betagamma2*betagamma2);
    else betagamma2 = 100;
    
    
    Float_t momtpc=track->GetTPCmomentum();
    
    for(Int_t i=0;i < 5;i++){
        Float_t resolutionTOF =  fESDpid.GetTOFResponse().GetExpectedSigma(p, exptimes[i], mass[i]);
        if(TMath::Abs(exptimes[i] - time) < 5 * resolutionTOF){
            Float_t dedxExp = 0;
            if(i==0) dedxExp =  fESDpid.GetTPCResponse().GetExpectedSignal(momtpc,AliPID::kElectron);
            else if(i==1) dedxExp =  fESDpid.GetTPCResponse().GetExpectedSignal(momtpc,AliPID::kMuon);
            else if(i==2) dedxExp =  fESDpid.GetTPCResponse().GetExpectedSignal(momtpc,AliPID::kPion);
            else if(i==3) dedxExp =  fESDpid.GetTPCResponse().GetExpectedSignal(momtpc,AliPID::kKaon);
            else if(i==4) dedxExp =  fESDpid.GetTPCResponse().GetExpectedSignal(momtpc,AliPID::kProton);
            
            Float_t resolutionTPC = 2;
            if(i==0) resolutionTPC =   fESDpid.GetTPCResponse().GetExpectedSigma(momtpc,track->GetTPCsignalN(),AliPID::kElectron);
            else if(i==1) resolutionTPC =   fESDpid.GetTPCResponse().GetExpectedSigma(momtpc,track->GetTPCsignalN(),AliPID::kMuon);
            else if(i==2) resolutionTPC =   fESDpid.GetTPCResponse().GetExpectedSigma(momtpc,track->GetTPCsignalN(),AliPID::kPion);
            else if(i==3) resolutionTPC =   fESDpid.GetTPCResponse().GetExpectedSigma(momtpc,track->GetTPCsignalN(),AliPID::kKaon);
            else if(i==4) resolutionTPC =   fESDpid.GetTPCResponse().GetExpectedSigma(momtpc,track->GetTPCsignalN(),AliPID::kProton);
            
            if(TMath::Abs(dedx - dedxExp) < 3 * resolutionTPC){
                status = kTRUE;
            }
        }
    }
    
    Float_t bb1 =  fESDpid.GetTPCResponse().Bethe(betagamma1);
    Float_t bb2 =  fESDpid.GetTPCResponse().Bethe(betagamma2);
    Float_t bbM =  fESDpid.GetTPCResponse().Bethe((betagamma1+betagamma2)*0.5);
    
    
    //  status = kFALSE;
    // for nuclei
    Float_t resolutionTOFpr =   fESDpid.GetTOFResponse().GetExpectedSigma(p, exptimes[4], mass[4]);
    Float_t resolutionTPCpr =   fESDpid.GetTPCResponse().GetExpectedSigma(momtpc,track->GetTPCsignalN(),AliPID::kProton);
    if(TMath::Abs(dedx-bb1) < resolutionTPCpr*3 && exptimes[4] < time-7*resolutionTOFpr){
        status = kTRUE;
    }
    else if(TMath::Abs(dedx-bb2) < resolutionTPCpr*3 && exptimes[4] < time-7*resolutionTOFpr){
        status = kTRUE;
    }
    else if(TMath::Abs(dedx-bbM) < resolutionTPCpr*3 && exptimes[4] < time-7*resolutionTOFpr){
        status = kTRUE;
    }
    
    return status;
}
