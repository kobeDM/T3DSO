/*
 * drawWF_all.C - root macro
 * to draw and decode binary file
 * Usage: $root 'drawWF_all.C("filename",evID)'
 * Author K. Miuchi
 * T3DSO binary data read
 * 2026 May
*/

#include <string>

int drawWF_1ev(string filename, const Int_t  evID){
  string inFilename = filename;
  //TString outFilename = filename.replace(filename.find(".")+1,3,"root");
  TString outFilename = "oneevent.root";

  //Canvas
  TCanvas *c1 = new TCanvas("c1", "raw waveforms", 1600, 600);
  c1->Divide(4,2);
  TCanvas *c2 = new TCanvas("c2", "calibrated waveforms", 1600, 600);
  c2->Divide(4,2);
  gStyle->SetStatX(0.88);
  gStyle->SetStatY(0.35);

  const Int_t verbose=0;
  //waveform constants
  const Int_t   maxdepth=8192;
  const Int_t chN = 4;
  Int_t clkN=0;

  //buffers 
  Char_t  buf1b; //1 byte buffer
  Short_t buf2b; //2 bytes buffer
  Int_t   buf4b; //4 bytes buffer
  char ch_tmp[1]; //1 character buffer
    
  // data file open
  ifstream file;
  //cerr<<"reading "<<inFilename<<endl;
  file.open(inFilename, ios::in | ios::binary);
  if(file.is_open()){
    cout << "file open " << inFilename << endl;
  }else{
    cerr << "file "<< inFilename << " open error" << endl;
    return 1;
  }

  //cal factors
  UChar_t TDIV_char[8],VDIV_char[8],Voffset_char[8],sampling_char[16];
  string TDIV_str="";
  string VDIV_str="";
  string Voffset_str="";
  string sampling_str="";
  string TIMESTAMP_str="";
  Short_t VDIV_char_length,Voffset_char_length,TDIV_char_length,sampling_char_length;
  Float_t TDIV_scale_base;// 
  Float_t TDIV_scale;// ns
  Float_t sampling_scale;// ns/clock
  Float_t Voffset[4];// V 
  Float_t Vscale[4];// V/ADC

  string s_title;
  
  //histgrams
  /**
  TH2D *histArr[chN],*histArr_LE[chN],*histArr_Vs[chN],*histArr_Vs_LE[chN];
  for ( int ch = 0; ch < chN; ch++ ) {     
    histArr[ch]= new TH2D( Form( "hist%d", ch+1 ), Form( "%s CH %d;clock;adc",filename.c_str(),ch+1 ), 350, 0, 3500, 128, -128., 128. ) ;
    histArr_LE[ch]= new TH2D( Form( "hist_LE%d", ch+1 ), Form( "%s CH %d (zoomed);clock;adc",filename.c_str(), ch+1 ), 150, 700, 850, 128, 0., 128. ) ;
    histArr_Vs[ch]= new TH2D( Form( "hist_Vs%d", ch+1 ), Form( "%s CH %d;ns;V",filename.c_str(), ch+1 ), 350, 0, 7000, 128, -1., 1. ) ;
      histArr_Vs_LE[ch]= new TH2D( Form( "hist_Vs_LE%d", ch+1 ), Form( "%s CH %d (zoomed);ns;V",filename.c_str(), ch+1 ), 150, 900, 1200, 150, -0.2, .8 ) ;
      }**/
    
    int offset=0;
    int ev=0;
    TGraph *gwf_LE[chN];
    TGraph *gwf_Vs[chN];
    TGraph *gwf[chN];
    TGraph *gwf_Vs_LE[chN];
    
    // variable for event
    UChar_t eventHeader[chN];
    UInt_t   eventSerial;
    char CHID[chN][4];
    UInt_t activeCH[chN];
    UInt_t activeCHs;
    UInt_t totEvt=0;
    uint64_t timestamp;
    uint64_t timestamp_sub;

    Float_t wf[2][chN][maxdepth];
    Float_t wf_Vs[2][chN][maxdepth];
    float x[maxdepth],y[maxdepth],y_Vs[maxdepth],x_Vs[maxdepth];
    Short_t shorttmp;    


    auto fout = new TFile(outFilename,"recreate");
    auto tree = new TTree("tree","T3DSO data");
    //    tree->Branch("chN",&chN,"chN/I");
    tree->Branch("clkN",&clkN,"clkN/I");
    tree->Branch("eventID",&ev,"ev/I");
    tree->Branch("timestamp",&timestamp,"timestamp/I");
    tree->Branch("timestamp_subsec",&timestamp_sub,"timestamp_sub/I");
    tree->Branch("filename",&filename,"filename/s");
    tree->Branch("TDIV_scale",&TDIV_scale,"TDIV_scale/F");
    tree->Branch("sampling_scale",&sampling_scale,"sampling_scale/F");
    tree->Branch("Vscale",Vscale,"Vscale[4]/F");
    tree->Branch("wf",wf,"wf[2][4][8192]/F");
    tree->Branch("wf_Vs",wf_Vs,"wf_Vs[2][4][8192]/F");
    
    file.seekg(0, std::ios::end);
    int headersize[4];
    long long int size = file.tellg();
    
    file.seekg(0);
    Char_t *data = new Char_t[size];
    file.read(data, size);
    std::cout << dec<<" total data size = " << size << "\n";

    //Read Events
    ULong64_t ievents = 0;
    ULong64_t length[4];
    Char_t c_length[10];

    file.seekg(0);
    
    //data-header check
    int index=0;
    while(true){
      //if(data[index]=='9'){
      if(data[index]=='#'&&data[index+1]=='9'){
	//index++;	
	index+=2;	
	break;
      }
       index++;
    }
    for(int i=0;i<9;i++){//read 9 digits for event length
      c_length[i]=data[index];
      index++;
    }
    for(int ch=0;ch<chN;ch++){
      headersize[ch] = index;
      //cout<<" Header("<<headersize[0]<<" bytes): ";
      length[ch]=atoi(c_length);
    }
    for(int i=0;i<headersize[0];i++){
      if(verbose)
          cout<<data[i];
    }
    cout<<endl;

    //get metadata from user-header
    activeCHs=0;
    for(int i=0;i<headersize[0];i++){
      if(data[i]=='A'&&data[i+1]=='C'&&data[i+2]=='T'&&data[i+3]=='I'&&data[i+4]=='V'&&data[i+5]=='E'){//"ACITVE" for active detector 
	//cout<<"active channels: (";
	for (int j=0;j<4;j++){
	  ch_tmp[0]=data[i+j*3+18];
	  activeCH[j]=atoi(ch_tmp);
	  activeCHs+=activeCH[j];
	  //cout<<ch_tmp[0]<<" ";
	  //cout<<activeCH[j]<<" ";
	}	
      }
      if(data[i]==' '&&data[i+1]=='N'&&data[i+2]=='A'&&data[i+3]=='M'&&data[i+4]=='E'&&data[i+5]=='S'){//NAMES for the detector name
     	for (int j=0;j<4;j++){
      	  for (int k=0;k<3;k++){	  
	    CHID[j][k]=data[i+j*5+9+k];
	    //cout<<data[i+j*5+4+k];
	  }
	}
      }
      if(data[i+1]=='T'&&data[i+2]=='D'&&data[i+3]=='I'&&data[i+4]=='V'){//TDIV
	TDIV_char_length=0;
	while (data[i+6+TDIV_char_length]!='S'){
	  TDIV_char[TDIV_char_length]=data[i+6+TDIV_char_length];
	  TDIV_char_length++;
	}
      }
      if(data[i+1]=='V'&&data[i+2]=='O'&&data[i+3]=='L'&&data[i+4]=='T'&&data[i+5]=='_'&&data[i+6]=='D'&&data[i+7]=='I'&&data[i+8]=='V'){//VDIV
	ch_tmp[0]=data[i-1];
	VDIV_char_length=0;
	while (data[i+9+VDIV_char_length]!='V'){
	  VDIV_char[VDIV_char_length]=data[i+9+VDIV_char_length];
	  VDIV_char_length++;	  
	}
	VDIV_str="";		  
	for(int j=0;j<VDIV_char_length;j++){
	  VDIV_str.push_back(VDIV_char[j]);
	}
	Vscale[atoi(ch_tmp)-1]=stof(VDIV_str)*8/256;
	//cout<<"############ ch= "<<ch_tmp[0]<<endl;
	//cout<<"############ VDIV= "<<VDIV_str<<", "<<endl;
      }
      if(data[i+1]=='I'&&data[i+2]=='M'&&data[i+3]=='E'&&data[i+4]=='S'&&data[i+5]=='T'&&data[i+6]=='A'&&data[i+7]=='M'&&data[i+8]=='P'){//TIMESTAMP
	TIMESTAMP_str="";		  
	for(int j=0;j<18;j++){
	  TIMESTAMP_str.push_back(data[i+j+10]);
	}
      }
      if(data[i+1]=='O'&&data[i+2]=='F'&&data[i+3]=='F'&&data[i+4]=='S'&&data[i+5]=='E'&&data[i+6]=='T'){//VOFFSET
	ch_tmp[0]=data[i-1];
	Voffset_char_length=0;
	while (data[i+8+Voffset_char_length]!='V'){
	  Voffset_char[Voffset_char_length]=data[i+8+Voffset_char_length];
	  Voffset_char_length++;	  
	}
	Voffset_str="";		  
	for(int j=0;j<Voffset_char_length;j++){
	  Voffset_str.push_back(Voffset_char[j]);
	}
	Voffset[atoi(ch_tmp)-1]=stof(Voffset_str);
	//cout<<"############ ch= "<<ch_tmp[0]<<endl;
	//cout<<"############ Voffset= "<<Voffset_str<<", "<<endl;
      }
      if(data[i+1]=='R'&&data[i+2]=='A'&&data[i+3]=='T'&&data[i+4]=='E'){//sampling rate
	sampling_char_length=0;
	while (data[i+5+sampling_char_length]!='S'){
	  sampling_char[sampling_char_length]=data[i+5+sampling_char_length];
	  sampling_char_length++;
	}
      }
    }
    //cout<<") "<<activeCHs<<" channels are active."<<endl;
    TDIV_char_length--;
   if(TDIV_char[TDIV_char_length]=='n'){
     //cerr<<"ns"<<endl;
     TDIV_scale_base=1e-9;
    }
    else if(TDIV_char[TDIV_char_length]=='u'){
      TDIV_scale_base=1e-6;
    }
    else if(TDIV_char[TDIV_char_length]=='m'){
      TDIV_scale_base=1e-3;
    }
    else{
      TDIV_scale_base=1.;
      TDIV_char_length++;
      }
    for(int i=0;i<TDIV_char_length;i++){
      TDIV_str.push_back(TDIV_char[i]);
    }
    for(int i=0;i<sampling_char_length;i++){
      sampling_str+=sampling_char[i];
    }
    TDIV_scale=stof(TDIV_str)*TDIV_scale_base/1e-9;
    sampling_scale=1/stof(sampling_str)*1e9;//ns/clock
    //cout<<"############ SAMPLING= "<<sampling_str<<endl;
    //cout<<"############ TDIV= "<<TDIV_scale<<" ns"<<endl;
    //cout<<"############ SAMPLING= "<<sampling_scale<<" ns/clock"<<endl;
    //TDIV_scale

    //wf intialise
    for(int ch=0;ch<4;ch++){      
      //      cout<<"ch"<<ch <<": Vscale="<<Vscale[ch]<<"V/adc, Voffset="<<Voffset[ch]<<"V"<<endl;
        for(int bin=0;bin<maxdepth;bin++){
	wf[0][ch][bin] = bin;
	wf_Vs[0][ch][bin] = bin*sampling_scale;
	wf[1][ch][bin] = wf_Vs[1][ch][bin]= 0;
      }
    }
    clkN=length[0];
    
    file.seekg(0);

    
    //main loop: read the data from the beggining   
    //while(ev<maxevents){
    while(ev<evID+1){
      //if(ev%100==0) cout<<"reading "<<ev<<"/"<<maxevents<<"\r";
      cout <<"reading "<<ev;//<<"/"<<maxevents<<"\t"<<offset;
      //while(ev<evtMax){
      if(offset+4*(headersize[0]+length[0])>size) break;
      for (int i=offset;i<size;i++){
	if(data[i+1]=='I'&&data[i+2]=='M'&&data[i+3]=='E'&&data[i+4]=='S'&&data[i+5]=='T'&&data[i+6]=='A'&&data[i+7]=='M'&&data[i+8]=='P'){//TIMESTAMP
	  TIMESTAMP_str="";		  
	  for(int j=0;j<18;j++){
	    TIMESTAMP_str.push_back(data[i+j+10]);
	  }
	}
      }
      timestamp=stoi(TIMESTAMP_str.substr(0,TIMESTAMP_str.find(".")));
      timestamp_sub=stoi(TIMESTAMP_str.substr(TIMESTAMP_str.find(".")+1,TIMESTAMP_str.size()-TIMESTAMP_str.find(".")));

      for (int ch=0;ch<chN;ch++){
	if(activeCH[ch]){
	  index=0;
	  while(true){	  
	    if(data[offset+index]=='#'&&data[offset+index+1]=='9'){
	      index+=2;	
	      break;
	    }
	    index++;
	  }
	  for(int i=0;i<9;i++){//read 9 digits for event length
	    c_length[i]=data[offset+index];
	    index++;
	  }
	  headersize[ch] = index;
	  
	  for(int i=0;i<headersize[ch];i++){
	    if(verbose)
	    if(ev==0) cout<<data[i+offset];
	  }
	  length[ch]=atoi(c_length);

	//index=0;
	if(ev==evID){
	for(int i=0;i<length[ch];i++){
	  //file.read((char*) &buf1b, 1);
	  shorttmp=data[offset+headersize[ch]+i]&0xff;
	  if(shorttmp>127){
	    shorttmp-=255;
	  }
	  wf[1][ch][i]=shorttmp;
	  wf_Vs[1][ch][i]=shorttmp*Vscale[ch];
	  //      shorttmp=data[index];
	  //if(wf[1][0][i]>255)
	  //if(wf[1][ch][i]>255)
	  //cout<<dec<<" "<<index<< " "<<wf[1][ch][i]<<endl;
	  index++;
	}
	//if(ev==0){
	if(length[0]>0){
	  clkN=length[0];
	}
	}
	//cout << "event "<<ev<<"\t"<<length[0]<<" "<<length[1]<<" "<<length[2]<<" "<<length[3]<<endl;
	}
	offset+=headersize[ch]+length[ch]+2;//for data-end 2 bytes
	//cout<<ev<<"\t"<<wf[0][ch][0]<<"\t"<<wf[1][ch][0]<<endl;
	if(ev==evID){
	  //cout <<" ### event 0 ###"<<endl;
	  
	  cout<<" event "<<ev;
	  cout<<" ch"<<ch << ": header "<<headersize[ch]<<" bytes, ";
	  cout<<dec<<"data "<<length[ch]<<"clock, ";
	  cout<<"Vscale="<<Vscale[ch]<<"V/adc, ";//, Voffset="<<Voffset[ch]<<"V, ";
	  cout<<dec<<" timestamp:"<<timestamp<<" "<<endl;
	}
      }
      ev++;
    }
    
  //    for(int ch=0;ch<4;ch++){
      //   cout<<"chN"<<chN<<",clkN"<<clkN<<endl;
  for ( int ch = 0; ch < 4; ch++ ) {

    for(int i=0;i<length[ch];i++){
      y[i]=float(wf[1][ch][i]);
      x[i]=i;
      y_Vs[i]=float(wf_Vs[1][ch][i]);
      x_Vs[i]=float(wf_Vs[0][ch][i]);
     }
    gwf[ch] = new TGraph(length[ch], x, y);
    gwf_Vs[ch] = new TGraph(length[ch], x_Vs, y_Vs);
    gwf_LE[ch] = new TGraph(length[ch], x, y);
    gwf_Vs_LE[ch] = new TGraph(length[ch], x_Vs, y_Vs);
    s_title=Form("%s event %d CH %d",filename.c_str(),evID,ch+1);
    gwf[ch]->SetTitle((s_title+";clock;adc").c_str());
    gwf_LE[ch]->SetTitle((s_title+"(zoom);clock;adc").c_str());
    gwf_Vs[ch]->SetTitle((s_title+";ns;V").c_str());
    gwf_Vs_LE[ch]->SetTitle((s_title+"(zoom);ns;V").c_str());
    //    gwf[ch]->SetTitle(s_title.substr(0,3).c_str());
    //gwf[ch]->SetTitle(s_title.c_str());
    gwf[ch]->SetMarkerStyle(8);
    gwf[ch]->SetMarkerSize(1.);
    gwf[ch]->SetLineWidth(2);
    gwf_Vs[ch]->SetMarkerStyle(8);
    gwf_Vs[ch]->SetMarkerSize(1.);
    gwf_Vs[ch]->SetLineWidth(2);
    TAxis *axis = gwf_LE[ch]->GetXaxis();
    axis->SetLimits(400.,600); 
    TAxis *axis_Vs = gwf_Vs_LE[ch]->GetXaxis();
    axis_Vs->SetLimits(950.,1200);                 // along X
   //gr1->GetHistogram()->SetMaximum(20.);   // along          
   //gr1->GetHistogram()->SetMinimum(-20.);  //   Y     
    //}
    //for ( int ch = 0; ch < chN; ch++ ) {
      //for ( int clk = clk_offset; clk < clkN; clk++ ) {
    //for ( int clk = 0; clk < clkN; clk++ ) {
          //cerr<<ch<<" "<<clk<<" "<<wf[1][ch][clk]<<endl<<flush;
          //if(evtID==2){
            //      pTree->Show(evt);
	  //          histArr->Fill(0.,0.);
	  //histArr[ch]->Fill( double(wf[0][ch][clk]),double( wf[1][ch][clk]) );
	  //histArr_LE[ch]->Fill( double(wf[0][ch][clk]),double( wf[1][ch][clk]) );
	  //  histArr_Vs[ch]->Fill( double(wf_Vs[0][ch][clk]),double( wf_Vs[1][ch][clk]) );
	  //histArr_Vs_LE[ch]->Fill( double(wf_Vs[0][ch][clk]),double( wf_Vs[1][ch][clk]) );
	  //histArr_LE[ch]->Fill( wf[0][ch][clk], wf[1][ch][clk] );
    //  }
  
    //histArr[0]->Fill(0.,0.);
    //for (int ch=0;ch<4;ch++){
      //c1->cd(ch+1);
      //gwf[ch]->Draw("APL");
    //}
    //c1->Update();
    //sleep(3);
  tree->Fill();
  }
  //    ev++;

    //    cout<<"\n";  
    cout<<dec<<" data end check (should be 0xa 0xa):";
    for(int i=0;i<2;i++){
      cout << dec<<offset-1-i<<" 0x"<<hex<<(data[offset-1-i]&0xff)<<" ";
    }
  
    //cout<<"data position:"<<dec<<offset<<endl;
    //cout<<endl;

    totEvt=ev;
    //}  
    // fileover:
    
    //cerr<<endl;
  for (int ch=0;ch<chN;ch++){
    //c4->cd(ch+1);
    c2->cd(ch+5);
    gwf_Vs_LE[ch]->Draw("AL");
    //histArr_Vs_LE[ch]->Draw("colz");
    c2->cd(ch+1);
    gwf_Vs[ch]->Draw("AL");
    //histArr_Vs[ch]->Draw("colz");
    c1->cd(ch+5);
    gwf_LE[ch]->Draw("AL");
    //histArr_LE[ch]->Draw("colz");
    c1->cd(ch+1);
    gwf[ch]->Draw("AL");
    //  histArr[ch]->Draw("colz");
   }

  //  TH2D* pHist_LE = histArr_LE.at( 0 );

  
    //pHist_LE->Draw( "colz" );
  //histArr[0]->Draw("COLZ");
  //gwf[0]->Draw("APL");	
  //c1->Update();
    //sleep(3);
 tree->Write();
 c1->Write();
 c2->Write();
 fout->Close();
 cerr<<" "<<chN<<"channels * "<<clkN<<"clocks * "<<ev<<"events were written in "<<outFilename<<endl;
 //cout << ev << " Events written." << endl;
 
 return 0;
}
