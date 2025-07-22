// Tell emacs that this is a C++ source
//  -*- C++ -*-.
#ifndef LATENCYANALYZER_H
#define LATENCYANALYZER_H

#include <fun4all/SubsysReco.h>

#include <string>
#include <vector>

#include <TFile.h>
#include <TTree.h>
#include <TVector2.h>
#include <TVector3.h>

#include <mvtx/CylinderGeom_Mvtx.h>
#include <mvtx/CylinderGeom_MvtxHelper.h>
#include <g4detectors/PHG4CylinderGeomContainer.h>
#include <g4main/PHG4Hit.h>
#include <g4main/PHG4HitContainer.h>

#include <trackbase/MvtxDefs.h>
#include <trackbase/MvtxEventInfov2.h>
#include <trackbase/TrkrHitSetContainerv1.h>
#include <trackbase/TrkrHitv2.h>
#include <trackbase/TrkrHitSet.h>
#include <trackbase/TrkrClusterContainerv4.h>
#include <trackbase/TrkrClusterHitAssocv3.h>
#include <trackbase/TrkrClusterv4.h>
#include <trackbase/ActsGeometry.h>

#include <calotrigger/TriggerAnalyzer.h>

class PHCompositeNode;

class latency_analyzer : public SubsysReco
{
 public:

  latency_analyzer(const std::string &name = "latency_analyzer");

  ~latency_analyzer() override;

  /** Called during initialization.
      Typically this is where you can book histograms, and e.g.
      register them to Fun4AllServer (so they can be output to file
      using Fun4AllServer::dumpHistos() method).
   */
  int Init(PHCompositeNode *topNode) override;

  /** Called for first event when run number is known.
      Typically this is where you may want to fetch data from
      database, because you know the run number. A place
      to book histograms which have to know the run number.
   */
  //int InitRun(PHCompositeNode *topNode) override;

  /** Called for each event.
      This is where you do the real work.
   */
  int process_event(PHCompositeNode *topNode) override;

  /// Clean up internals after each event.
  //int ResetEvent(PHCompositeNode *topNode) override;

  /// Called at the end of each run.
  //int EndRun(const int runnumber) override;

  /// Called at the end of all processing.
  int End(PHCompositeNode *topNode) override;

  /// Reset
  int Reset(PHCompositeNode * /*topNode*/) override;

  void Print(const std::string &what = "ALL") const override;

  void writeFile( const std::string &file ){ outFileName = file; }

 private:
  std::string to_string_with_precision(const float a_value, const int n);

  int f4aCounter = 0; 

  TriggerAnalyzer *triggeranalyzer = nullptr;

  TrkrHitSetContainer *trkrHitSetContainer = nullptr;
  TrkrClusterContainer *trkrClusterContainer = nullptr;

  TFile* outFile = nullptr;
  TTree* outTree = nullptr;
  std::string outFileName = "outputClusters.root";
  static const unsigned int nTriggers = 20;
  int m_runNumber = 0;
  int event = 0;
  int n_hits = 0;
  int n_hits_L0 = 0;
  int n_hits_L1 = 0;
  int n_hits_L2 = 0;
  int n_clusters_L1 = 0;
  int n_clusters_L2 = 0;
  float trigger_latency = 0;
  bool m_trigger_bit[nTriggers] = {false};
};

#endif // LATENCYANALYZER_H
