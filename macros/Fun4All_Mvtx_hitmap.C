#pragma once
#if ROOT_VERSION_CODE >= ROOT_VERSION(6,00,0)

#include <fun4all/Fun4AllServer.h>
#include <fun4all/Fun4AllInputManager.h>
#include <fun4all/Fun4AllOutputManager.h>
#include <fun4all/Fun4AllDstOutputManager.h>

#include <fun4allraw/Fun4AllStreamingInputManager.h>
#include <fun4allraw/InputManagerType.h>
#include <fun4allraw/SingleMvtxPoolInput.h>

#include <ffamodules/CDBInterface.h>
#include <ffamodules/FlagHandler.h>
#include <ffamodules/HeadReco.h>
#include <ffamodules/SyncReco.h>

#include <phool/recoConsts.h>

#include <mvtx/MvtxCombinedRawDataDecoder.h>
#include <event_display_maker/mvtx_hit_map.h>
#include <event_display_maker/mvtx_file_finder.h>

#include <vector>
#include <string>

#include <Trkr_Reco.C>
#include <Trkr_RecoInit.C>

R__LOAD_LIBRARY(libfun4all.so)
R__LOAD_LIBRARY(libfun4allraw.so)
R__LOAD_LIBRARY(libeventdisplaymaker.so)
R__LOAD_LIBRARY(libmvtx.so)

#endif


void Fun4All_Mvtx_hitmap(const int run_number=43999,
  const std::string run_type="physics",
  const int nEvents = 10,
  const std::string output_name = "drop_BCO_diff.root"
  )
{  
  // use filefinder
  mvtx_file_finder * filefinder = new mvtx_file_finder();
  filefinder->SetDataPath("/sphenix/lustre01/sphnxpro/physics/MVTX");
  filefinder->SetRunType(run_type);
  filefinder->SetRunNumber(run_number);
  
  // F4A init
  Fun4AllServer *se = Fun4AllServer::instance();
  se->Verbosity(0);

  recoConsts *rc = recoConsts::instance();
  Enable::CDB = true;
  rc->set_StringFlag("CDB_GLOBALTAG", CDB::global_tag);
  rc->set_uint64Flag("TIMESTAMP",run_number);
  rc->set_IntFlag("RUNNUMBER",  run_number);

  Fun4AllStreamingInputManager *in = new Fun4AllStreamingInputManager("Comb");
  in->Verbosity(1);
  for(unsigned int iflx =0; iflx <6; iflx++)
  {
    auto * sngl = new SingleMvtxPoolInput("MVTX_FLX" + to_string(iflx));
    sngl->Verbosity(0);
    sngl->SetBcoRange(10);
    std::vector<std::string> infiles = filefinder->GetFiles(iflx);
    std::cout << "Adding files for flx " << iflx << std::endl;
    for(auto &infile : infiles)
    {
      std::cout << "\tAdding file: " << infile << std::endl;
      sngl->AddFile(infile);
    }
    in->registerStreamingInput(sngl, InputManagerType::MVTX);
  }
  se->registerInputManager(in);

  MvtxCombinedRawDataDecoder *myUnpacker = new MvtxCombinedRawDataDecoder("myUnpacker");
  myUnpacker->Verbosity(0);
  se->registerSubsystem(myUnpacker);

  mvtx_hit_map *mhm = new mvtx_hit_map("mvtx_file_finder");
  mhm->SetOutputFile(output_name);
  se->registerSubsystem(mhm);


  se->run(nEvents);
  se->End();
  gSystem->Exit(0);
  return 0;

}
