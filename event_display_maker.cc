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

  trkrHitSetContainer = findNode::getClass<TrkrHitSetContainerv1>(dstNode, "TRKR_HITSET");
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

  mvtx_event_header = findNode::getClass<MvtxEventInfov2>(topNode, "MVTXEVENTHEADER");
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
  globalX = 0.;
  globalY = 0.;
  globalZ = 0.;
  if (numberL1s == 0) return Fun4AllReturnCodes::ABORTEVENT;

  //Set up the event display writer
  std::ofstream outFile;
  bool firstHits = true;
  float minX = 0., minY = 0., minZ = 0., maxX = 0., maxY = 0., maxZ = 0.;
  if (trktClusterContainer->size() >= m_min_clusters)
  {
    outFile.open(m_evt_display_path + "/EvtDisplay_" + m_runNumber + "_" + L1_BCOs[0] + ".json");
    event_file_start(outFile, m_run_date, m_runNumber, L1_BCOs[0]); 
  }

  TrkrHitSetContainer::ConstRange hitsetrange = trkrHitSetContainer->getHitSets();

  for (TrkrHitSetContainer::ConstIterator hitsetitr = hitsetrange.first; hitsetitr != hitsetrange.second; ++hitsetitr)
  {
    TrkrHitSet *hitset = hitsetitr->second;
    auto hitsetkey = hitset->getHitSetKey();

    layer = TrkrDefs::getLayer(hitsetkey);

    TrkrClusterContainer::ConstRange clusterrange = trktClusterContainer->getClusters(hitsetkey);

    TVector2 LocalUse;
    TVector3 ClusterWorld;

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
      }
      else if (3 <= layer && layer <= 6)
      {
        auto surface = actsGeom->maps().getSiliconSurface(hitsetkey);
        auto layergeom = dynamic_cast<CylinderGeomIntt *>(geantGeomIntt->GetLayerGeom(layer));
        ClusterWorld = layergeom->get_world_from_local_coords(surface, actsGeom, LocalUse);
      }
      else if (7 <= layer && layer <= 54)
      {
        double AdcClockPeriod = 53.0;
        PHG4TpcCylinderGeom* layergeom = geantGeomTpc->GetLayerCellGeom(layer);
 	double radius = layergeom->get_radius();
        float phibin = cluster->getPhiSize();
        float tbin = cluster->getZSize();
        double zdriftlength = tbin * actsGeom->get_drift_velocity() * AdcClockPeriod;
        unsigned short NTBins = (unsigned short) layergeom->get_zbins();
        double m_tdriftmax = AdcClockPeriod * NTBins / 2.0;
        double clusz = (m_tdriftmax * actsGeom->get_drift_velocity()) - zdriftlength;
        float side = TpcDefs::getSide(hitsetkey);
        if (side == 0)
        {
          clusz = -clusz;
        }
        float phi_center = layergeom->get_phicenter(phibin);
	globalX = radius * cos(phi_center);
	globalY = radius * sin(phi_center);
        globalZ = clusz; 
      }
      else if (55 <= layer && layer <= 56)
      {
        uint8_t tileID = MicromegasDefs::getTileId(hitsetkey);
        auto layergeom = dynamic_cast<CylinderGeomMicromegas *>(geantGeomTpot->GetLayerGeom(layer));
        ClusterWorld = layergeom->get_world_from_local_coords(tileID, actsGeom, LocalUse);
      }
      else
      {
        continue;
      }

      if (layer < 7 || 54 < layer) //Outside the TPC volume
      {
        globalX = ClusterWorld.X();
        globalY = ClusterWorld.Y();
        globalZ = ClusterWorld.Z();
      }

      if (outFile)
      {
        std::ostringstream spts;
        if (firstHits)
        {
          firstHits = false;
          minX = maxX = globalX;
          minY = maxY = globalY;
          minZ = maxZ = globalZ;
        }
        else
        {
          spts << ",";
          if (globalY < minY)
          {
            minX = globalX;
            minY = globalY;
            minZ = globalZ;
          }
          if (globalY > maxY)
          {
            maxX = globalX;
            maxY = globalY;
            maxZ = globalZ;
          }
        }

        spts << "{ \"x\": ";
        spts << globalX;
        spts << ", \"y\": ";
        spts << globalY;
        spts << ", \"z\": ";
        spts << globalZ;
        spts << ", \"e\": 0}";

        outFile << (boost::format("%1%") % spts.str());
        spts.clear();
        spts.str("");
      }
    }
  }

  if (outFile)
  {
    event_file_trailer(outFile, minX, minY, minZ, maxX, maxY, maxZ);
    outFile.close();
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


void event_display_maker::event_file_start(std::ofstream &jason_file_header, std::string date, int runid, uint64_t bco)
{
  jason_file_header << "{" << std::endl;
  jason_file_header << "    \"EVENT\": {" << std::endl;
  jason_file_header << "        \"runid\":" << runid << ", " << std::endl;
  jason_file_header << "        \"evtid\": 1, " << std::endl;
  jason_file_header << "        \"time\": 0, " << std::endl;
  jason_file_header << "       \"type\": \"Collision\", " << std::endl;
  jason_file_header << "        \"s_nn\": 0, " << std::endl;
  jason_file_header << "        \"B\": 3.0," << std::endl;
  jason_file_header << "        \"pv\": [0,0,0]," << std::endl;
  jason_file_header << " \"runstats\": [\"sPHENIX Internal\"," << std::endl;
  jason_file_header << "\"Cosmic\"," << std::endl;
  jason_file_header << "\"" << date << ", Run " << runid << "\"," << std::endl;
  jason_file_header << "\"BCO: " << bco <<"\"]  " << std::endl;
  jason_file_header << "    }," << std::endl;
  jason_file_header << "" << std::endl;
  jason_file_header << "    \"META\": {" << std::endl;
  jason_file_header << "       \"HITS\": {" << std::endl;
  jason_file_header << "          \"INNERTRACKER\": {" << std::endl;
  jason_file_header << "              \"type\": \"3D\"," << std::endl;
  jason_file_header << "              \"options\": {" << std::endl;
  jason_file_header << "              \"size\": 5," << std::endl;
  jason_file_header << "              \"color\": 16777215" << std::endl;
  jason_file_header << "              } " << std::endl;
  jason_file_header << "          }," << std::endl;
  jason_file_header << "" << std::endl;
  jason_file_header << "          \"TRACKHITS\": {" << std::endl;
  jason_file_header << "              \"type\": \"3D\"," << std::endl;
  jason_file_header << "              \"options\": {" << std::endl;
  jason_file_header << "              \"size\": 0.5," << std::endl;
  jason_file_header << "              \"transparent\": 0.5," << std::endl;
  jason_file_header << "              \"color\": 16073282" << std::endl;
  jason_file_header << "              } " << std::endl;
  jason_file_header << "          }," << std::endl;
  jason_file_header << "" << std::endl;
  jason_file_header << "    \"JETS\": {" << std::endl;
  jason_file_header << "        \"type\": \"JET\"," << std::endl;
  jason_file_header << "        \"options\": {" << std::endl;
  jason_file_header << "            \"rmin\": 0," << std::endl;
  jason_file_header << "            \"rmax\": 78," << std::endl;
  jason_file_header << "            \"emin\": 0," << std::endl;
  jason_file_header << "            \"emax\": 30," << std::endl;
  jason_file_header << "            \"color\": 16777215," << std::endl;
  jason_file_header << "            \"transparent\": 0.5 " << std::endl;
  jason_file_header << "        }" << std::endl;
  jason_file_header << "    }" << std::endl;
  jason_file_header << "        }" << std::endl;
  jason_file_header << "    }" << std::endl;
  jason_file_header << "," << std::endl;
  jason_file_header << "    \"HITS\": {" << std::endl;
  jason_file_header << "        \"CEMC\":[{\"eta\": 0, \"phi\": 0, \"e\": 0}" << std::endl;
  jason_file_header << "            ]," << std::endl;
  jason_file_header << "        \"HCALIN\": [{\"eta\": 0, \"phi\": 0, \"e\": 0}" << std::endl;
  jason_file_header << "            ]," << std::endl;
  jason_file_header << "        \"HCALOUT\": [{\"eta\": 0, \"phi\": 0, \"e\": 0}" << std::endl;
  jason_file_header << " " << std::endl;
  jason_file_header << "            ]," << std::endl;
  jason_file_header << "" << std::endl;
  jason_file_header << "" << std::endl;
  jason_file_header << "    \"TRACKHITS\": [" << std::endl;
  jason_file_header << "" << std::endl;
  jason_file_header << " ";
}

void event_display_maker::event_file_trailer(std::ofstream &json_file_trailer, float minX, float minY, float minZ, float maxX, float maxY, float maxZ)
{
  float deltaX = minX - maxX;
  float deltaY = minY - maxY;
  float deltaZ = minZ - maxZ;
  float length = sqrt(pow(deltaX, 2) + pow(deltaY, 2) + pow(deltaZ, 2));

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
  json_file_trailer << "         " << maxX << ", " << std::endl;
  json_file_trailer << "          " << maxY << ", " << std::endl;
  json_file_trailer << "         " << maxZ << std::endl;
  json_file_trailer << "       ]" << std::endl;
  json_file_trailer << "     }" << std::endl;
  json_file_trailer << "   ]" << std::endl;
  json_file_trailer << "}" << std::endl;
  json_file_trailer << "}" << std::endl;
}
