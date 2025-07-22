#include "mvtx_standalone_cluster.h"

#include <math.h>

#include <fun4all/Fun4AllReturnCodes.h>

#include <ffaobjects/EventHeader.h>
#include <phool/getClass.h>
#include <phool/PHCompositeNode.h>
#include <phool/PHNodeIterator.h>
#include <phool/recoConsts.h>

#include <boost/format.hpp>
#include <boost/math/special_functions/sign.hpp>

std::map<int,int> event_hit_map;

std::string mvtx_standalone_cluster::to_string_with_precision(const float a_value, const int n)
{
    std::ostringstream out;
    out.precision(n);
    out << std::fixed << a_value;
    return out.str();
}

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
  outTree->OptimizeBaskets();
  outTree->SetAutoSave(-5e6);

  outTree->Branch("event", &event, "event/I");
  outTree->Branch("layer", &layer, "layer/I");
  outTree->Branch("stave", &stave, "stave/I");
  outTree->Branch("chip", &chip, "chip/I");
  outTree->Branch("localX", &localX);
  outTree->Branch("localY", &localY);
  outTree->Branch("globalX", &globalX);
  outTree->Branch("globalY", &globalY);
  outTree->Branch("globalZ", &globalZ);
  outTree->Branch("globalPhi", &globalPhi);
  outTree->Branch("chip_occupancy", &chip_occupancy, "chip_occupancy/F");
  outTree->Branch("chip_hits", &chip_hits, "chip_hits/I");
/*
  recoConsts *rc = recoConsts::instance();
  m_runNumber = rc->get_IntFlag("RUNNUMBER");
  std::string textFileName = "clusterOut_run" + std::to_string(m_runNumber) + ".txt";
 
  clusterTextFile.open(textFileName.c_str());
  clusterTextFile << "strobe_BCO       layer stave chip cluster X cluster Y global R  global phi  global z   nPixels\n";
*/
  return Fun4AllReturnCodes::EVENT_OK;
}
//____________________________________________________________________________..
int mvtx_standalone_cluster::process_event(PHCompositeNode *topNode)
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
/*
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
*/
  event = f4aCounter;
  numberL1s = 0;// mvtx_event_header->get_number_L1s();
  layer = 0;
  stave = 0;
  chip = 0;
  localX.clear();
  localY.clear();
  globalX.clear();
  globalY.clear();
  globalZ.clear();
  globalPhi.clear();
  clusZ.clear();
  clusPhi.clear();
  clusSize.clear();
  chip_occupancy = 0.;
  chip_hits = 0.;

  TrkrHitSetContainer::ConstRange hitsetrange = trkrHitSetContainer->getHitSets(TrkrDefs::TrkrId::mvtxId);

  int nHits = 0;

  for (TrkrHitSetContainer::ConstIterator hitsetitr = hitsetrange.first; hitsetitr != hitsetrange.second; ++hitsetitr)
  {
    TrkrHitSet *hitset = hitsetitr->second;
    auto hitsetkey = hitset->getHitSetKey();

    layer = TrkrDefs::getLayer(hitsetkey);
    stave = MvtxDefs::getStaveId(hitsetkey);
    chip = MvtxDefs::getChipId(hitsetkey);

    auto surface = actsGeom->maps().getSiliconSurface(hitsetkey);
    auto layergeom = dynamic_cast<CylinderGeom_Mvtx *>(geantGeom->GetLayerGeom(layer));
    TVector2 LocalUse;

    chip_hits = hitsetitr->second->size();
    chip_occupancy = (float) chip_hits / (512*1024);

    TrkrHitSet::ConstRange hit_range = hitsetitr->second->getHits();
    for (TrkrHitSet::ConstIterator hit_iter = hit_range.first; hit_iter != hit_range.second; ++hit_iter)
    {
      TrkrDefs::hitkey hitKey = hit_iter->first;

      TVector3 local_coords = layergeom->get_local_coords_from_pixel(MvtxDefs::getRow(hitKey), MvtxDefs::getCol(hitKey));
      localX.push_back(local_coords.x());
      localY.push_back(local_coords.z());
      LocalUse.SetX(local_coords.x());
      LocalUse.SetY(local_coords.z());
      TVector3 ClusterWorld = CylinderGeom_MvtxHelper::get_world_from_local_coords(surface, actsGeom, LocalUse);
      globalX.push_back(ClusterWorld.X());
      globalY.push_back(ClusterWorld.Y());
      globalZ.push_back(ClusterWorld.Z());
      float phi = atan2(ClusterWorld.Y(), ClusterWorld.X());
      globalPhi.push_back(phi);



      //if (layer == 0 && stave == 4) ++nHits;
      ++nHits;
    }

    outTree->Fill();

    localX.clear();
    localY.clear();
    globalX.clear();
    globalY.clear();
    globalZ.clear();
    globalPhi.clear();
    clusZ.clear();
    clusPhi.clear();
    clusSize.clear();

  }

  if (nHits > 9e4) event_hit_map.insert(std::pair<int, int>(event, nHits)); 

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

  //clusterTextFile.close();

std::cout << "MVTX hit analyser end" << std::endl;
std::cout << "Interesting events with more than 90k hits" << std::endl;
for (auto iter = event_hit_map.begin(); iter != event_hit_map.end(); ++iter)
{
  std::cout << "Event: " << iter->first << ", nHits: " << iter->second << std::endl;
}

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
