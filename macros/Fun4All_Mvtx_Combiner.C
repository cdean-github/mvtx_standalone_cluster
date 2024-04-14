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
#include <mvtx/MvtxCombinedRawDataDecoder.h>
#include <Trkr_Clustering.C>
#include <Trkr_Reco.C>
#include <Trkr_RecoInit.C>
#include "G4Setup_sPHENIX.C"
#include <mvtx_standalone_cluster/mvtx_standalone_cluster.h>

#include <phool/recoConsts.h>

#include <array>

R__LOAD_LIBRARY(libfun4all.so)
R__LOAD_LIBRARY(libfun4allraw.so)
R__LOAD_LIBRARY(libmvtxstandalonecluster.so)
R__LOAD_LIBRARY(libmvtx.so)

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

void Fun4All_Mvtx_Combiner(int nEvents = 0,
		           const string &input_file00 = "mvtx-flx0.list",
		           const string &input_file01 = "mvtx-flx1.list",
		           const string &input_file02 = "mvtx-flx2.list",
		           const string &input_file03 = "mvtx-flx3.list",
		           const string &input_file04 = "mvtx-flx4.list",
		           const string &input_file05 = "mvtx-flx5.list")
{
  bool runTrkrHits = true;
  bool runTkrkClus = true;
  bool writeOutputDST = false;
  bool stripRawHit = true;

  vector<string> infiles;
  infiles.push_back(input_file00);
  infiles.push_back(input_file01);
  infiles.push_back(input_file02);
  infiles.push_back(input_file03);
  infiles.push_back(input_file04);
  infiles.push_back(input_file05);

  Fun4AllServer *se = Fun4AllServer::instance();
  se->Verbosity(1);

  recoConsts *rc = recoConsts::instance();
  Enable::CDB = true;
  std::string executable_command = "echo \"$(head -n 1 "; executable_command += input_file00; executable_command += "  | awk '{print $1}' | cut -d \"-\" -f2)\"";
  std::string run_number = exec(executable_command.c_str());
  rc->set_StringFlag("CDB_GLOBALTAG", CDB::global_tag);
  rc->set_uint64Flag("TIMESTAMP", std::stoi(run_number));
  rc->set_IntFlag("RUNNUMBER", std::stoi(run_number));

  Fun4AllStreamingInputManager *in = new Fun4AllStreamingInputManager("Comb");
  in->Verbosity(1);
  int i = 0;
  for (auto& infile : infiles)
  {
    auto* sngl= new SingleMvtxPoolInput("MVTX_FLX" + to_string(i));
    sngl->Verbosity(0);
    sngl->SetBcoRange(10);
    sngl->AddListFile(infile);
    in->registerStreamingInput(sngl, InputManagerType::MVTX);
    i++;
  }
  se->registerInputManager(in);

  if (runTrkrHits)
  {
    MvtxCombinedRawDataDecoder *myUnpacker = new MvtxCombinedRawDataDecoder("myUnpacker");
    myUnpacker->Verbosity(0);
    se->registerSubsystem(myUnpacker);
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
 
    mvtx_standalone_cluster *myTester = new mvtx_standalone_cluster();
    myTester->writeEventDisplays(true);
    myTester->setEventDisplayPath("/gpfs/mnt/gpfs02/sphenix/user/cdean/public/MVTX_run24_cosmics_fun4AllDecoder");
    se->registerSubsystem(myTester);
  }

  if (writeOutputDST)
  {
    std::string outputFile = "DST_mvtx_hits_" + run_number + ".root";
    Fun4AllOutputManager *out = new Fun4AllDstOutputManager("out", outputFile);
    if (stripRawHit)
    {
      out->StripNode("MVTXRAWHIT");
    }
    se->registerOutputManager(out);
  }

  se->run(nEvents);

  se->End();
  delete se;
  gSystem->Exit(0);
}
