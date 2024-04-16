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
#include <trackbase/MvtxEventInfo.h>
#include <trackbase/TpcDefs.h>
#include <trackbase/TrkrHit.h>
#include <trackbase/TrkrHitSet.h>
#include <trackbase/TrkrHitSetContainer.h>
#include <trackbase/TrkrClusterContainer.h>
#include <trackbase/TrkrClusterHitAssoc.h>
#include <trackbase/TrkrCluster.h>
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

  void setEventDisplayPath( std::string path ) { m_evt_display_path = path; }

  void minMVTXClusters( int value ) { m_minMVTXClusters = value; }
  void minINTTClusters( int value ) { m_minINTTClusters = value; }
  void minTPCHits     ( int value ) { m_minTPCHits = value; }
  void minTPOTClusters( int value ) { m_minTPOTClusters = value; }

  void setRunDate( std::string date ) { m_run_date = date; }

 private:

  void event_file_start(std::ofstream &json_file_header, std::string date, int runid, uint64_t bco);

  void event_file_trailer(std::ofstream &json_file_trailer, float minHit[3], float maxHit[3]);

  void addHit(std::ofstream &json_file_hit, bool &firstHit, float hit[3], float &minX, float &minY, float &minZ, float &maxX, float &maxY, float &maxZ, bool isMVTX);

  std::string getDate();

  //std::string exec(const char *cmd);

  TrkrHitSetContainer *trkrHitSetContainer = nullptr;
  TrkrClusterContainer *trktClusterContainer = nullptr;
  ActsGeometry *actsGeom = nullptr;
  PHG4CylinderGeomContainer *geantGeomMvtx;
  PHG4CylinderGeomContainer *geantGeomIntt;
  PHG4TpcCylinderGeomContainer *geantGeomTpc;
  PHG4CylinderGeomContainer *geantGeomTpot;
  MvtxEventInfo* mvtx_event_header = nullptr;

  int m_runNumber = 0;
  std::vector<uint64_t> L1_BCOs;

  int event = 0;
  int numberL1s = 0;
  int layer = 0;
  float localX = 0.;
  float localY = 0.;
  float global[3] = {0};
  std::string m_evt_display_path = ".";
  std::string m_run_date = "2024-04-14";
  int m_minMVTXClusters = 4;
  int m_minINTTClusters = 2;
  int m_minTPCHits = 40;
  int m_minTPOTClusters = 1;
};

#endif // EVENTDISPLAYMAKER_H
