#include <cstdint>
#include <iostream>
#include <vector>
#include <array>
#include <algorithm>


int CalcChipOccupancy(TString filename, TString outputname)
{

    TFile *f = new TFile(filename);
    TTree *t = (TTree*)f->Get("event_info");
    int event = 0;
    std::vector<uint64_t>* strobe_BCO = 0;
    std::vector<int>* layer = 0;
    std::vector<int>* stave = 0;
    std::vector<int>* chip = 0;
    std::vector<int>* row = 0;
    std::vector<int>* col = 0;

    t->SetBranchAddress("event", &event);
    t->SetBranchAddress("strobe_BCOs", &strobe_BCO);
    t->SetBranchAddress("layer", &layer);
    t->SetBranchAddress("stave", &stave);
    t->SetBranchAddress("chip", &chip);
    t->SetBranchAddress("row", &row);
    t->SetBranchAddress("col", &col);

    int n_events = t->GetEntries();
    std::cout << "Processing " << n_events << " events" << std::endl;

   // output file
    TFile *fout = new TFile(outputname, "RECREATE"); 
    TTree *tout = new TTree("hitmaps", "hitmaps");
    int n_hits_L0 = 0;
    std::vector<int> L0_stave_hits {};
    std::vector<int> L0_chip_hits {};
    std::vector<int> L0_stave {};
    std::vector<int> L0_chip {};
    std::vector<int> L0_row {};
    std::vector<int> L0_col {};

    int n_hits_L1 = 0;
    std::vector<int> L1_stave_hits {};
    std::vector<int> L1_chip_hits {};
    std::vector<int> L1_stave {};
    std::vector<int> L1_chip {};
    std::vector<int> L1_row {};
    std::vector<int> L1_col {};

    int n_hits_L2 = 0;
    std::vector<int> L2_stave_hits {};
    std::vector<int> L2_chip_hits {};
    std::vector<int> L2_stave {};
    std::vector<int> L2_chip {};
    std::vector<int> L2_row {};
    std::vector<int> L2_col {};  

    tout->Branch("n_hits_L0", &n_hits_L0);
    tout->Branch("L0_stave_hits", &L0_stave_hits);
    tout->Branch("L0_chip_hits", &L0_chip_hits);
    tout->Branch("L0_stave", &L0_stave);
    tout->Branch("L0_chip", &L0_chip);
    tout->Branch("L0_row", &L0_row);
    tout->Branch("L0_col", &L0_col);

    tout->Branch("n_hits_L1", &n_hits_L1);
    tout->Branch("L1_stave_hits", &L1_stave_hits);
    tout->Branch("L1_chip_hits", &L1_chip_hits);
    tout->Branch("L1_stave", &L1_stave);
    tout->Branch("L1_chip", &L1_chip);
    tout->Branch("L1_row", &L1_row);
    tout->Branch("L1_col", &L1_col);

    tout->Branch("n_hits_L2", &n_hits_L2);
    tout->Branch("L2_stave_hits", &L2_stave_hits);
    tout->Branch("L2_chip_hits", &L2_chip_hits);
    tout->Branch("L2_stave", &L2_stave);
    tout->Branch("L2_chip", &L2_chip);
    tout->Branch("L2_row", &L2_row);
    tout->Branch("L2_col", &L2_col);

    std::array<int, 12*9> lay0_chip_hits {};
    std::array<int, 16*9> lay1_chip_hits {};
    std::array<int, 20*9> lay2_chip_hits {};
    std::array<int, 12> lay0_stave_hits {};
    std::array<int, 16> lay1_stave_hits {};
    std::array<int, 20> lay2_stave_hits {};

    int n_strobes_processed = 0;
    int n_events_above_occ = 0;
    int update_10_percent = int(n_events/10);
    int progress = 0;
    int TOTAL_PIXELS_LAYER0 = 12*9*1024*512;
    double target_occ = 0.025;
    
    for (int ievent = 0; ievent < n_events; ievent++) 
    {
        t->GetEntry(ievent);
        if(ievent % update_10_percent == 0)
        {
            std::cout << "\t\tProcessing event " << ievent << " (" << progress << "%)" << std::endl;
            progress += 10;
        }
        if (strobe_BCO->size()  == 0) continue; // skip events with no strobes

        // clear tree
        n_hits_L0 = 0;
        n_hits_L1 = 0;
        n_hits_L2 = 0;
        L0_stave_hits.clear();
        L0_chip_hits.clear();
        L0_stave.clear();
        L0_chip.clear();
        L0_row.clear();
        L0_col.clear();
        L1_stave_hits.clear();
        L1_chip_hits.clear();
        L1_stave.clear();
        L1_chip.clear();
        L1_row.clear();
        L1_col.clear();
        L2_stave_hits.clear();
        L2_chip_hits.clear();
        L2_stave.clear();
        L2_chip.clear();
        L2_row.clear();
        L2_col.clear();
        // clear counter arrays
        for (int j = 0; j < 12*9; j++) lay0_chip_hits[j] = 0;
        for (int j = 0; j < 16*9; j++) lay1_chip_hits[j] = 0;
        for (int j = 0; j < 20*9; j++) lay2_chip_hits[j] = 0;
        for (int j = 0; j < 12; j++) lay0_stave_hits[j] = 0;
        for (int j = 0; j < 16; j++) lay1_stave_hits[j] = 0;
        for (int j = 0; j < 20; j++) lay2_stave_hits[j] = 0;

        for (int j = 0; j < strobe_BCO->size(); j++)
        {
            if (layer->at(j) == 0)
            {
                n_hits_L0++;
                L0_stave.push_back(stave->at(j));
                L0_chip.push_back(chip->at(j));
                L0_row.push_back(row->at(j));
                L0_col.push_back(col->at(j));
                int s = stave->at(j);
                int c = chip->at(j);
                lay0_chip_hits[s*9 + c]++;
                lay0_stave_hits[s]++;
            }
            else if (layer->at(j) == 1)
            {
                n_hits_L1++;
                L1_stave.push_back(stave->at(j));
                L1_chip.push_back(chip->at(j));
                L1_row.push_back(row->at(j));
                L1_col.push_back(col->at(j));
                int s = stave->at(j);
                int c = chip->at(j);
                lay1_chip_hits[s*9 + c]++;
                lay1_stave_hits[s]++;
            }
            else if (layer->at(j) == 2)
            {
                n_hits_L2++;
                L2_stave.push_back(stave->at(j));
                L2_chip.push_back(chip->at(j));
                L2_row.push_back(row->at(j));
                L2_col.push_back(col->at(j));
                int s = stave->at(j);
                int c = chip->at(j);
                lay2_chip_hits[s*9 + c]++;
                lay2_stave_hits[s]++;
            }
        }

        // check if layer 0 occupancy is above threshold
        if ((100*double(n_hits_L0)/TOTAL_PIXELS_LAYER0) > target_occ)
        {

            for (int j = 0; j < 12*9; j++) L0_chip_hits.push_back(lay0_chip_hits[j]);
            for (int j = 0; j < 16*9; j++) L1_chip_hits.push_back(lay1_chip_hits[j]);
            for (int j = 0; j < 20*9; j++) L2_chip_hits.push_back(lay2_chip_hits[j]);
            for (int j = 0; j < 12; j++) L0_stave_hits.push_back(lay0_stave_hits[j]);
            for (int j = 0; j < 16; j++) L1_stave_hits.push_back(lay1_stave_hits[j]);
            for (int j = 0; j < 20; j++) L2_stave_hits.push_back(lay2_stave_hits[j]);
            
            tout->Fill();
            n_events_above_occ++;
        }

        n_strobes_processed++;

    }
    std::cout << "Processed " << n_strobes_processed << " strobes" << std::endl;
    std::cout << "Found " << n_events_above_occ << " events with layer 0 occupancy above " << target_occ << " %" << std::endl;

    fout->cd();
    tout->Write();
    fout->Close();
    f->Close();

    return 0;
}

