#include "mvtx_standalone_cluster.h"

#include <fun4all/Fun4AllReturnCodes.h>

#include <ffaobjects/EventHeader.h>
#include <phool/getClass.h>
#include <phool/PHCompositeNode.h>
#include <phool/PHNodeIterator.h>
#include <phool/recoConsts.h>

#include <boost/format.hpp>
#include <boost/math/special_functions/sign.hpp>

int nEvents = 0;
int nEventsWTrigger = 0;
int nEventsWOccGreaterThan0p3 = 0;
int nEventsWTriggerAndOccGreaterThan0p3 = 0;
int nChips = 0;
int nChipsWOccGreaterThan0p3 = 0;

// Reference: https://github.com/sPHENIX-Collaboration/analysis/blob/master/TPC/DAQ/macros/prelimEvtDisplay/TPCEventDisplay.C
//____________________________________________________________________________..
mvtx_standalone_cluster::mvtx_standalone_cluster(const std::string &name):
 SubsysReco(name)
{
}

//____________________________________________________________________________..
mvtx_standalone_cluster::~mvtx_standalone_cluster()
{
}

//____________________________________________________________________________..
int mvtx_standalone_cluster::Init(PHCompositeNode *topNode)
{
  outFile = new TFile(outFileName.c_str(), "RECREATE");
  outTree = new TTree("Hits", "Hits");
  //outTree = new TTree("Clusters", "Clusters");
  outTree->OptimizeBaskets();
  outTree->SetAutoSave(-5e6);

  outTree->Branch("event", &event, "event/I");
  outTree->Branch("strobe_BCOs", &strobe_BCOs);
  outTree->Branch("L1_BCOs", &L1_BCOs);
  outTree->Branch("numberL1s", &numberL1s, "numberL1s/I");
  outTree->Branch("layer", &layer, "layer/I");
  outTree->Branch("stave", &stave, "stave/I");
  outTree->Branch("chip", &chip, "chip/I");
  outTree->Branch("localX", &localX);
  outTree->Branch("localY", &localY);
  outTree->Branch("globalX", &globalX);
  outTree->Branch("globalY", &globalY);
  outTree->Branch("globalZ", &globalZ);
  //outTree->Branch("clusZSize", &clusZ);
  //outTree->Branch("clusPhiSize", &clusPhi);
  //outTree->Branch("clusSize", &clusSize);
  outTree->Branch("chip_occupancy", &chip_occupancy, "chip_occupancy/F");
  outTree->Branch("chip_hits", &chip_hits, "chip_hits/I");

  return Fun4AllReturnCodes::EVENT_OK;
}
//____________________________________________________________________________..
int mvtx_standalone_cluster::process_event(PHCompositeNode *topNode)
{
  bool occAbove0p3 = false;

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

  //trktClusterContainer = findNode::getClass<TrkrClusterContainer>(dstNode, "TRKR_CLUSTER");
  //if (!trktClusterContainer)
  //{
  //  std::cout << __FILE__ << "::" << __func__ << " - TRKR_CLUSTER missing, doing nothing." << std::endl;
  //  exit(1);
  //}

  actsGeom = findNode::getClass<ActsGeometry>(topNode, "ActsGeometry");
  if (!actsGeom)
  {
    std::cout << __FILE__ << "::" << __func__ << " - ActsGeometry missing, doing nothing." << std::endl;
    exit(1);
  }

  geantGeom = findNode::getClass<PHG4CylinderGeomContainer>(topNode, "CYLINDERGEOM_MVTX");
  if (!geantGeom)
  {
    std::cout << __FILE__ << "::" << __func__ << " - CYLINDERGEOM_MVTX missing, doing nothing." << std::endl;
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
    strobe_BCOs.push_back(*iterStrobe); 
    std::set<uint64_t> l1List = mvtx_event_header->get_L1_BCO_from_strobe_BCO(*iterStrobe);
    for (auto iterL1 = l1List.begin(); iterL1 != l1List.end(); ++iterL1)
    {
      L1_BCOs.push_back(*iterL1); 
    }
  }

  event = f4aCounter;
  numberL1s = mvtx_event_header->get_number_L1s();
  layer = 0;
  stave = 0;
  chip = 0;
  localX.clear();
  localY.clear();
  globalX.clear();
  globalY.clear();
  globalZ.clear();
  clusZ.clear();
  clusPhi.clear();
  clusSize.clear();
  chip_occupancy = 0.;
  chip_hits = 0.;

  ++nEvents;
  if (numberL1s != 0) ++nEventsWTrigger;

  //Set up the event display writer
  std::ofstream outFile;
  bool firstHits = true;
  float minX = 0., minY = 0., minZ = 0., maxX = 0., maxY = 0., maxZ = 0.;
  if (m_write_evt_display)// && trktClusterContainer->size() >= m_min_clusters)
  {
    outFile.open(m_evt_display_path + "/EvtDisplay_" + m_runNumber + "_" + L1_BCOs[0] + ".json");
    event_file_start(outFile, m_run_date, m_runNumber, L1_BCOs[0]); 
  }

  TrkrHitSetContainer::ConstRange hitsetrange = trkrHitSetContainer->getHitSets(TrkrDefs::TrkrId::mvtxId);

  for (TrkrHitSetContainer::ConstIterator hitsetitr = hitsetrange.first; hitsetitr != hitsetrange.second; ++hitsetitr)
  {
    TrkrHitSet *hitset = hitsetitr->second;
    auto hitsetkey = hitset->getHitSetKey();

    layer = TrkrDefs::getLayer(hitsetkey);
    stave = MvtxDefs::getStaveId(hitsetkey);
    chip = MvtxDefs::getChipId(hitsetkey);

    //TrkrClusterContainer::ConstRange clusterrange = trktClusterContainer->getClusters(hitsetkey);

    auto surface = actsGeom->maps().getSiliconSurface(hitsetkey);
    auto layergeom = dynamic_cast<CylinderGeom_Mvtx *>(geantGeom->GetLayerGeom(layer));
    TVector2 LocalUse;

    ++nChips;
    chip_hits = hitsetitr->second->size();
    chip_occupancy = (float) chip_hits / (512*1024);
    if (chip_occupancy*100 >= 0.3) //Take all clusters
    {
      ++nChipsWOccGreaterThan0p3;
      if (!occAbove0p3)
      {
        occAbove0p3 = true;
        ++nEventsWOccGreaterThan0p3;
        if (numberL1s != 0) ++nEventsWTriggerAndOccGreaterThan0p3;
      }
      //for (TrkrClusterContainer::ConstIterator clusteritr = clusterrange.first; clusteritr != clusterrange.second; ++clusteritr)
      //{
      TrkrHitSet::ConstRange hit_range = hitsetitr->second->getHits();
      for (TrkrHitSet::ConstIterator hit_iter = hit_range.first; hit_iter != hit_range.second; ++hit_iter)
      {
        //TrkrCluster *cluster = clusteritr->second;
        TrkrDefs::hitkey hitKey = hit_iter->first;

        TVector3 local_coords = layergeom->get_local_coords_from_pixel(MvtxDefs::getRow(hitKey), MvtxDefs::getCol(hitKey));
        localX.push_back(local_coords.x());
        localY.push_back(local_coords.z());
        //localX.push_back(cluster->getLocalX());
        //localY.push_back(cluster->getLocalY());
        //clusZ.push_back(cluster->getZSize());
        //clusPhi.push_back(cluster->getPhiSize());
        //clusSize.push_back(cluster->getAdc());

        LocalUse.SetX(local_coords.x());
        LocalUse.SetY(local_coords.z());
        //LocalUse.SetX(cluster->getLocalX());
        //LocalUse.SetY(cluster->getLocalY());
        TVector3 ClusterWorld = layergeom->get_world_from_local_coords(surface, actsGeom, LocalUse);
        globalX.push_back(ClusterWorld.X());
        globalY.push_back(ClusterWorld.Y());
        globalZ.push_back(ClusterWorld.Z());

        if (outFile)
        {
          std::ostringstream spts;
          if (firstHits)
          {
            firstHits = false;
            minX = maxX = ClusterWorld.X();
            minY = maxY = ClusterWorld.Y();
            minZ = maxZ = ClusterWorld.Z();
          }
          else
          {
            spts << ",";
            if (ClusterWorld.Y() < minY)
            {
              minX = ClusterWorld.X();
              minY = ClusterWorld.Y();
              minZ = ClusterWorld.Z();
            }
            if (ClusterWorld.Y() > maxY)
            {
              maxX = ClusterWorld.X();
              maxY = ClusterWorld.Y();
              maxZ = ClusterWorld.Z();
            }
          }

          spts << "{ \"x\": ";
          spts << ClusterWorld.X();
          spts << ", \"y\": ";
          spts << ClusterWorld.Y();
          spts << ", \"z\": ";
          spts << ClusterWorld.Z();
          spts << ", \"e\": 0}";

          outFile << (boost::format("%1%") % spts.str());
          spts.clear();
          spts.str("");
        }
      }

      outTree->Fill();

      localX.clear();
      localY.clear();
      globalX.clear();
      globalY.clear();
      globalZ.clear();
      clusZ.clear();
      clusPhi.clear();
      clusSize.clear();
    }
  }

  if (outFile)
  {
    event_file_trailer(outFile, minX, minY, minZ, maxX, maxY, maxZ);
    outFile.close();
  }

  strobe_BCOs.clear();
  L1_BCOs.clear();
  ++f4aCounter;

  return Fun4AllReturnCodes::EVENT_OK;
}


