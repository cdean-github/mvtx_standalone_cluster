#include "silicon_detector_analyser.h"

#include <fun4all/Fun4AllReturnCodes.h>

#include <phool/getClass.h>
#include <phool/PHCompositeNode.h>
#include <phool/PHNodeIterator.h>
#include <phool/recoConsts.h>

#include <boost/format.hpp>
#include <boost/math/special_functions/sign.hpp>
// Reference: https://github.com/sPHENIX-Collaboration/analysis/blob/master/TPC/DAQ/macros/prelimEvtDisplay/TPCEventDisplay.C
//____________________________________________________________________________..
silicon_detector_analyser::silicon_detector_analyser(const std::string &name):
 SubsysReco(name)
{
}

//____________________________________________________________________________..
silicon_detector_analyser::~silicon_detector_analyser()
{
}

//____________________________________________________________________________..
int silicon_detector_analyser::Init(PHCompositeNode *topNode)
{
  return Fun4AllReturnCodes::EVENT_OK;
}
//____________________________________________________________________________..
int silicon_detector_analyser::process_event(PHCompositeNode *topNode)
{
  ++event;
  PHNodeIterator dstiter(topNode);

  recoConsts *rc = recoConsts::instance();
  m_runNumber = rc->get_IntFlag("RUNNUMBER");

  PHCompositeNode* dstNode = dynamic_cast<PHCompositeNode *>(dstiter.findFirst("PHCompositeNode", "DST"));
  if (!dstNode)
  {
    std::cout << __FILE__ << "::" << __func__ << " - DST Node missing, doing nothing." << std::endl;
    exit(1);
  }

  trkrHitSetContainer = findNode::getClass<TrkrHitSetContainer>(dstNode, "TRKR_HITSET");
  if (!trkrHitSetContainer)
  {
    std::cout << __FILE__ << "::" << __func__ << " - TRKR_HITSET  missing, doing nothing." << std::endl;
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

  geantGeomMvtx = findNode::getClass<PHG4CylinderGeomContainer>(topNode, "CYLINDERGEOM_MVTX");
  if (!geantGeomMvtx)
  {
    std::cout << __FILE__ << "::" << __func__ << " - CYLINDERGEOM_MVTX missing, doing nothing." << std::endl;
    exit(1);
  }

  geantGeomIntt = findNode::getClass<PHG4CylinderGeomContainer>(topNode, "CYLINDERGEOM_INTT");
  if (!geantGeomIntt)
  {
    std::cout << __FILE__ << "::" << __func__ << " - CYLINDERGEOM_INTT missing, doing nothing." << std::endl;
    exit(1);
  }
/*
  geantGeomTpc = findNode::getClass<PHG4TpcCylinderGeomContainer>(topNode, "CYLINDERCELLGEOM_SVTX");
  if (!geantGeomTpc)
  {
    std::cout << __FILE__ << "::" << __func__ << " - CYLINDERCELLGEOM_SVTX missing, doing nothing." << std::endl;
    exit(1);
  }

  geantGeomTpot = findNode::getClass<PHG4CylinderGeomContainer>(topNode, "CYLINDERGEOM_MICROMEGAS_FULL");
  if (!geantGeomTpot)
  {
    std::cout << __FILE__ << "::" << __func__ << " - CYLINDERGEOM_MICROMEGAS_FULL missing, doing nothing." << std::endl;
    exit(1);
  }
*/
  mvtx_event_header = findNode::getClass<MvtxEventInfo>(topNode, "MVTXEVENTHEADER");
  if (!mvtx_event_header)
  {
    std::cout << __FILE__ << "::" << __func__ << " - MVTXEVENTHEADER missing, doing nothing." << std::endl;
    exit(1);
  }

  std::set<uint64_t> strobeList = mvtx_event_header->get_strobe_BCOs();
  for (auto iterStrobe = strobeList.begin(); iterStrobe != strobeList.end(); ++iterStrobe)
  {
    std::set<uint64_t> l1List = mvtx_event_header->get_L1_BCO_from_strobe_BCO(*iterStrobe);
    for (auto iterL1 = l1List.begin(); iterL1 != l1List.end(); ++iterL1)
    {
      L1_BCOs.push_back(*iterL1); 
    }
  }

  SvtxTrackMap* trackMap = findNode::getClass<SvtxTrackMap>(topNode, "SvtxTrackMap");
  if (!trackMap)
  {
    std::cout << __FILE__ << "::" << __func__ << " - SvtxTrackMap missing, doing nothing." << std::endl;
    exit(1);
  }

  SvtxVertexMap* vertexMap = findNode::getClass<SvtxVertexMap>(topNode, "SvtxVertexMap");
  if (!trackMap)
  {
    std::cout << __FILE__ << "::" << __func__ << " - SvtxVertexMap missing, doing nothing." << std::endl;
    exit(1);
  }

  numberL1s = mvtx_event_header->get_number_L1s();
  layer = 0;
  global[0] = 0.;
  global[1] = 0.;
  global[2] = 0.;
  //int count_MVTXClusters = 0, count_INTTClusters = 0;
  if (numberL1s == 0)
  {
    return Fun4AllReturnCodes::ABORTEVENT;
  }
  //Set up the event display writer
  std::ofstream outFile;
  bool firstHits = true;

  uint64_t triggerBCO = L1_BCOs.size() > 0 ? L1_BCOs[0] : event;
  std::string outFileName = m_evt_display_path + "/EvtDisplay_" + std::to_string(m_runNumber) + "_" + std::to_string(triggerBCO) + ".json";
  outFile.open(outFileName.c_str());
  m_run_date = getDate();
  event_file_start(outFile, m_run_date, m_runNumber, triggerBCO); 
/*
  TrkrHitSetContainer::ConstRange hitsetrange = trkrHitSetContainer->getHitSets();

  for (TrkrHitSetContainer::ConstIterator hitsetitr = hitsetrange.first; hitsetitr != hitsetrange.second; ++hitsetitr)
  {
    TrkrHitSet *hitset = hitsetitr->second;
    auto hitsetkey = hitset->getHitSetKey();

    layer = TrkrDefs::getLayer(hitsetkey);

    TVector2 LocalUse;
    TVector3 ClusterWorld;

    TrkrClusterContainer::ConstRange clusterrange = trktClusterContainer->getClusters(hitsetkey);
    for (TrkrClusterContainer::ConstIterator clusteritr = clusterrange.first; clusteritr != clusterrange.second; ++clusteritr)
    {
      TrkrCluster *cluster = clusteritr->second;

      localX = cluster->getLocalX();
      localY = cluster->getLocalY();

      LocalUse.SetX(localX);
      LocalUse.SetY(localY);
      
      if (0 <= layer && layer <= 2)
      {
        auto surface = actsGeom->maps().getSiliconSurface(hitsetkey);
        auto layergeom = dynamic_cast<CylinderGeom_Mvtx *>(geantGeomMvtx->GetLayerGeom(layer));
        ClusterWorld = layergeom->get_world_from_local_coords(surface, actsGeom, LocalUse);
        ++count_MVTXClusters;
      }
      else if (3 <= layer && layer <= 6)
      {
        auto surface = actsGeom->maps().getSiliconSurface(hitsetkey);
        auto layergeom = dynamic_cast<CylinderGeomIntt *>(geantGeomIntt->GetLayerGeom(layer));
        ClusterWorld = layergeom->get_world_from_local_coords(surface, actsGeom, LocalUse);
        ++count_INTTClusters;
      }
      else
      {
        continue; //Not a cluster within the silicon volume
      }

      global[0] = ClusterWorld.X();
      global[1] = ClusterWorld.Y();
      global[2] = ClusterWorld.Z();

      bool isMVTX = (0 <= layer && layer <= 2) ? true : false;
      if (global[0] < 0) addHit(outFile, firstHits, global);
    }
  }
*/

  for (auto& iter : *trackMap)
  {
    TrackSeed* silseed = iter.second->get_silicon_seed(); 
    TVector2 LocalUse;
    TVector3 ClusterWorld;

    for (SvtxTrack::ConstClusterKeyIter iter_local = silseed->begin_cluster_keys();
     iter_local != silseed->end_cluster_keys();
     ++iter_local)
    {
      TrkrDefs::cluskey cluster_key = *iter_local;
      layer = TrkrDefs::getLayer(cluster_key);

      TrkrCluster *cluster = trktClusterContainer->findCluster(cluster_key);

      localX = cluster->getLocalX();
      localY = cluster->getLocalY();

      LocalUse.SetX(localX);
      LocalUse.SetY(localY);

      auto surface = actsGeom->maps().getSurface(cluster_key, cluster);

      switch (TrkrDefs::getTrkrId(cluster_key))
      {
        case TrkrDefs::TrkrId::mvtxId:
        {
          CylinderGeom_Mvtx* layergeom = dynamic_cast<CylinderGeom_Mvtx *>(geantGeomMvtx->GetLayerGeom(layer));
          ClusterWorld = layergeom->get_world_from_local_coords(surface, actsGeom, LocalUse);
          break;
        }
        case TrkrDefs::TrkrId::inttId:
        {
          CylinderGeomIntt* layergeom = dynamic_cast<CylinderGeomIntt *>(geantGeomIntt->GetLayerGeom(layer));
          ClusterWorld = layergeom->get_world_from_local_coords(surface, actsGeom, LocalUse);
          break;
        }
        default:
          break;
      }
      
      global[0] = ClusterWorld.X();
      global[1] = ClusterWorld.Y();
      global[2] = ClusterWorld.Z();

      addHit(outFile, firstHits, global);
    }
  }

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
  if (vertexMap->size() == 0)
  {
    std::remove(outFileName.c_str());
  }

  if ((nTracks >= m_min_tracks) && vertexMap->size() > 0)
  {
    std::cout << "For BCO " << std::to_string(triggerBCO) << " you have " << nTracks << " tracks, and " << vertexMap->size() << " vertices" << std:: endl;
  } 

  L1_BCOs.clear();

  return Fun4AllReturnCodes::EVENT_OK;
}


//____________________________________________________________________________..
int silicon_detector_analyser::End(PHCompositeNode *topNode)
{
  return Fun4AllReturnCodes::EVENT_OK;
}

//____________________________________________________________________________..
int silicon_detector_analyser::Reset(PHCompositeNode *topNode)
{
  return Fun4AllReturnCodes::EVENT_OK;
}

//____________________________________________________________________________..
void silicon_detector_analyser::Print(const std::string &what) const
{
}


void silicon_detector_analyser::event_file_start(std::ofstream &json_file_header, std::string date, int runid, uint64_t bco)
{
  json_file_header << "{" << std::endl;
  json_file_header << "    \"EVENT\": {" << std::endl;
  json_file_header << "        \"runid\":" << runid << ", " << std::endl;
  json_file_header << "        \"evtid\": 1, " << std::endl;
  json_file_header << "        \"time\": 0, " << std::endl;
  json_file_header << "       \"type\": \"Collision\", " << std::endl;
  json_file_header << "        \"s_nn\": 0, " << std::endl;
  json_file_header << "        \"B\": 3.0," << std::endl;
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
  json_file_header << "              \"color\": 16777215" << std::endl;
  json_file_header << "              } " << std::endl;
  json_file_header << "          }," << std::endl;
  json_file_header << "" << std::endl;
  json_file_header << "          \"TRACKHITS\": {" << std::endl;
  json_file_header << "              \"type\": \"3D\"," << std::endl;
  json_file_header << "              \"options\": {" << std::endl;
  json_file_header << "              \"size\": 1," << std::endl;
  json_file_header << "              \"transparent\": 1," << std::endl;
  json_file_header << "              \"color\": 16073282" << std::endl;
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
  json_file_header << "    \"TRACKHITS\": [" << std::endl;
  json_file_header << "" << std::endl;
  json_file_header << " ";
}

void silicon_detector_analyser::event_file_trailer_1(std::ofstream &json_file_trailer)
{
  json_file_trailer << "]," << std::endl;
  json_file_trailer << "    \"JETS\": [" << std::endl;
  json_file_trailer << "         ]" << std::endl;
  json_file_trailer << "    }," << std::endl;
  json_file_trailer << "\"TRACKS\": {" << std::endl;
  json_file_trailer << "   \"B\": 0.000014," << std::endl;
  json_file_trailer << "   \"TRACKHITS\": [" << std::endl;
}

void silicon_detector_analyser::event_file_trailer_2(std::ofstream &json_file_trailer)
{
  json_file_trailer << "   ]" << std::endl;
  json_file_trailer << "}" << std::endl;
  json_file_trailer << "}" << std::endl;
}

void silicon_detector_analyser::addHit(std::ofstream &json_file_hit, bool &firstHit, float hit[3])
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
  spts << hit[0];
  spts << ", \"y\": ";
  spts << hit[1];
  spts << ", \"z\": ";
  spts << hit[2];
  spts << ", \"e\": 0}";
  json_file_hit << (boost::format("%1%") % spts.str());
  spts.clear();
  spts.str("");
}

