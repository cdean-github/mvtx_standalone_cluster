#include <fun4all/Fun4AllServer.h>
#include <fun4all/Fun4AllInputManager.h>
#include <fun4all/Fun4AllDstInputManager.h>
#include <fun4all/Fun4AllDstOutputManager.h>

#include <G4Setup_sPHENIX.C>
#include <G4_ActsGeom.C>

#include <Trkr_Clustering.C>
#include <Trkr_Reco.C>
#include <Trkr_RecoInit.C>

#include <ffamodules/CDBInterface.h>
#include <ffamodules/FlagHandler.h>
#include <ffamodules/HeadReco.h>
#include <ffamodules/SyncReco.h>

#include <fun4all/Fun4AllUtils.h>

#include <mvtx/MvtxCombinedRawDataDecoder.h>
#include <mvtx_standalone_cluster/mvtx_standalone_cluster.h> 

#include <phool/recoConsts.h>

R__LOAD_LIBRARY(libfun4all.so)
R__LOAD_LIBRARY(libmvtxstandalonecluster.so)
R__LOAD_LIBRARY(libmvtx.so)

void Fun4All_tester(const std::string file = "/sphenix/u/ycmorales/work/sphenix/Fun4All/Run25332/mvtxCombinerOutput.root")
{
  Fun4AllServer *se = Fun4AllServer::instance();
  se->Verbosity(1);
  recoConsts *rc = recoConsts::instance();

  Fun4AllInputManager *infile = new Fun4AllDstInputManager("DSTin");
  infile->AddFile(file.c_str());
  se->registerInputManager(infile);

  Enable::CDB = true;
  // global tag
  rc->set_StringFlag("CDB_GLOBALTAG", CDB::global_tag);
  // 64 bit timestamp
  rc->set_uint64Flag("TIMESTAMP", CDB::timestamp);

 /*
  pair<int, int> runseg = Fun4AllUtils::GetRunSegment(outputFile);
  int runnumber = runseg.first;
  int segment = runseg.second;
  if (runnumber != 0) 
  {
    rc->set_IntFlag("RUNNUMBER", runnumber);
    Fun4AllSyncManager *syncman = se->getSyncManager();
    syncman->SegmentNumber(segment);
  }
*/
  MvtxCombinedRawDataDecoder *myUnpacker = new MvtxCombinedRawDataDecoder();
  myUnpacker->Verbosity(0);
  se->registerSubsystem(myUnpacker);

  Enable::MVTX = true;
  Enable::INTT = true;
  Enable::TPC = true;
  Enable::MICROMEGAS = true;

  G4Init();
  G4Setup();

  TrackingInit();
  Mvtx_Clustering();

  mvtx_standalone_cluster *myTester = new mvtx_standalone_cluster();
  se->registerSubsystem(myTester);
 
  Fun4AllDstOutputManager *out = new Fun4AllDstOutputManager("DSTOUT", "outputDST.root");
  se->registerOutputManager(out);
 
  se->run(100);

  se->End();
  delete se;
  gSystem->Exit(0);
}
