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

  numberL1s = mvtx_event_header->get_number_L1s();
  layer = 0;
  global[0] = 0.;
  global[1] = 0.;
  global[2] = 0.;
  int count_MVTXClusters = 0, count_INTTClusters = 0, count_TPCHits = 0, count_TPOTClusters = 0;
  if (numberL1s == 0)
  {
    return Fun4AllReturnCodes::ABORTEVENT;
  }
  //Set up the event display writer
  std::ofstream outFile;
  bool firstHits = true;
  float minHit[3] = {10.}, maxHit[3] = {-10.};

  uint64_t triggerBCO = L1_BCOs.size() > 0 ? L1_BCOs[0] : event;
  std::string outFileName = m_evt_display_path + "/EvtDisplay_" + std::to_string(m_runNumber) + "_" + std::to_string(triggerBCO) + ".json";
  outFile.open(outFileName.c_str());
  m_run_date = getDate();
  event_file_start(outFile, m_run_date, m_runNumber, triggerBCO); 

  TrkrHitSetContainer::ConstRange hitsetrange = trkrHitSetContainer->getHitSets();

  for (TrkrHitSetContainer::ConstIterator hitsetitr = hitsetrange.first; hitsetitr != hitsetrange.second; ++hitsetitr)
  {
    TrkrHitSet *hitset = hitsetitr->second;
    auto hitsetkey = hitset->getHitSetKey();

    layer = TrkrDefs::getLayer(hitsetkey);

    if (layer < 7 || 54 < layer) //Outside the TPC volume
    {
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
        else if (55 <= layer && layer <= 56)
        {
          uint8_t tileID = MicromegasDefs::getTileId(hitsetkey);
          auto layergeom = dynamic_cast<CylinderGeomMicromegas *>(geantGeomTpot->GetLayerGeom(layer));
          ClusterWorld = layergeom->get_world_from_local_coords(tileID, actsGeom, LocalUse);
          ++count_TPOTClusters;
        }
        else
        {
          continue; //Not a cluster within the tracking volume
        }

        global[0] = ClusterWorld.X();
        global[1] = ClusterWorld.Y();
        global[2] = ClusterWorld.Z();

        bool isMVTX = (0 <= layer && layer <= 2) ? true : false;
        addHit(outFile, firstHits, global, minHit[0], minHit[1], minHit[2], maxHit[0], maxHit[1], maxHit[2], isMVTX);
      }
    }
    else
    {
      if (hitset->size() >= 200) continue; //There's a lot of noise in the TPC, it shows up as the same (x,y) but different z
      TrkrHitSet::ConstRange hitrangei = hitset->getHits();
      for (TrkrHitSet::ConstIterator hitr = hitrangei.first;
           hitr != hitrangei.second;
           ++hitr)
      {
        TrkrDefs::hitkey hit_key = hitr->first;

        double drift_velocity_rescale = 45.0/44.4;
        double clusz_offset = 16.;
        double AdcClockPeriod = 53.0;
        PHG4TpcCylinderGeom* layergeom = geantGeomTpc->GetLayerCellGeom(layer);
        double radius = layergeom->get_radius();
        float phibin = (float) TpcDefs::getPad(hit_key);
        float tbin = (float) TpcDefs::getTBin(hit_key);
        double zdriftlength = tbin * actsGeom->get_drift_velocity()*drift_velocity_rescale * AdcClockPeriod;
        unsigned short NTBins = (unsigned short) layergeom->get_zbins();
        double m_tdriftmax = AdcClockPeriod * NTBins / 2.0;
        double clusz = (m_tdriftmax * actsGeom->get_drift_velocity()*drift_velocity_rescale) - zdriftlength + clusz_offset;
        float side = TpcDefs::getSide(hitsetkey);
        if (side == 0)
        {
          clusz = -clusz;
        }
        float phi_center = layergeom->get_phicenter(phibin);
        global[0] = radius * cos(phi_center);
        global[1] = radius * sin(phi_center);
        global[2] = clusz;

        ++count_TPCHits;
        addHit(outFile, firstHits, global, minHit[0], minHit[1], minHit[2], maxHit[0], maxHit[1], maxHit[2], false);
      } 
    }
  }

  event_file_trailer(outFile, minHit, maxHit);
  outFile.close();
  if ((count_MVTXClusters < m_minMVTXClusters) ||
      (count_INTTClusters < m_minINTTClusters) ||
      (count_TPCHits < m_minTPCHits) ||
      (count_TPCHits < m_minTPOTClusters))
  {
    std::remove(outFileName.c_str());
  }

  L1_BCOs.clear();

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


