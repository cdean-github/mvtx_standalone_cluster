#include <fun4all/Fun4AllUtils.h>
#include <fun4all/Fun4AllDstInputManager.h>
#include <fun4all/Fun4AllServer.h>
#include <fun4all/Fun4AllInputManager.h>
#include <fun4all/Fun4AllOutputManager.h>
#include <fun4all/Fun4AllDstOutputManager.h>
#include <fun4all/Fun4AllRunNodeInputManager.h>
#include <fun4allraw/InputManagerType.h>

#include <ffamodules/CDBInterface.h>
#include <ffamodules/FlagHandler.h>
#include <ffamodules/HeadReco.h>
#include <ffamodules/SyncReco.h>
#include <intt/InttCombinedRawDataDecoder.h>
#include <mvtx/MvtxCombinedRawDataDecoder.h>
#include <trackingdiagnostics/TrackSeedTrackMapConverter.h>
#include <trackingdiagnostics/TrackResiduals.h>
#include <trackreco/AzimuthalSeeder.h>
#include <trackreco/PHActsSiliconSeeding.h>
#include <trackreco/PHSiliconSeedMerger.h>
#include <trackreco/PHSimpleVertexFinder.h>
#include <Trkr_Clustering.C>
#include <Trkr_Reco.C>
#include <Trkr_RecoInit.C>
#include "G4Setup_sPHENIX.C"
#include <event_display_maker/mvtx_standalone_cluster.h>

#include <phool/recoConsts.h>

#include <array>

R__LOAD_LIBRARY(libfun4all.so)
R__LOAD_LIBRARY(libfun4allraw.so)
R__LOAD_LIBRARY(libintt.so)
R__LOAD_LIBRARY(libeventdisplaymaker.so)
R__LOAD_LIBRARY(libmvtx.so)

bool isGood(const string &infile);

// https://stackoverflow.com/questions/478898/how-do-i-execute-a-command-and-get-the-output-of-the-command-within-c-using-po
string exec(const char *cmd)
{
  std::array<char, 128> buffer;
  std::string result;
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
  if (!pipe)
  {
    throw std::runtime_error("popen() failed!");
  }
  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
  {
    result += buffer.data();
  }
  return result;
}

void Fun4All_Silicon_Analyser(int nEvents = 0, string infile = "dummy.file")
{
  bool readPRDF = false;
  bool runTrkrHits = true;
  bool runTkrkClus = true;
  bool runSeeding = true;
  bool writeOutputDST = false;
  bool stripRawHit = true;

  int verbosity = 0;

  Fun4AllServer *se = Fun4AllServer::instance();
  se->Verbosity(1);

  G4TRACKING::convert_seeds_to_svtxtracks = true;
  std::cout << "Converting to seeds : " << G4TRACKING::convert_seeds_to_svtxtracks << std::endl;
  std::pair<int, int> runseg = Fun4AllUtils::GetRunSegment(infile);
  int run_number = runseg.first;
  int file_number = runseg.second;

  recoConsts *rc = recoConsts::instance();
  Enable::CDB = true;
  rc->set_StringFlag("CDB_GLOBALTAG", "ProdA_2024");
  rc->set_uint64Flag("TIMESTAMP", 6);
  rc->set_IntFlag("RUNNUMBER", run_number);

  ACTSGEOM::mvtxMisalignment = 100;
  ACTSGEOM::inttMisalignment = 100.;
  //ACTSGEOM::tpotMisalignment = 100.;

  std::string geofile = CDBInterface::instance()->getUrl("Tracking_Geometry");
  Fun4AllRunNodeInputManager *ingeo = new Fun4AllRunNodeInputManager("GeoIn");
  ingeo->AddFile(geofile);
  se->registerInputManager(ingeo);

  G4MAGNET::magfield_rescale = 1;

  ACTSGEOM::ActsGeomInit();

  Fun4AllInputManager *inputmanager = new Fun4AllDstInputManager("DSTin");
  inputmanager->AddFile(infile);
  se->registerInputManager(inputmanager);   

  string outpath = ".";
  string outtrailer = "MVTX_clusterVtx_" + to_string(run_number) + "_" + to_string(file_number) + ".root";

  if (runTrkrHits)
  {
    Mvtx_HitUnpacking();
    Intt_HitUnpacking();
  }

  if (runTkrkClus) 
  {
    Mvtx_Clustering();
    Intt_Clustering();
  }

  if (runSeeding)
  {  
    auto silicon_Seeding = new PHActsSiliconSeeding;
    silicon_Seeding->Verbosity(verbosity);
    silicon_Seeding->setunc(1000);
    silicon_Seeding->helixcut(0.1);
    se->registerSubsystem(silicon_Seeding);

    auto merger = new PHSiliconSeedMerger;
    merger->Verbosity(verbosity);
    se->registerSubsystem(merger);

    auto converter = new TrackSeedTrackMapConverter;
    converter->setTrackSeedName("SiliconTrackSeedContainer");
    converter->setFieldMap(G4MAGNET::magfield_tracking);
    converter->Verbosity(verbosity);
    //converter->constField();
    se->registerSubsystem(converter);

    PHSimpleVertexFinder *finder = new PHSimpleVertexFinder;
    finder->Verbosity(verbosity);
    finder->setDcaCut(0.25);
    finder->setTrackPtCut(-99999.);
    finder->setBeamLineCut(1);
    finder->setTrackQualityCut(50);
    finder->setNmvtxRequired(3);
    finder->setOutlierPairCut(0.08);
    se->registerSubsystem(finder);
 
    string clusterpath = "clusterVtxTrees";

    auto resid = new TrackResiduals("TrackResiduals");
    resid->outfileName(clusterpath + "/TrackResiduals_" + outtrailer);
    resid->alignment(false);
    resid->clusterTree();
    resid->hitTree();
    resid->Verbosity(verbosity);
    se->registerSubsystem(resid);
  }

  if (writeOutputDST)
  {
    std::string outputFile = outpath + "/DST_" + outtrailer;
    Fun4AllOutputManager *out = new Fun4AllDstOutputManager("out", outputFile);
    if (stripRawHit)
    {
      out->StripNode("GL1RAWHIT");
      out->StripNode("INTTRAWHIT");
      out->StripNode("G4HIT_INTT");
      out->StripNode("MVTXRAWEVTHEADER");
      out->StripNode("MVTXRAWHIT");
      out->StripNode("G4HIT_MVTX");
      out->StripNode("G4HIT_TPC");
      out->StripNode("G4HIT_MICROMEGAS");
      out->StripNode("G4TruthInfo");
      out->StripNode("PHG4INEVENT");
    }
    se->registerOutputManager(out);
  }

  se->run(nEvents);

  se->End();
  delete se;
  gSystem->Exit(0);
}

bool isGood(const string &infile)
{
  ifstream intest;
  intest.open(infile);
  bool goodfile = false;
  if (intest.is_open())
  {
    if (intest.peek() != std::ifstream::traits_type::eof()) // is it non zero?
    {
      goodfile = true;
    }
      intest.close();
  }
  return goodfile;
}
