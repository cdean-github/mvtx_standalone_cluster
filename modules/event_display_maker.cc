#include "event_display_maker.h"

#include <fun4all/Fun4AllReturnCodes.h>

#include <phool/getClass.h>
#include <phool/PHCompositeNode.h>
#include <phool/PHNodeIterator.h>
#include <phool/recoConsts.h>

#include <boost/format.hpp>
#include <boost/math/special_functions/sign.hpp>
// Reference: https://github.com/sPHENIX-Collaboration/analysis/blob/master/TPC/DAQ/macros/prelimEvtDisplay/TPCEventDisplay.C
//____________________________________________________________________________..
event_display_maker::event_display_maker(const std::string &name):
 SubsysReco(name)
{
}

//____________________________________________________________________________..
event_display_maker::~event_display_maker()
{
}

//____________________________________________________________________________..
int event_display_maker::Init(PHCompositeNode *topNode)
{
  return Fun4AllReturnCodes::EVENT_OK;
}
//____________________________________________________________________________..
int event_display_maker::process_event(PHCompositeNode *topNode)
{
  PHNodeIterator dstiter(topNode);

  recoConsts *rc = recoConsts::instance();
  m_runNumber = rc->get_IntFlag("RUNNUMBER");

  PHCompositeNode* dstNode = dynamic_cast<PHCompositeNode *>(dstiter.findFirst("PHCompositeNode", "DST"));
  if (!dstNode)
  {
    std::cout << __FILE__ << "::" << __func__ << " - DST Node missing, doing nothing." << std::endl;
    exit(1);
  }

  trktClusterContainer = findNode::getClass<TrkrClusterContainer>(dstNode, "TRKR_CLUSTER");
  if (!trktClusterContainer)
  {
    std::cout << __FILE__ << "::" << __func__ << " - TRKR_CLUSTER missing, doing nothing." << std::endl;
    exit(1);
  }

  actsGeom = findNode::getClass<ActsGeometry>(topNode, "ActsGeometry");
  if (!actsGeom)
  {
    std::cout << __FILE__ << "::" << __func__ << " - ActsGeometry missing, doing nothing." << std::endl;
    exit(1);
  }

  gl1info = findNode::getClass<Gl1RawHit>(topNode, "GL1RAWHIT");
  if (!gl1info)
  {
    std::cout << __FILE__ << "::" << __func__ << " - GL1RAWHIT missing, doing nothing." << std::endl;
    exit(1);
  }

  triggerBCO = gl1info->get_bco(); 

  trackMap = findNode::getClass<SvtxTrackMap>(topNode, "SvtxTrackMap");
  if (!trackMap)
  {
    std::cout << __FILE__ << "::" << __func__ << " - SvtxTrackMap missing, doing nothing." << std::endl;
    exit(1);
  }

  if (trackMap->size() == 0)
  {
    return Fun4AllReturnCodes::ABORTEVENT;
  }

  if (m_require_vertex)
  {
    vertexMap = findNode::getClass<SvtxVertexMap>(topNode, "SvtxVertexMap");
    if (!vertexMap)
    {
      std::cout << __FILE__ << "::" << __func__ << " - SvtxVertexMap missing, doing nothing." << std::endl;
      exit(1);
    }

    if (vertexMap->size() == 0)
    {
      return Fun4AllReturnCodes::ABORTEVENT;
    }
  }

  //Set up the event display writer
  std::string outFileName = m_evt_display_path + "/EvtDisplay_" + std::to_string(m_runNumber) + "_" + std::to_string(triggerBCO) + ".json";
  outFile.open(outFileName.c_str());
  m_run_date = getDate();
  event_file_start(outFile, m_run_date, m_runNumber, triggerBCO); 

  //First add the silicon clusters
  loopTrackClusters(true);

  event_file_trailer_0(outFile);

  //Now go back and add the TPC clusters
  loopTrackClusters(false);

  event_file_trailer_1(outFile);
  unsigned int nTracks = trackMap->size();
  unsigned int counter = 0;
  for (auto& iter : *trackMap)
  {
    ++counter;
    bool isLastTrack = counter == nTracks ? true : false;
    addTrack(outFile, iter.second, isLastTrack);
  }
  event_file_trailer_2(outFile);
  outFile.close();

  ++evtDisplayCounter;
  if (evtDisplayCounter > maxEvtDisplays)
  {
    std::remove(outFileName.c_str());
  }

  return Fun4AllReturnCodes::EVENT_OK;
}


