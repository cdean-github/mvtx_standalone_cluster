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
#include <g4detectors/PHG4CylinderGeomContainer.h>

#include <trackbase/MvtxDefs.h>
#include <trackbase/MvtxEventInfo.h>
#include <trackbase/TrkrHit.h>
#include <trackbase/TrkrHitSet.h>
#include <trackbase/TrkrHitSetContainer.h>
#include <trackbase/TrkrClusterContainer.h>
#include <trackbase/TrkrClusterHitAssoc.h>
#include <trackbase/TrkrCluster.h>
#include <trackbase/ActsGeometry.h>
#include <trackbase_historic/SvtxTrackMap.h>
#include <globalvertex/SvtxVertexMap.h>

class PHCompositeNode;

class silicon_detector_analyser : public SubsysReco
{
 public:

  silicon_detector_analyser(const std::string &name = "silicon_detector_analyser");

  ~silicon_detector_analyser() override;

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

  void writeFile( const std::string &file ){ outRootName = file; }

  void setEventDisplayPath( std::string path ) { m_evt_display_path = path; }

  void setRunDate( std::string date ) { m_run_date = date; }

 private:

  void event_file_start(std::ofstream &json_file_header, std::string date, int runid, uint64_t bco);

  void event_file_trailer_1(std::ofstream &json_file_trailer);
  void event_file_trailer_2(std::ofstream &json_file_trailer);

  void addHit(std::ofstream &json_file_hit, bool &firstHit, float hit[3]);
  void addTrack(std::ofstream &json_file_hit, SvtxTrack* aTrack, bool lastTrack);

  std::string getDate();

  //std::string exec(const char *cmd);

  TrkrHitSetContainer *trkrHitSetContainer = nullptr;
  TrkrClusterContainer *trktClusterContainer = nullptr;
  ActsGeometry *actsGeom = nullptr;
  PHG4CylinderGeomContainer *geantGeomMvtx;
  PHG4CylinderGeomContainer *geantGeomIntt;
  MvtxEventInfo* mvtx_event_header = nullptr;

  TFile* outRoot = nullptr;
  TTree* outTree = nullptr;
  std::string outRootName = "outputClusters.root";

  int m_runNumber = 0;
  uint64_t triggerBCO = 0;
  std::vector<uint64_t> L1_BCOs;
  float vertex_x = 0;
  float vertex_y = 0;
  float vertex_z = 0;
  std::vector<uint64_t> strobe_BCOs;
  std::vector<int> clusLayer;
  std::vector<float> clusPhi;
  std::vector<float> clusEta;
  std::vector<unsigned int> clusSize;
  std::vector<float> clusX;
  std::vector<float> clusY;
  std::vector<float> clusZ;

  int event = 0;
  int numberL1s = 0;
  int layer = 0;
  float localX = 0.;
  float localY = 0.;
  float global[3] = {0};
  std::string m_evt_display_path = ".";
  std::string m_run_date = "2024-04-14";

  unsigned int evtDisplayCounter = 0;
  unsigned int maxEvtDisplays = 10;
};

#endif // EVENTDISPLAYMAKER_H
