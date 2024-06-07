// Tell emacs that this is a C++ source
//  -*- C++ -*-.
#ifndef MVTXHITMAP_H
#define MVTXHITMAP_H

#include <fun4all/SubsysReco.h>

#include <string>
#include <vector>
#include <cstdint>

#include <trackbase/MvtxEventInfov2.h>
#include <trackbase/TrkrHitSetContainerv1.h>

#include <TTree.h>

class PHCompositeNode;

/// mvtx raw data decoder
class mvtx_hit_map : public SubsysReco
{
 public:

  mvtx_hit_map(const std::string& name = "mvtx_hit_map") : SubsysReco(name) {}
  ~mvtx_hit_map() override {}
 
  int InitRun(PHCompositeNode*) override;
  int process_event(PHCompositeNode*) override;
  int End(PHCompositeNode*) override;

  void SetOutputFile(const std::string& name) { m_output_filename = name; }
  void SetPrefix(const std::string& prefix) { m_prefix = prefix; }

 private:

  std::string m_output_filename{"outputHits.root"};
  std::string m_prefix{""};
  TTree *m_tree{nullptr};
  
  int f4a_event_counter{0};
  int event{0};
  std::vector<double> strobe_BCOs{};
  std::vector<double> L1_BCOs{};
  int n_L1s{0};
  int n_strobe_BCOs{0};
  std::vector<int> layer_occupancies{};
  std::vector<int> stave_occupancies{};
  std::vector<int> chip_occupancies{};
  int layer{0};
  int stave{0};
  int chip{0};
  std::vector<int> row{};
  std::vector<int> col{};
 
};

#endif