//____________________________________________________________________________..
int event_display_maker::End(PHCompositeNode *topNode)
{
  return Fun4AllReturnCodes::EVENT_OK;
}

//____________________________________________________________________________..
int event_display_maker::Reset(PHCompositeNode *topNode)
{
  return Fun4AllReturnCodes::EVENT_OK;
}

//____________________________________________________________________________..
void event_display_maker::Print(const std::string &what) const
{
}

void event_display_maker::loopTrackClusters(bool isSilicon)
{
  bool firstHits = true;
  for (auto& iter : *trackMap)
  {
    for (auto state_iter = iter.second->begin_states();
         state_iter != iter.second->end_states();
         ++state_iter)
    {
      SvtxTrackState* tstate = state_iter->second;
      if (tstate->get_pathlength() != 0) //The first track state is an extrapolation so has no cluster
      {
        auto stateckey = tstate->get_cluskey();
        uint8_t layer = TrkrDefs::getLayer(stateckey);

        int maxSiliconLayer = 6;
        bool writeHit = false;

        if (isSilicon)
        {
          writeHit = layer <= maxSiliconLayer ? true : false;
        }
        else
        {
          writeHit = layer > maxSiliconLayer ? true : false;
        }
 
        if (writeHit)
        {
          TrkrCluster *cluster = trktClusterContainer->findCluster(stateckey);
          Acts::Vector3 global = actsGeom->getGlobalPosition(stateckey, cluster);      
          addHit(outFile, firstHits, global);
        }
      }
    }
  }
}

void event_display_maker::event_file_start(std::ofstream &json_file_header, std::string date, int runid, uint64_t bco)
{
  json_file_header << "{" << std::endl;
  json_file_header << "    \"EVENT\": {" << std::endl;
  json_file_header << "        \"runid\":" << runid << ", " << std::endl;
  json_file_header << "        \"evtid\": 1, " << std::endl;
  json_file_header << "        \"time\": 0, " << std::endl;
  json_file_header << "       \"type\": \"Collision\", " << std::endl;
  json_file_header << "        \"s_nn\": 0, " << std::endl;
  json_file_header << "        \"B\": 1.38," << std::endl;
  json_file_header << "        \"pv\": [0,0,0]," << std::endl;
  json_file_header << " \"runstats\": [\"sPHENIX Internal\"," << std::endl;
  json_file_header << "\"200 GeV pp\"," << std::endl;
  json_file_header << "\"" << date << ", Run " << runid << "\"," << std::endl;
  json_file_header << "\"BCO: " << bco <<"\"]  " << std::endl;
  json_file_header << "    }," << std::endl;
  json_file_header << "" << std::endl;
  json_file_header << "    \"META\": {" << std::endl;
  json_file_header << "       \"HITS\": {" << std::endl;
  json_file_header << "          \"INNERTRACKER\": {" << std::endl;
  json_file_header << "              \"type\": \"3D\"," << std::endl;
  json_file_header << "              \"options\": {" << std::endl;
  json_file_header << "              \"size\": 5," << std::endl;
  json_file_header << "              \"color\": 16711680" << std::endl;
  json_file_header << "              } " << std::endl;
  json_file_header << "          }," << std::endl;
  json_file_header << "" << std::endl;
  json_file_header << "          \"TRACKHITS\": {" << std::endl;
  json_file_header << "              \"type\": \"3D\"," << std::endl;
  json_file_header << "              \"options\": {" << std::endl;
  json_file_header << "              \"size\": 2," << std::endl;
  json_file_header << "              \"transparent\": 1," << std::endl;
  json_file_header << "              \"color\": 16446450" << std::endl;
  json_file_header << "              } " << std::endl;
  json_file_header << "          }," << std::endl;
  json_file_header << "" << std::endl;
  json_file_header << "    \"JETS\": {" << std::endl;
  json_file_header << "        \"type\": \"JET\"," << std::endl;
  json_file_header << "        \"options\": {" << std::endl;
  json_file_header << "            \"rmin\": 0," << std::endl;
  json_file_header << "            \"rmax\": 78," << std::endl;
  json_file_header << "            \"emin\": 0," << std::endl;
  json_file_header << "            \"emax\": 30," << std::endl;
  json_file_header << "            \"color\": 16777215," << std::endl;
  json_file_header << "            \"transparent\": 0.5 " << std::endl;
  json_file_header << "        }" << std::endl;
  json_file_header << "    }" << std::endl;
  json_file_header << "        }" << std::endl;
  json_file_header << "    }" << std::endl;
  json_file_header << "," << std::endl;
  json_file_header << "    \"HITS\": {" << std::endl;
  json_file_header << "        \"CEMC\":[{\"eta\": 0, \"phi\": 0, \"e\": 0}" << std::endl;
  json_file_header << "            ]," << std::endl;
  json_file_header << "        \"HCALIN\": [{\"eta\": 0, \"phi\": 0, \"e\": 0}" << std::endl;
  json_file_header << "            ]," << std::endl;
  json_file_header << "        \"HCALOUT\": [{\"eta\": 0, \"phi\": 0, \"e\": 0}" << std::endl;
  json_file_header << " " << std::endl;
  json_file_header << "            ]," << std::endl;
  json_file_header << "" << std::endl;
  json_file_header << "" << std::endl;
  json_file_header << "    \"INNERTRACKER\": [" << std::endl;
  json_file_header << " ";
}

