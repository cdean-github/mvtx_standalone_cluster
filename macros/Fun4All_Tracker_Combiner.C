#include <fun4all/Fun4AllServer.h>
#include <fun4all/Fun4AllInputManager.h>
#include <fun4all/Fun4AllOutputManager.h>
#include <fun4all/Fun4AllDstOutputManager.h>

#include <fun4allraw/Fun4AllStreamingInputManager.h>
#include <fun4allraw/InputManagerType.h>
#include <fun4allraw/SingleMvtxPoolInput.h>
#include <fun4allraw/SingleGl1PoolInput.h>
#include <fun4allraw/SingleInttPoolInput.h>
#include <fun4allraw/SingleMicromegasPoolInput.h>
#include <fun4allraw/SingleTpcPoolInput.h>

#include <ffamodules/CDBInterface.h>
#include <ffamodules/FlagHandler.h>
#include <ffamodules/HeadReco.h>
#include <ffamodules/SyncReco.h>
#include <intt/InttCombinedRawDataDecoder.h>
#include <micromegas/MicromegasCombinedDataDecoder.h>
#include <mvtx/MvtxCombinedRawDataDecoder.h>
#include <tpc/TpcCombinedRawDataUnpacker.h>
#include <Trkr_Clustering.C>
#include <Trkr_Reco.C>
#include <Trkr_RecoInit.C>
#include "G4Setup_sPHENIX.C"
#include <event_display_maker/event_display_maker.h>

#include <phool/recoConsts.h>

#include <array>

R__LOAD_LIBRARY(libfun4all.so)
R__LOAD_LIBRARY(libfun4allraw.so)
R__LOAD_LIBRARY(libeventdisplaymaker.so)
R__LOAD_LIBRARY(libintt.so)
R__LOAD_LIBRARY(libmvtx.so)
R__LOAD_LIBRARY(libmicromegas.so)

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

void Fun4All_Tracker_Combiner(int nEvents = 0,
                              vector<string> gl1_infile = {"dummy.list"},
                              vector<string> mvtx_infiles = {"dummy.list"},
                              vector<string> intt_infile = {"dummy.list"},
                              vector<string> tpc_infile = {"dummy.list"},
                              vector<string> tpot_infile = {"dummy.list"})
{
  bool runTrkrHits = true;
  bool runTkrkClus = true;
  bool writeOutputDST = false;
  bool stripRawHit = true;

  Fun4AllServer *se = Fun4AllServer::instance();
  se->Verbosity(1);

  recoConsts *rc = recoConsts::instance();
  Enable::CDB = true;
  std::string executable_command = "echo \"$(head -n 1 "; executable_command += mvtx_infiles[0] ; executable_command += "  | awk '{print $1}' | cut -d \"-\" -f2)\"";
  std::string run_number = exec(executable_command.c_str());
  rc->set_StringFlag("CDB_GLOBALTAG", CDB::global_tag);
  rc->set_uint64Flag("TIMESTAMP", std::stoi(run_number));
  rc->set_IntFlag("RUNNUMBER", std::stoi(run_number));

  Fun4AllStreamingInputManager *in = new Fun4AllStreamingInputManager("Comb");
  in->Verbosity(0);

  int i = 0;
  for (auto& iter : mvtx_infiles)
  {
    auto* mvtx_sngl= new SingleMvtxPoolInput("MVTX_" + to_string(i));
    mvtx_sngl->Verbosity(0);
    mvtx_sngl->SetBcoRange(1000);
    mvtx_sngl->SetNegativeBco(1000);
    mvtx_sngl->AddListFile(iter);
    in->registerStreamingInput(mvtx_sngl, InputManagerType::MVTX);
    i++;
  }

  i = 0;
  for (auto iter : gl1_infile)
  {
    if (isGood(iter))
    {
      SingleGl1PoolInput *gl1_sngl = new SingleGl1PoolInput("GL1_" + to_string(i));
      gl1_sngl->AddListFile(iter);
      in->registerStreamingInput(gl1_sngl, InputManagerType::GL1);
      i++;
    }
  }

  i = 0;
  for (auto iter : intt_infile)
  {
    if (isGood(iter))
    {
      SingleInttPoolInput *intt_sngl = new SingleInttPoolInput("INTT_" + to_string(i));
      intt_sngl->SetNegativeBco(1);
      intt_sngl->SetBcoRange(2);
      intt_sngl->AddListFile(iter);
      in->registerStreamingInput(intt_sngl, InputManagerType::INTT);
      i++;
    }
  }

  i = 0;
  for (auto iter : tpc_infile)
  {
    if (isGood(iter))
    {
    SingleTpcPoolInput *tpc_sngl = new SingleTpcPoolInput("TPC_" + to_string(i));
    tpc_sngl->SetBcoRange(130);
    tpc_sngl->AddListFile(iter);
    in->registerStreamingInput(tpc_sngl, InputManagerType::TPC);
    i++;
    }
  }

  i = 0;
  for (auto iter : tpot_infile)
  {
    if (isGood(iter))
    {
    SingleMicromegasPoolInput *mm_sngl = new SingleMicromegasPoolInput("MICROMEGAS_" + to_string(i));
    mm_sngl->SetBcoRange(100);
    mm_sngl->SetNegativeBco(2);
    mm_sngl->AddListFile(iter);
    in->registerStreamingInput(mm_sngl, InputManagerType::MICROMEGAS);
    i++;
    }
  }

  se->registerInputManager(in);

  if (runTrkrHits)
  {
    MvtxCombinedRawDataDecoder *myMvtxUnpacker = new MvtxCombinedRawDataDecoder("myMvtxUnpacker");
    se->registerSubsystem(myMvtxUnpacker);

    InttCombinedRawDataDecoder *myInttUnpacker = new InttCombinedRawDataDecoder("myInttUnpacker");
    se->registerSubsystem(myInttUnpacker);

    TpcCombinedRawDataUnpacker *myTpcUnpacker = new TpcCombinedRawDataUnpacker("myTpcUnpacker");
    se->registerSubsystem(myTpcUnpacker);

    MicromegasCombinedDataDecoder *myTpotUnpacker = new MicromegasCombinedDataDecoder("myTpotUnpacker");
    myTpotUnpacker->set_calibration_file("/gpfs/mnt/gpfs02/sphenix/user/cdean/software/OnlMon/subsystems/tpot/calib/TPOT_Pedestal-000.root");
    se->registerSubsystem(myTpotUnpacker);
  }

  if (runTkrkClus) 
  {
    Enable::MVTX = true;
    Enable::INTT = true;
    Enable::TPC = true;
    Enable::MICROMEGAS = true;
  
    G4Init();
    G4Setup();

    TrackingInit();
    Mvtx_Clustering();
    Intt_Clustering();
    TPC_Clustering();
    Micromegas_Clustering();
 
    event_display_maker *myTester = new event_display_maker();
    myTester->setEventDisplayPath("/sphenix/user/cdean/public/run24_cosmic_event_displays");
    myTester->minMVTXClusters(3);
    myTester->minINTTClusters(2);
    myTester->minTPCHits(1000);
    myTester->minTPOTClusters(1);
    se->registerSubsystem(myTester);
  }

  if (writeOutputDST)
  {
    std::string outputFile = "DST_tracker_hits_" + run_number + ".root";
    Fun4AllOutputManager *out = new Fun4AllDstOutputManager("out", outputFile);
    if (stripRawHit)
    {
      out->StripNode("MVTXRAWHIT");
      out->StripNode("INTTRAWHIT");
      out->StripNode("TPCRAWHIT");
      out->StripNode("MICROMEGASRAWHIT");
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
