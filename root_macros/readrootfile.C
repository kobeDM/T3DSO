/*
 * readrootfile.C - root macro
 * Usage:
 * Author K. Miuchi
 * T3DSO binary data test
 * 2026 May
*/

#include <string>
//#include <cstdlib>
void readrootfile(string inputFilePath){
  TFile *file = TFile::Open(inputFilePath.c_str( ) );
  TTree* pTree=(TTree*)file->Get("tree");
  
  const int totEvt=pTree->GetEntries();

  uint64_t timestamp;
  uint64_t timestamp_sub;

  const Int_t chN = 4;
  Int_t clkN;
  Int_t eventID;
  const Int_t   maxdepth=8192;
  Float_t wf[2][chN][maxdepth];
  Float_t wf_Vs[2][chN][maxdepth];
  Float_t TDIV_scale_base;// 
  Float_t TDIV_scale;// ns
  Float_t sampling_scale;// ns/clock
  Float_t Voffset[4];// V 
  Float_t Vscale[4];// V/ADC
    
  cerr<<"total  "<<totEvt<<" events"<<endl;
  pTree->SetBranchAddress("Vscale",Vscale);
  pTree->SetBranchAddress("clkN",&clkN);
  pTree->SetBranchAddress("eventID",&eventID);
  pTree->SetBranchAddress("timestamp",&timestamp);
  pTree->SetBranchAddress("timestamp_subsec",&timestamp_sub);
  pTree->SetBranchAddress("TDIV_scale",&TDIV_scale);
  pTree->SetBranchAddress("sampling_scale",&sampling_scale);
  pTree->SetBranchAddress("wf",wf);

  for(int ev=0;ev<totEvt;ev++){
    pTree->GetEntry(ev);
    cout<<ev;
    for ( int ch = 0; ch < chN; ch++ ) {
      cout<<" "<<wf[0][ch][0]<<" "<<wf[1][ch][0];
    }
    cout<<endl;
  }
}
