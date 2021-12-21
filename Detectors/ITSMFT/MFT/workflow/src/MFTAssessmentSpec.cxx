// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// @file MFTAssessmentSpec.cxx

#include "Framework/ControlService.h"
#include "Framework/ConfigParamRegistry.h"
#include "Framework/Logger.h"
#include "MFTWorkflow/MFTAssessmentSpec.h"
#include "CommonUtils/NameConf.h"
#include <TFile.h>

using namespace o2::framework;

namespace o2
{
namespace mft
{

//_____________________________________________________________
void MFTAssessmentSpec::init(InitContext& ic)
{

  for (int sw = 0; sw < NStopWatches; sw++) {
    mTimer[sw].Stop();
    mTimer[sw].Reset();
  }

  mTimer[SWTot].Start(false);
  mMFTAssessment = std::make_unique<o2::mft::MFTAssessment>(mUseMC);
  mMFTAssessment->init();
}

//_____________________________________________________________
void MFTAssessmentSpec::run(o2::framework::ProcessingContext& pc)
{
  mTimer[SWQCAsync].Start(false);
  mMFTAssessment->runASyncQC(pc);
  mTimer[SWQCAsync].Stop();

  if (mUseMC) {

    mTimer[SWTrackables].Start(false);
    mMFTAssessment->processTrackables();
    mTimer[SWTrackables].Stop();

    mTimer[SWGenerated].Start(false);
    mMFTAssessment->processGeneratedTracks();
    mTimer[SWGenerated].Stop();

    mTimer[SWRecoAndTrue].Start(false);
    mMFTAssessment->processRecoAndTrueTracks();
    mTimer[SWRecoAndTrue].Stop();
  }
}

//_____________________________________________________________
void MFTAssessmentSpec::endOfStream(o2::framework::EndOfStreamContext& ec)
{
  mMFTAssessment->finalize();
  sendOutput(ec.outputs());
  mTimer[SWTot].Stop();

  for (int i = 0; i < NStopWatches; i++) {
    LOGF(info, "Timing %18s: Cpu: %.3e s; Real: %.3e s in %d slots", TimerName[i], mTimer[i].CpuTime(), mTimer[i].RealTime(), mTimer[i].Counter() - 1);
  }
}

//_____________________________________________________________
void MFTAssessmentSpec::sendOutput(DataAllocator& output)
{
  TObjArray objar;
  mMFTAssessment->getHistos(objar);

  output.snapshot(Output{"MFT", "MFTASSESSMENT", 0, Lifetime::Timeframe}, objar);

  TFile* f = new TFile(Form("MFTAssessment.root"), "RECREATE");
  objar.Write("MFTAssessment", TObject::kSingleKey);
  f->Close();
}

//_____________________________________________________________
DataProcessorSpec getMFTAssessmentSpec(bool useMC)
{
  std::vector<InputSpec> inputs;
  inputs.emplace_back("compClusters", "MFT", "COMPCLUSTERS", 0, Lifetime::Timeframe);
  inputs.emplace_back("patterns", "MFT", "PATTERNS", 0, Lifetime::Timeframe);
  inputs.emplace_back("clustersrofs", "MFT", "CLUSTERSROF", 0, Lifetime::Timeframe);
  inputs.emplace_back("tracksrofs", "MFT", "MFTTrackROF", 0, Lifetime::Timeframe);
  inputs.emplace_back("tracks", "MFT", "TRACKS", 0, Lifetime::Timeframe);

  if (useMC) {
    inputs.emplace_back("clslabels", "MFT", "CLUSTERSMCTR", 0, Lifetime::Timeframe);
    inputs.emplace_back("trklabels", "MFT", "TRACKSMCTR", 0, Lifetime::Timeframe);
  }

  std::vector<OutputSpec> outputs;
  outputs.emplace_back("MFT", "MFTASSESSMENT", 0, Lifetime::Timeframe);

  return DataProcessorSpec{
    "mft-assessment",
    inputs,
    outputs,
    AlgorithmSpec{adaptFromTask<o2::mft::MFTAssessmentSpec>(useMC)},
    Options{{}}};
}

} // namespace mft
} // namespace o2
