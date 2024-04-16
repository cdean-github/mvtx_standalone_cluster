// Tell emacs that this is a C++ source
//  -*- C++ -*-.
#ifndef EVENTDISPLAYMAKER_H
#define EVENTDISPLAYMAKER_H

#include <fun4all/SubsysReco.h>

#include <string>
#include <vector>

#include <TFile.h>
#include <TTree.h>
#include <TVector2.h>
#include <TVector3.h>

#include <mvtx/CylinderGeom_Mvtx.h>
#include <intt/CylinderGeomIntt.h>
#include <micromegas/CylinderGeomMicromegas.h>
#include <g4detectors/PHG4CylinderGeomContainer.h>
#include <g4detectors/PHG4TpcCylinderGeomContainer.h>
#include <g4detectors/PHG4TpcCylinderGeom.h>

#include <micromegas/MicromegasDefs.h>
#include <trackbase/MvtxDefs.h>
#include <trackbase/MvtxEventInfov2.h>
#include <trackbase/TpcDefs.h>
#include <trackbase/TrkrHitSetContainerv1.h>
#include <trackbase/TrkrHitv2.h>
#include <trackbase/TrkrHitSet.h>
#include <trackbase/TrkrClusterContainerv4.h>
#include <trackbase/TrkrClusterHitAssocv3.h>
#include <trackbase/TrkrClusterv4.h>
#include <trackbase/ActsGeometry.h>

class PHCompositeNode;

class event_display_maker : public SubsysReco
{
 public:

  event_display_maker(const std::string &name = "event_display_maker");

  ~event_display_maker() override;

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

  void event_file_start(std::ofstream &jason_file_header, std::string date, int runid, uint64_t bco);

  void event_file_trailer(std::ofstream &json_file_trailer, float minX, float minY, float minZ, float maxX, float maxY, float maxZ);

  void setEventDisplayPath( std::string path ) { m_evt_display_path = path; }

  void setMinClusters( unsigned int value ) { m_min_clusters = value; }

  void setRunDate( std::string date ) { m_run_date = date; }

 private:

  TrkrHitSetContainerv1 *trkrHitSetContainer = nullptr;
  TrkrClusterContainer *trktClusterContainer = nullptr;
  ActsGeometry *actsGeom = nullptr;
  PHG4CylinderGeomContainer *geantGeomMvtx;
  PHG4CylinderGeomContainer *geantGeomIntt;
  PHG4TpcCylinderGeomContainer *geantGeomTpc;
  PHG4CylinderGeomContainer *geantGeomTpot;
  MvtxEventInfov2* mvtx_event_header = nullptr;

  int m_runNumber = 0;
  std::vector<uint64_t> L1_BCOs;

  int numberL1s = 0;
  int layer = 0;
  float localX = 0.;
  float localY = 0.;
  float globalX = 0.;
  float globalY = 0.;
  float globalZ = 0.;
  std::string m_evt_display_path = ".";
  unsigned int m_min_clusters = 6;
  std::string m_run_date = "2024-04-14";
};

#endif // EVENTDISPLAYMAKER_H