void event_display_maker::event_file_trailer_0(std::ofstream &json_file_trailer)
{
  json_file_trailer << "]," << std::endl;
  json_file_trailer << "    \"TRACKHITS\": [" << std::endl;
  json_file_trailer << " ";
}

void event_display_maker::event_file_trailer_1(std::ofstream &json_file_trailer)
{
  json_file_trailer << "]," << std::endl;
  json_file_trailer << "    \"JETS\": [" << std::endl;
  json_file_trailer << "         ]" << std::endl;
  json_file_trailer << "    }," << std::endl;
  json_file_trailer << "\"TRACKS\": {" << std::endl;
  json_file_trailer << "   \"B\": 0.000014," << std::endl;
  json_file_trailer << "   \"TRACKHITS\": [" << std::endl;
}

void event_display_maker::event_file_trailer_2(std::ofstream &json_file_trailer)
{
  json_file_trailer << "   ]" << std::endl;
  json_file_trailer << "}" << std::endl;
  json_file_trailer << "}" << std::endl;
}

void event_display_maker::addHit(std::ofstream &json_file_hit, bool &firstHit, Acts::Vector3 hit)
{
  std::ostringstream spts;
  if (firstHit)
  {
    firstHit = false;
  }
  else
  {
    spts << ",";
  }

  spts << "{ \"x\": ";
  spts << hit.x();
  spts << ", \"y\": ";
  spts << hit.y();
  spts << ", \"z\": ";
  spts << hit.z();
  spts << ", \"e\": 0}";
  json_file_hit << (boost::format("%1%") % spts.str());
  spts.clear();
  spts.str("");
}

void event_display_maker::addTrack(std::ofstream &json_file_track, SvtxTrack* aTrack, bool lastTrack)
{
  std::string addComma = lastTrack ? "     }" : "     },";

  float x = aTrack->get_x();
  float y = aTrack->get_y();
  float z = aTrack->get_z();
  float px = aTrack->get_px();
  float py = aTrack->get_py();
  float pz = aTrack->get_pz();
  int charge = aTrack->get_charge();
  float length = 0;

  for (auto state_iter = aTrack->begin_states();
       state_iter != aTrack->end_states();
       ++state_iter)
  {
    SvtxTrackState* tstate = state_iter->second;
    length = tstate->get_pathlength();

  }

  json_file_track << "     {" << std::endl;
  json_file_track << "       \"color\": 16776960," << std::endl;
  json_file_track << "       \"l\": " << length << "," << std::endl;
  json_file_track << "       \"nh\": 6," << std::endl;
  json_file_track << "       \"pxyz\": [" << std::endl;
  json_file_track << "        " << px << ","<< std::endl;
  json_file_track << "        " << py << ","<< std::endl;
  json_file_track << "        " << pz << ""<< std::endl;
  json_file_track << "       ]," << std::endl;
  json_file_track << "       \"q\": " << charge << "," << std::endl;
  json_file_track << "       \"xyz\": [" << std::endl;
  json_file_track << "         " << x << ","<< std::endl;
  json_file_track << "          " << y << ","<< std::endl;
  json_file_track << "         " << z << ""<< std::endl;
  json_file_track << "       ]" << std::endl;
  json_file_track << addComma << std::endl;
}

//https://stackoverflow.com/questions/997946/how-to-get-current-time-and-date-in-c
std::string event_display_maker::getDate()
{
    std::time_t t = std::time(0);   // get time now
    std::tm* now = std::localtime(&t);
    
    std::stringstream date;
    date << (now->tm_year + 1900) << '-' 
         << (now->tm_mon + 1) << '-'
         <<  now->tm_mday;
    return date.str();
}