void event_display_maker::event_file_start(std::ofstream &json_file_header, std::string date, int runid, uint64_t bco)
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
  json_file_header << "\"Cosmic\"," << std::endl;
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

void event_display_maker::event_file_trailer(std::ofstream &json_file_trailer, float minHit[3], float maxHit[3])
{
  float deltaX = minHit[0] - maxHit[0];
  float deltaY = minHit[1] - maxHit[1];
  float deltaZ = minHit[2] - maxHit[2];
  float length = 18*sqrt(pow(deltaX, 2) + pow(deltaY, 2) + pow(deltaZ, 2));
  float scale = 1e3;

  json_file_trailer << "]," << std::endl;
  json_file_trailer << "    \"JETS\": [" << std::endl;
  json_file_trailer << "         ]" << std::endl;
  json_file_trailer << "    }," << std::endl;
  json_file_trailer << "\"TRACKS\": {" << std::endl;
  json_file_trailer << "   \"B\": 0.000014," << std::endl;
  json_file_trailer << "   \"TRACKHITS\": [" << std::endl;
  json_file_trailer << "     {" << std::endl;
  json_file_trailer << "       \"color\": 16777215," << std::endl;
  json_file_trailer << "       \"l\": " << length << "," << std::endl;
  json_file_trailer << "       \"nh\": 6," << std::endl;
  json_file_trailer << "       \"pxyz\": [" << std::endl;
  json_file_trailer << "         " << deltaX << "," << std::endl;
  json_file_trailer << "        " << deltaY << "," << std::endl;
  json_file_trailer << "         " << deltaZ << std::endl;
  json_file_trailer << "       ]," << std::endl;
  json_file_trailer << "       \"q\": 1," << std::endl;
  json_file_trailer << "       \"xyz\": [" << std::endl;
  json_file_trailer << "         " << maxHit[0]*scale << ", " << std::endl;
  json_file_trailer << "          " << maxHit[1]*scale << ", " << std::endl;
  json_file_trailer << "         " << maxHit[2](scale << std::endl;
  json_file_trailer << "       ]" << std::endl;
  json_file_trailer << "     }," << std::endl;
  json_file_trailer << "     {" << std::endl;
  json_file_trailer << "       \"color\": 16777215," << std::endl;
  json_file_trailer << "       \"l\": " << length << "," << std::endl;
  json_file_trailer << "       \"nh\": 6," << std::endl;
  json_file_trailer << "       \"pxyz\": [" << std::endl;
  json_file_trailer << "         " << -1*deltaX << "," << std::endl;
  json_file_trailer << "        " << -1*deltaY << "," << std::endl;
  json_file_trailer << "         " << -1*deltaZ << std::endl;
  json_file_trailer << "       ]," << std::endl;
  json_file_trailer << "       \"q\": 1," << std::endl;
  json_file_trailer << "       \"xyz\": [" << std::endl;
  json_file_trailer << "         " << maxHit[0]*scale << ", " << std::endl;
  json_file_trailer << "          " << maxHit[1]*scale << ", " << std::endl;
  json_file_trailer << "         " << maxHit[2]*scale << std::endl;
  json_file_trailer << "       ]" << std::endl;
  json_file_trailer << "     }" << std::endl;
  json_file_trailer << "   ]" << std::endl;
  json_file_trailer << "}" << std::endl;
  json_file_trailer << "}" << std::endl;
}

void event_display_maker::addHit(std::ofstream &json_file_hit, bool &firstHit, float hit[3], float &minX, float &minY, float &minZ, float &maxX, float &maxY, float &maxZ, bool isMVTX)
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

  if (isMVTX)
  {
    if (hit[1] < minY)
    {
      minX = hit[0];
      minY = hit[1];
      minZ = hit[2];
    }
    if (hit[1] > maxY)
    {
      maxX = hit[0];
      maxY = hit[1];
      maxZ = hit[2];
    }
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
