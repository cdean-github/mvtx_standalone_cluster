#include <fun4all/Fun4AllServer.h>
#include <fun4all/Fun4AllDstInputManager.h>
#include <fun4all/Fun4AllInputManager.h>
#include <fun4all/Fun4AllOutputManager.h>
#include <fun4all/Fun4AllDstOutputManager.h>

#include <fun4allraw/Fun4AllStreamingInputManager.h>
#include <fun4allraw/InputManagerType.h>

#include <ffamodules/CDBInterface.h>
#include <ffamodules/FlagHandler.h>
#include <ffamodules/HeadReco.h>
#include <ffamodules/SyncReco.h>
#include <Trkr_RecoInit.C>
#include "G4Setup_sPHENIX.C"
#include <event_display_maker/mvtx_standalone_cluster.h>

#include <phool/recoConsts.h>

#include <array>

R__LOAD_LIBRARY(libfun4all.so)
R__LOAD_LIBRARY(libfun4allraw.so)
R__LOAD_LIBRARY(libeventdisplaymaker.so)
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

void Fun4All_writeHit(int nEvents = 0, std::string fileList = "dummy.list")
{
  Fun4AllServer *se = Fun4AllServer::instance();
  se->Verbosity(1);

  std::string run_number_command = "echo \"$(echo "; run_number_command += fileList; run_number_command += " | cut -d \"_\" -f4)\"";
  std::string run_number = exec(run_number_command.c_str());

  std::string file_number_command = "echo \"$(echo "; file_number_command += fileList; file_number_command += " | cut -d \"_\" -f5 | cut -d \".\" -f1)\"";
  std::string file_number = exec(file_number_command.c_str());

  string outpath = ".";
  string outtrailer = "MVTX_clusters_" + run_number.substr(0, run_number.size()-1) + "_" + file_number.substr(0, file_number.size()-1) + ".root";

  recoConsts *rc = recoConsts::instance();
  Enable::CDB = true;
  rc->set_StringFlag("CDB_GLOBALTAG", CDB::global_tag);
  rc->set_uint64Flag("TIMESTAMP", std::stoi(run_number));
  rc->set_IntFlag("RUNNUMBER", std::stoi(run_number));

  Fun4AllInputManager *infile = new Fun4AllDstInputManager("DSTin");
  infile->AddFile(fileList);
  se->registerInputManager(infile);

  Enable::MVTX = true;
  Enable::INTT = true;
  Enable::TPC = true;
  Enable::MICROMEGAS = true;

  G4Init();
  G4Setup();

  TrackingInit();
  
  mvtx_standalone_cluster *myTester = new mvtx_standalone_cluster();
  myTester->writeFile(outpath + "/TTree_" + outtrailer);
  myTester->writeEventDisplays(false);
  myTester->setEventDisplayPath(".");
  se->registerSubsystem(myTester);

  se->run(nEvents);

  se->End();
  delete se;
  gSystem->Exit(0);
}