//____________________________________________________________________________..
int mvtx_standalone_cluster::End(PHCompositeNode *topNode)
{
  outFile->Write();
  outFile->Close();
  delete outFile;


std::cout << "MVTX hit analyser end" << std::endl;
std::cout << "Number of events analysed: " << nEvents << std::endl;
std::cout << "Number of events analysed with trigger: " << nEventsWTrigger << std::endl;
std::cout << "Number of events analysed with occupancy above 0.3%: " << nEventsWOccGreaterThan0p3 << std::endl;
std::cout << "Number of events analysed with trigger and occupancy above 0.3%: " << nEventsWTriggerAndOccGreaterThan0p3 << std::endl;
std::cout << "Number of chips with hits: " << nChips << std::endl;
std::cout << "Number of chips with occupancy above 0.3%: " << nChipsWOccGreaterThan0p3 << std::endl;

  return Fun4AllReturnCodes::EVENT_OK;
}

//____________________________________________________________________________..
int mvtx_standalone_cluster::Reset(PHCompositeNode *topNode)
{
  return Fun4AllReturnCodes::EVENT_OK;
}

//____________________________________________________________________________..
void mvtx_standalone_cluster::Print(const std::string &what) const
{
}


void mvtx_standalone_cluster::event_file_start(std::ofstream &jason_file_header, std::string date, int runid, int bco)
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
  jason_file_header << "\"BCO:" << bco <<"\"]  " << std::endl;
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

void mvtx_standalone_cluster::event_file_trailer(std::ofstream &json_file_trailer, float minX, float minY, float minZ, float maxX, float maxY, float maxZ)
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

