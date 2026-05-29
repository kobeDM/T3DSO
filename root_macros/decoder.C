/*
 * decoder.C - root macro
 * Usage: $root -l -b -q decoder.C
 * Author K. Miuchi
 * T3DSO binary data test
 * 2026 May
*/

#include <string>

int decoder(TString filename = "data"){
  // File name setting
  TString path = "./";
  TString inFilename = path + filename + ".dat";
  TString outFilename = path + filename + ".root";
  
  // data file open
  ifstream file;
  file.open(inFilename, ios::in | ios::binary);
    if(file.is_open()){
        cout << "file open " << inFilename << endl;
    }else{
        cerr << "file open error" << endl;
        return 1;
    }

    // Char_t   1 byte
    // Short_t  2 byte
    // Int_t    4 byte
    const Int_t   maxdepth=8192;
    Char_t  ch_tmp[1];
    //UChar_t *ch_tmp = new UChar_t[1];

    Char_t  buf1b;
    Short_t buf2b;
    Int_t   buf4b;

    Char_t header[64];


    // variable for header
    UChar_t fileHeader[4];
    UChar_t timeHeader[4];
    UChar_t boardSerialId[2];
    UShort_t boardSerial;
    Float_t time1BinWidth[1024];
    Float_t time2BinWidth[1024];
    Float_t time3BinWidth[1024];
    Float_t time4BinWidth[1024];

    // variable for event
    UChar_t eventHeader[4];
    UInt_t   eventSerial;
    char CHID[4][4];
    UInt_t activeCH[4];
    UInt_t activeCHs;

    /**    UShort_t Year;
    UShort_t Month;
    UShort_t Day;
    UShort_t Hour;
    UShort_t Minute;
    UShort_t Second;
    UShort_t MillSecond;
    UShort_t Range;**/

    /**    Char_t boardSerialId2[2];
    Short_t boardSerial2;
    Char_t triggerSellId[2];
    Short_t triggerSell;
    Char_t  ch1EventHeader[4];
    Int_t   ch1Scaler;
    UShort_t ch1wf[1024];
    Char_t  ch2EventHeader[4];
    Int_t   ch2Scaler;
    UShort_t ch2wf[1024];
    Char_t  ch3EventHeader[4];
    Int_t   ch3Scaler;
    UShort_t ch3wf[1024];
    Char_t  ch4EventHeader[4];
    Int_t   ch4Scaler;
    UShort_t ch4wf[1024];
    **/
    //UShort_t wf[2][4][maxdepth];
    Float_t wf[2][4][maxdepth];
    Short_t shorttmp;    
    for(int ch=0;ch<4;ch++){
      for(int bin=0;bin<maxdepth;bin++){
	wf[0][ch][bin] = bin;
	wf[1][ch][bin] = 0;
      }
    }

    auto fout = new TFile(outFilename,"recreate");
    auto tree = new TTree("tree","T3DSO data");
    //tree->Branch("time1BinWidth",time1BinWidth,"time1BinWidth[1024]/F");
    //tree->Branch("time2BinWidth",time2BinWidth,"time2BinWidth[1024]/F");

    //tree->Branch("eventSerial",&eventSerial,"eventSerial/I");
    //tree->Branch("Year",&Year,"Year/s");
    //tree->Branch("Month",&Month,"Month/s");
    //tree->Branch("Day",&Day,"Day/s");
    //tree->Branch("Hour",&Hour,"Hour/s"); 
    //tree->Branch("Minute",&Minute,"Minute/s");
    //tree->Branch("Second",&Second,"Second/s");
    //tree->Branch("MillSecond",&MillSecond,"MillSecond/s");
    //tree->Branch("Range",&Range,"Range/s");
    //tree->Branch("triggerSell",&triggerSell,"triggerSell/s");
    //tree->Branch("ch1wf",ch1wf,"ch1wf[1024]/s");
    //tree->Branch("ch2wf",ch2wf,"ch2wf[1024]/s");
    tree->Branch("wf",wf,"wf[2][4][8192]/F");
    //tree->Branch("ch1Scaler",&ch1Scaler,"ch1Scaler/I");
    //tree->Branch("ch2Scaler",&ch2Scaler,"ch2Scaler/I");


    
    file.seekg(0, std::ios::end);
    int headersize[4];
    long long int size = file.tellg();
    file.seekg(0);
    Char_t *data = new Char_t[size];
    file.read(data, size);
    std::cout << dec<<"size = " << size << "\n";

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
      // cout<<c_length[i];
    }
    headersize[0] = index;
    cout<<" Header("<<headersize[0]<<" bytes): ";
    for(int i=0;i<headersize[0];i++){
          cout<<data[i];
    }
    cout<<endl;

    //get metadata from user-header
    activeCHs=0;
    for(int i=0;i<headersize[0];i++){
      if(data[i]=='A'&&data[i+1]=='C'&&data[i+2]=='T'&&data[i+3]=='I'&&data[i+4]=='V'&&data[i+5]=='E'){
	cout<<"active channels: (";
	for (int j=0;j<4;j++){
	  ch_tmp[0]=data[i+j*3+18];
	  activeCH[j]=atoi(ch_tmp);
	  activeCHs+=activeCH[j];
	  cout<<ch_tmp[0]<<" ";
	  cout<<activeCH[j]<<" ";
	}	
      }
      if(data[i]==' '&&data[i+1]=='N'&&data[i+2]=='A'&&data[i+3]=='M'&&data[i+4]=='E'&&data[i+5]=='S'){
      	for (int j=0;j<4;j++){
      	  for (int k=0;k<3;k++){	  
	    CHID[j][k]=data[i+j*5+9+k];
	    //cout<<data[i+j*5+4+k];
	  }
	}
      }
    }
    cout<<") "<<activeCHs<<" channels are active."<<endl;

    
    //read the data from the beggining   
    file.seekg(0);
    int offset=0;
    for (int ch=0;ch<4;ch++){
      if(activeCH[ch]){
	cout <<"ID "<<ch<<" (";
	for (int k=0;k<3;k++){
	    cout<<CHID[ch][k];
	  }
	cout<<") is active."<<endl;
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
	cout<<" Header("<<headersize[ch]<<" bytes): ";
	for(int i=0;i<headersize[ch];i++){
          cout<<data[i];
	}
	cout<<endl;
	length[ch]=atoi(c_length);
	//index=0;
	for(int i=0;i<length[ch];i++){
	  //file.read((char*) &buf1b, 1);
	  shorttmp=data[offset+headersize[ch]+i]&0xff;
	  if(shorttmp>127){
	    shorttmp-=255;
	  }
	  wf[1][ch][i]=shorttmp;
	  //      shorttmp=data[index];
	  //if(wf[1][0][i]>255)
	  if(wf[1][ch][i]>255)
	    cout<<dec<<" "<<index<< " "<<wf[1][ch][i]<<endl;
	  index++;
	}	
      }
      offset+=headersize[ch]+length[ch]+2;//for data-end 2 bytes 
      cout<<dec<<" sample length:"<<length[ch]<<endl;
      cout<<dec<<" data end check(should be 0xa 0xa):";
      for(int i=0;i<2;i++){
	cout << dec<<offset-1-i<<" 0x"<<hex<<(data[offset-1-i]&0xff)<<" ";
      }
      cout<<"data position:"<<dec<<offset<<endl;
      cout<<endl;
    }


    cout<<endl;
    
    TGraph *gwf[4];
    string s_title;
    float x[maxdepth],y[maxdepth];

    for(int ch=0;ch<4;ch++){
      for(int i=0;i<length[ch];i++){
	y[i]=float(wf[1][ch][i]);
	x[i]=i;
      }
      gwf[ch] = new TGraph(length[ch], x, y);
      s_title=CHID[ch];
      s_title=s_title.substr(0,3)+";clock([/2ns]);ADC";
      //gwf[i]->SetTitle("CH1 waveform;X axis;Y axis");
      //gwf[i]->SetTitle(s_title.substr(0,3).c_str());
      gwf[ch]->SetTitle(s_title.c_str());
      gwf[ch]->SetMarkerStyle(8);
      gwf[ch]->SetMarkerSize(1.);
      gwf[ch]->SetLineWidth(2);
    }
    
    TCanvas *c1 = new TCanvas("c1", "c1", 800, 600);
    c1->Divide(2,2);


    for (int ch=0;ch<4;ch++){
      c1->cd(ch+1);
      gwf[ch]->Draw("APL");
    }
    c1->Update();
    //}

    

      /**    
while(0){
    //    while(true){
    //while(ievents<1){
        if(ievents % 10000 == 0) cout << ievents << endl;

        if(header[0]=='D' && header[1]=='R' && header[2]=='S'){
            cout << "Welcome, the DRS Version is " << header[3] << endl;
            if(!file.read((char*) &header, 4)) break;
        }

        if(header[0]=='T' && header[1]=='I' && header[2]=='M' && header[3]=='E'){

            file.read((char*) &header, 2); // 'B#'
            file.read((char*) &boardSerial, 2); // Serial Number
            cout << "Serial number is " << boardSerial << endl;
            if(!file.read((char*) &header, 4)) break;

            while(true){
                if(header[0]=='C' && header[1]=='0' && header[2]=='0' && header[3]=='1'){
                    cout << "ch1" << endl;
                    file.read((char*) &time1BinWidth, 4096);
                } else if(header[0]=='C' && header[1]=='0' && header[2]=='0' && header[3]=='2'){
                    cout << "ch2" << endl;
                    file.read((char*) &time2BinWidth, 4096);
                } else if(header[0]=='C' && header[1]=='0' && header[2]=='0' && header[3]=='3'){
                    cout << "ch3" << endl;
                    file.read((char*) &time3BinWidth, 4096);
                } else if(header[0]=='C' && header[1]=='0' && header[2]=='0' && header[3]=='4'){
                    cout << "ch4" << endl;
                    file.read((char*) &time4BinWidth, 4096);
                } else {
                    //if(!file.read((char*) &header, 4)) break;
                    break;
                }
                if(!file.read((char*) &header, 4)) break;
            }
        }


        if(header[0]=='E' && header[1]=='H' && header[2]=='D' && header[3]=='R'){
            
            //cout << "event header " << endl;
            ++ievents;**/
	    /**
            file.read((char*) &eventSerial, 4);
            file.read((char*) &Year, 2);
            file.read((char*) &Month, 2);
            file.read((char*) &Day, 2);
            file.read((char*) &Hour, 2);
            file.read((char*) &Minute, 2);
            file.read((char*) &Second, 2);
            file.read((char*) &MillSecond, 2);
            file.read((char*) &Range, 2);

            file.read((char*) &boardSerialId2, 2);
            file.read((char*) &boardSerial2, 2);
            file.read((char*) &triggerSellId, 2);
            file.read((char*) &triggerSell, 2);
	    **/
	    /**            if(!file.read((char*) &header, 4)) break;
            while(true){
	      if(header[0]=='C' && header[1]=='0' && header[2]=='0' && header[3]=='1'){
                    //cout << "ch1" << endl;
		file.read((char*) &ch1Scaler, 4);
		file.read((char*) &ch1wf, 2048);
		for(int i=0;i<1024;++i){
                        //if(ch1wf[i] < 20000) cout << i << "  " <<  ch1wf[i] << endl;
                        //cout << i << "  " <<  ch1wf[i] << endl;
                    }
                    for(int bin=0;bin<1024;++bin){
                        wf[0][bin] = ch1wf[bin];
                    }
                } else if(header[0]=='C' && header[1]=='0' && header[2]=='0' && header[3]=='2'){
                    //cout << "ch2" << endl;
                    file.read((char*) &ch2Scaler, 4);
                    file.read((char*) &ch2wf, 2048);
                    for(int bin=0;bin<1024;++bin){
                        wf[1][bin] = ch2wf[bin];
                    }
                } else if(header[0]=='C' && header[1]=='0' && header[2]=='0' && header[3]=='3'){
                    //cout << "ch3" << endl;
                    file.read((char*) &ch3Scaler, 4);
                    file.read((char*) &ch3wf, 2048);
                    for(int bin=0;bin<1024;++bin){
                        wf[2][bin] = ch3wf[bin];
                    }
                } else if(header[0]=='C' && header[1]=='0' && header[2]=='0' && header[3]=='4'){
                    //cout << "ch4" << endl;
                    file.read((char*) &ch4Scaler, 4);
                    file.read((char*) &ch4wf, 2048);
                    for(int bin=0;bin<1024;++bin){
                        wf[3][bin] = ch4wf[bin];
                    }
	      } else {
		tree->Fill();
		for(int ch=0;ch<4;++ch){
		  for(int bin=0;bin<1024;++bin){
		    wf[ch][bin] = 0;
		  }
		}
		break;
	      }
	      if(!file.read((char*) &header, 4)){
                    goto fileover;
                }
            }
        }
	}**/

	    // fileover:
 tree->Fill();
 tree->Write();
 fout->Close();
 cout << ievents << " Events written." << endl;
 
 return 0;
    }
