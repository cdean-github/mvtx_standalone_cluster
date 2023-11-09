// Tell emacs that this is a C++ source
//  -*- C++ -*-.
#ifndef MVTXSTANDALONECLUSTER_H
#define MVTXSTANDALONECLUSTER_H

#include <fun4all/SubsysReco.h>

#include <string>
#include <vector>

#include <TFile.h>
#include <TTree.h>
#include <TVector2.h>
#include <TVector3.h>

#include <mvtx/CylinderGeom_Mvtx.h>
#include <g4detectors/PHG4CylinderGeomContainer.h>

#include <trackbase/MvtxDefs.h>
#include <trackbase/MvtxEventInfov2.h>
#include <trackbase/TrkrHitSetContainerv1.h>
#include <trackbase/TrkrHitv2.h>
#include <trackbase/TrkrHitSet.h>
#include <trackbase/TrkrClusterContainerv4.h>
#include <trackbase/TrkrClusterHitAssocv3.h>
#include <trackbase/TrkrClusterv4.h>
#include <trackbase/ActsGeometry.h>

class PHCompositeNode;

class mvtx_standalone_cluster : public SubsysReco
{
 public:

  mvtx_standalone_cluster(const std::string &name = "mvtx_standalone_cluster");

  ~mvtx_standalone_cluster() override;

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
  int f4aCounter = 0; 

  TrkrHitSetContainerv1 *trkrHitSetContainer = nullptr;
  TrkrClusterContainer *trktClusterContainer = nullptr;
  ActsGeometry *actsGeom = nullptr;
  PHG4CylinderGeomContainer *geantGeom;
  MvtxEventInfov2* mvtx_event_header = nullptr;

  TFile* outFile = nullptr;
  TTree* outTree = nullptr;
  std::string outFileName = "outputClusters.root";

  int event = 0;
  int strobe_BCO = 0;
  std::vector<int> L1_BCOs;
  int numberL1s = 0;
  int layer = 0;
  int stave = 0;
  int chip = 0;
  int row = 0;
  int col = 0;
  float localX = 0.;
  float localY = 0.;
  float globalX = 0.;
  float globalY = 0.;
  float globalZ = 0.;
  float clusZ = 0.;
  float clusPhi = 0.;
  unsigned int clusSize = 0;
};

#endif // MVTXSTANDALONECLUSTER_H
