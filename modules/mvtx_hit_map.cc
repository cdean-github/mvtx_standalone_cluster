
#include "mvtx_hit_map.h"

#include <fun4all/Fun4AllReturnCodes.h>
#include <fun4all/PHTFileServer.h>

#include <phool/PHCompositeNode.h>
#include <phool/getClass.h>
#include <phool/PHNodeIterator.h>
#include <phool/recoConsts.h>

#include <ffarawobjects/Gl1RawHit.h>

#include <trackbase/MvtxDefs.h>
#include <trackbase/MvtxEventInfov2.h>
#include <trackbase/TrkrHitSet.h>
#include <trackbase/TrkrHitSetContainerv1.h>
#include <trackbase/TrkrHitv2.h>

#include <algorithm>
#include <cassert>
#include <string>
#include <vector>


int mvtx_hit_map::InitRun(PHCompositeNode *topNode)
{
  
  if (m_prefix != "")
  {
    m_output_filename = m_prefix + m_output_filename;
  }
 // create output file
  PHTFileServer::get().open(m_output_filename, "RECREATE");
  m_tree= new TTree("T", "T");
  m_tree->OptimizeBaskets();
  m_tree->SetAutoSave(-5e6);
  m_tree->Branch("event", &event, "event/I");
  m_tree->Branch("n_strobe_BCOs", &n_strobe_BCOs, "n_strobe_BCOs/I");
  m_tree->Branch("strobe_BCOs", &strobe_BCOs);
  m_tree->Branch("n_L1s", &n_L1s, "n_L1s/I");
  m_tree->Branch("L1_BCOs", &L1_BCOs);
  m_tree->Branch("layer", &layer, "layer/I");
  m_tree->Branch("stave", &stave, "stave/I");
  m_tree->Branch("chip", &chip, "chip/I");
  m_tree->Branch("row", &row);
  m_tree->Branch("col", &col);

  return Fun4AllReturnCodes::EVENT_OK;
}

int mvtx_hit_map::process_event(PHCompositeNode *topNode)
{

    TrkrHitSetContainerv1 * trkr_hitset_cont = findNode::getClass<TrkrHitSetContainerv1>(topNode, "TRKR_HITSET");
    if (!trkr_hitset_cont)
    {
      std::cout << __FILE__ << "::" << __func__ << " - TRKR_HITSET  missing, doing nothing." << std::endl;
      exit(1);
    }

   

    MvtxEventInfov2* mvtx_event_header = findNode::getClass<MvtxEventInfov2>(topNode, "MVTXEVENTHEADER");
    if (!mvtx_event_header)
    {
      std::cout << __FILE__ << "::" << __func__ << " - MVTXEVENTHEADER missing, doing nothing." << std::endl;
      exit(1);
    }

    std::set<uint64_t> strobeList = mvtx_event_header->get_strobe_BCOs();
    for (auto iterStrobe = strobeList.begin(); iterStrobe != strobeList.end(); ++iterStrobe)
    {
      strobe_BCOs.push_back(static_cast<double>(*iterStrobe)); 
      std::set<uint64_t> l1List = mvtx_event_header->get_L1_BCO_from_strobe_BCO(*iterStrobe);
      for (auto iterL1 = l1List.begin(); iterL1 != l1List.end(); ++iterL1)
      {
        L1_BCOs.push_back(static_cast<double>(*iterL1));
      }
    }

    event = f4a_event_counter;
    n_L1s = mvtx_event_header->get_number_L1s();
    n_strobe_BCOs = strobe_BCOs.size();


    // clear hit vectors
    layer = -1;
    stave = -1;
    chip = -1;
    row.clear();
    col.clear();

    TrkrHitSetContainer::ConstRange mvtx_hitsetrange = trkr_hitset_cont->getHitSets(TrkrDefs::TrkrId::mvtxId);
    for (TrkrHitSetContainer::ConstIterator hitset_itr = mvtx_hitsetrange.first; hitset_itr != mvtx_hitsetrange.second; ++hitset_itr)
    {
      TrkrHitSet *myhitset = hitset_itr->second;
      auto myHitsetkey = myhitset->getHitSetKey();

      layer = TrkrDefs::getLayer(myHitsetkey);
      stave = MvtxDefs::getStaveId(myHitsetkey);
      chip = MvtxDefs::getChipId(myHitsetkey);


      TrkrHitSet::ConstRange hitrange = myhitset->getHits();
      for (TrkrHitSet::ConstIterator hit_itr = hitrange.first; hit_itr != hitrange.second; ++hit_itr)
      {
        
        TrkrDefs::hitkey myHitkey = hit_itr->first;
        int this_row = MvtxDefs::getRow(myHitkey);
        int this_col = MvtxDefs::getCol(myHitkey);
        row.push_back(this_row);
        col.push_back(this_col);
      }

    
      m_tree->Fill();

      row.clear();
      col.clear();

    }

    f4a_event_counter++;
    strobe_BCOs.clear();
    L1_BCOs.clear();

    return Fun4AllReturnCodes::EVENT_OK;
    
}

//_____________________________________________________________________
int mvtx_hit_map::End(PHCompositeNode * /*topNode*/)
{

  PHTFileServer::get().cd(m_output_filename);
  PHTFileServer::get().write(m_output_filename);
  std::cout << "Hits::EndRun - Done" << std::endl;
  return Fun4AllReturnCodes::EVENT_OK;
}

