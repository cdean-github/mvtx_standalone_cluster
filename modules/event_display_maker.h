// Tell emacs that this is a C++ source
//  -*- C++ -*-.
#ifndef EVENTDISPLAYMAKER_H
#define EVENTDISPLAYMAKER_H

#include <fun4all/SubsysReco.h>

#include <string>

#include <ffarawobjects/Gl1RawHit.h>
#include <trackbase/TrkrClusterContainer.h>
#include <trackbase/TrkrCluster.h>
#include <trackbase/ActsGeometry.h>
#include <trackbase_historic/SvtxTrackMap.h>
#include <globalvertex/SvtxVertexMap.h>

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

  void setRunDate( std::string date ) { m_run_date = date; }

  void setNEventDisplays( int nDisplays ) { maxEvtDisplays = nDisplays; }

  void requirePV() { m_require_vertex = true; }

 private:

  std::ofstream outFile;

  void event_file_start(std::ofstream &json_file_header, std::string date, int runid, uint64_t bco);

  void event_file_trailer_0(std::ofstream &json_file_trailer);
  void event_file_trailer_1(std::ofstream &json_file_trailer);
  void event_file_trailer_2(std::ofstream &json_file_trailer);

  void addHit(std::ofstream &json_file_hit, bool &firstHit, Acts::Vector3 hit);
  void addTrack(std::ofstream &json_file_hit, SvtxTrack* aTrack, bool lastTrack);
  void loopTrackClusters(bool isSilicon);

  std::string getDate();

  TrkrClusterContainer *trktClusterContainer = nullptr;
  ActsGeometry *actsGeom = nullptr;
  Gl1RawHit* gl1info = nullptr;
  SvtxVertexMap* vertexMap = nullptr;
  SvtxTrackMap* trackMap = nullptr;

  int m_runNumber = 0;
  uint64_t triggerBCO = 0;

  bool m_require_vertex = false;
  std::string m_evt_display_path = ".";
  std::string m_run_date = "2024-04-14";

  unsigned int evtDisplayCounter = 0;
  unsigned int maxEvtDisplays = 10;
};

#endif // EVENTDISPLAYMAKER_H
