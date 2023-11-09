#include "mvtx_standalone_cluster.h"

#include <fun4all/Fun4AllReturnCodes.h>

#include <phool/getClass.h>
#include <phool/PHCompositeNode.h>
#include <phool/PHNodeIterator.h>

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
  outTree = new TTree("Clusters", "Clusters");
  outTree->OptimizeBaskets();
  outTree->SetAutoSave(-5e6);

  outTree->Branch("event", &event, "event/I");
  outTree->Branch("strobe_BCO", &strobe_BCO, "strobe_BCO/I");
  outTree->Branch("L1_BCOs", &L1_BCOs);
  outTree->Branch("numberL1s", &numberL1s, "numberL1s/I");
  outTree->Branch("layer", &layer, "layer/I");
  outTree->Branch("stave", &stave, "stave/I");
  outTree->Branch("chip", &chip, "chip/I");
  outTree->Branch("localX", &localX, "localX/F");
  outTree->Branch("localY", &localY, "localY/F");
  outTree->Branch("globalX", &globalX, "globalX/F");
  outTree->Branch("globalY", &globalY, "globalY/F");
  outTree->Branch("globalZ", &globalZ, "globalZ/F");
  outTree->Branch("clusZSize", &clusZ, "clusZSize/F");
  outTree->Branch("clusPhiSize", &clusPhi, "clusPhiSize/F");
  outTree->Branch("clusSize", &clusSize, "clusSize/i");

  return Fun4AllReturnCodes::EVENT_OK;
}
//____________________________________________________________________________..
int mvtx_standalone_cluster::process_event(PHCompositeNode *topNode)
{
  PHNodeIterator dstiter(topNode);

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
    std::set<uint64_t> l1List = mvtx_event_header->get_L1_BCO_from_strobe_BCO(*iterStrobe);
    for (auto iterL1 = l1List.begin(); iterL1 != l1List.end(); ++iterL1)
    {
      L1_BCOs.push_back(*iterL1); 
    }
  }

  event = f4aCounter;
  strobe_BCO = mvtx_event_header->get_strobe_BCO();
  numberL1s = mvtx_event_header->get_number_L1s();
  layer = 0;
  stave = 0;
  chip = 0;
  row = 0;
  col = 0;
  localX = 0.;
  localY = 0.;
  globalX = 0.;
  globalY = 0.;
  globalZ = 0.;
  clusZ = 0.;
  clusPhi = 0.;
  clusSize = 0;

  TrkrHitSetContainer::ConstRange hitsetrange = trkrHitSetContainer->getHitSets(TrkrDefs::TrkrId::mvtxId);

  for (TrkrHitSetContainer::ConstIterator hitsetitr = hitsetrange.first; hitsetitr != hitsetrange.second; ++hitsetitr)
  {
    TrkrHitSet *hitset = hitsetitr->second;
    auto hitsetkey = hitset->getHitSetKey();

    layer = TrkrDefs::getLayer(hitsetkey);
    stave = MvtxDefs::getStaveId(hitsetkey);
    chip = MvtxDefs::getChipId(hitsetkey);

    TrkrClusterContainer::ConstRange clusterrange = trktClusterContainer->getClusters(hitsetkey);

    auto surface = actsGeom->maps().getSiliconSurface(hitsetkey);
    auto layergeom = dynamic_cast<CylinderGeom_Mvtx *>(geantGeom->GetLayerGeom(layer));
    TVector2 LocalUse;


    for (TrkrClusterContainer::ConstIterator clusteritr = clusterrange.first; clusteritr != clusterrange.second; ++clusteritr)
    {
      TrkrCluster *cluster = clusteritr->second;

      localX = cluster->getLocalX();
      localY = cluster->getLocalY();
      clusZ = cluster->getZSize();
      clusPhi = cluster->getPhiSize();
      clusSize = cluster->getAdc();

      LocalUse.SetX(localX);
      LocalUse.SetY(localY);
      TVector3 ClusterWorld = layergeom->get_world_from_local_coords(surface, actsGeom, LocalUse);
      globalX = ClusterWorld.X();
      globalY = ClusterWorld.Y();
      globalZ = ClusterWorld.Z();

      outTree->Fill();
    }
  }

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
