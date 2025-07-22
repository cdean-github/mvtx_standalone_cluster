#include "latency_analyzer.h"

#include <math.h>

#include <fun4all/Fun4AllReturnCodes.h>

#include <ffaobjects/EventHeader.h>
#include <phool/getClass.h>
#include <phool/PHCompositeNode.h>
#include <phool/PHNodeIterator.h>
#include <phool/recoConsts.h>

#include <boost/format.hpp>
#include <boost/math/special_functions/sign.hpp>

#include <g4main/PHG4TruthInfoContainer.h>
#include <g4main/PHG4Particle.h>

std::string latency_analyzer::to_string_with_precision(const float a_value, const int n)
{
    std::ostringstream out;
    out.precision(n);
    out << std::fixed << a_value;
    return out.str();
}

// Reference: https://github.com/sPHENIX-Collaboration/analysis/blob/master/TPC/DAQ/macros/prelimEvtDisplay/TPCEventDisplay.C
//____________________________________________________________________________..
latency_analyzer::latency_analyzer(const std::string &name):
 SubsysReco(name)
{
}

//____________________________________________________________________________..
latency_analyzer::~latency_analyzer()
{
}

//____________________________________________________________________________..
int latency_analyzer::Init(PHCompositeNode *topNode)
{
  outFile = new TFile(outFileName.c_str(), "RECREATE");

  outTree = new TTree("Hits", "Hits");
  outTree->OptimizeBaskets();
  outTree->SetAutoSave(-5e6);

  outTree->Branch("event", &event, "event/I");
  outTree->Branch("n_hits", &n_hits, "n_hits/I");
  outTree->Branch("n_hits_L0", &n_hits_L0, "n_hits_L0/I");
  outTree->Branch("n_hits_L1", &n_hits_L1, "n_hits_L1/I");
  outTree->Branch("n_hits_L2", &n_hits_L2, "n_hits_L2/I");
  outTree->Branch("n_clusters_L1", &n_clusters_L1, "n_clusters_L1/I");
  outTree->Branch("n_clusters_L2", &n_clusters_L2, "n_clusters_L2/I");
  outTree->Branch("trigger_latency", &trigger_latency, "trigger_latency/F");

  for (unsigned int i = 0; i < nTriggers; ++i)
  {
    std::string trigger_bit = "trigger_bit_" + std::to_string(i);
    std::string trigger_bit_type = trigger_bit + "/O";
    outTree->Branch(trigger_bit.c_str(), &m_trigger_bit[i], trigger_bit_type.c_str());
  }

  triggeranalyzer = new TriggerAnalyzer();
  return Fun4AllReturnCodes::EVENT_OK;
}
//____________________________________________________________________________..
int latency_analyzer::process_event(PHCompositeNode *topNode)
{
  n_hits = n_hits_L0 = n_hits_L1 = n_hits_L2 = n_clusters_L1 = n_clusters_L2 = 0;
  trigger_latency = 0.;

  for (unsigned int i = 0; i < nTriggers; ++i)
  {
    m_trigger_bit[i] = false;
  } 

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

  trkrClusterContainer = findNode::getClass<TrkrClusterContainer>(dstNode, "TRKR_CLUSTER");
  if (!trkrClusterContainer)
  {
    std::cout << __FILE__ << "::" << __func__ << " - TRKR_CLUSTER  missing, doing nothing." << std::endl;
    exit(1);
  }

  triggeranalyzer->decodeTriggers(topNode);

  for (unsigned int i = 0; i < nTriggers; ++i)
  {
    m_trigger_bit[i] = triggeranalyzer->didTriggerFire(i);
  }

  event = f4aCounter;

  TrkrHitSetContainer::ConstRange hitsetrange = trkrHitSetContainer->getHitSets(TrkrDefs::TrkrId::mvtxId);
  for (TrkrHitSetContainer::ConstIterator hitsetitr = hitsetrange.first; hitsetitr != hitsetrange.second; ++hitsetitr)
  {
    TrkrHitSet *hitset = hitsetitr->second;
    n_hits += hitset->size();
    auto hitsetkey = hitset->getHitSetKey();

    int layer = TrkrDefs::getLayer(hitsetkey);
    uint8_t stave = MvtxDefs::getStaveId(hitsetkey);

    if (layer == 1 && (stave == 2 || stave == 13 || stave == 14))
    {
      n_hits_L1 +=  hitset->size();
      auto range = trkrClusterContainer->getClusters(hitsetkey);
      for (auto iter = range.first; iter != range.second; ++iter)
      {
        ++n_clusters_L1;
      }
    }
    else if (layer == 2 && (stave == 15 || stave == 16 || stave == 17 || stave == 18))
    {
      n_hits_L2 +=  hitset->size();
      auto range = trkrClusterContainer->getClusters(hitsetkey);
      for (auto iter = range.first; iter != range.second; ++iter)
      {
        ++n_clusters_L2;
      }
    }
    else continue;
  }

  outTree->Fill();

  ++f4aCounter;
  return Fun4AllReturnCodes::EVENT_OK;
}


//____________________________________________________________________________..
int latency_analyzer::End(PHCompositeNode *topNode)
{
  outFile->Write();
  outFile->Close();
  delete outFile;
  return Fun4AllReturnCodes::EVENT_OK;
}

//____________________________________________________________________________..
int latency_analyzer::Reset(PHCompositeNode *topNode)
{
  return Fun4AllReturnCodes::EVENT_OK;
}

//____________________________________________________________________________..
void latency_analyzer::Print(const std::string &what) const
{
}