void silicon_detector_analyser::addTrack(std::ofstream &json_file_track, SvtxTrack* aTrack, bool lastTrack)
{
  std::string addComma = lastTrack ? "     }" : "     },";

  float x = aTrack->get_x();
  float y = aTrack->get_y();
  float z = aTrack->get_z();
  float px = aTrack->get_silicon_seed()->get_px();
  float py = aTrack->get_silicon_seed()->get_py();
  float pz = aTrack->get_silicon_seed()->get_pz();
  int charge = aTrack->get_charge();

  json_file_track << "     {" << std::endl;
  json_file_track << "       \"color\": 16777215," << std::endl;
  json_file_track << "       \"l\": 20," << std::endl;//" << length << "," << std::endl;
  json_file_track << "       \"nh\": 6," << std::endl;
  json_file_track << "       \"pxyz\": [" << std::endl;
  json_file_track << "        " << px << ","<< std::endl;// " << deltaX*scale << "," << std::endl;
  json_file_track << "        " << py << ","<< std::endl;//" << deltaY*scale << "," << std::endl;
  json_file_track << "        " << pz << ""<< std::endl;// " << deltaZ*scale << std::endl;
  json_file_track << "       ]," << std::endl;
  json_file_track << "       \"q\": " << charge << "," << std::endl;
  json_file_track << "       \"xyz\": [" << std::endl;
  json_file_track << "         " << x << ","<< std::endl;//" << maxHit[0] << ", " << std::endl;
  json_file_track << "          " << y << ","<< std::endl;//" << maxHit[1] << ", " << std::endl;
  json_file_track << "         " << z << ""<< std::endl;//" << maxHit[2] << std::endl;
  json_file_track << "       ]" << std::endl;
  json_file_track << addComma << std::endl;
}

//https://stackoverflow.com/questions/997946/how-to-get-current-time-and-date-in-c
std::string silicon_detector_analyser::getDate()
{
    std::time_t t = std::time(0);   // get time now
    std::tm* now = std::localtime(&t);
    
    std::stringstream date;
    date << (now->tm_year + 1900) << '-' 
         << (now->tm_mon + 1) << '-'
         <<  now->tm_mday;
    return date.str();
}
