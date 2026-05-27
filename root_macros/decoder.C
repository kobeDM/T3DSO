/*
 * decoder.C - root macro
 * Usage: $root -l -b -q decoder.C
 * Author K. Miuchi
 * T3DSO binary data test
 * 2026 May
*/


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
    UShort_t Year;
    UShort_t Month;
    UShort_t Day;
    UShort_t Hour;
    UShort_t Minute;
    UShort_t Second;
    UShort_t MillSecond;
    UShort_t Range;

    Char_t boardSerialId2[2];
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

    UShort_t wf[4][1024];
    for(int ch=0;ch<4;++ch){
        for(int bin=0;bin<1024;++bin){
            wf[ch][bin] = 0;
        }
    }

    auto fout = new TFile(outFilename,"recreate");
    auto tree = new TTree("tree","DRS4 data");
    //tree->Branch("time1BinWidth",time1BinWidth,"time1BinWidth[1024]/F");
    //tree->Branch("time2BinWidth",time2BinWidth,"time2BinWidth[1024]/F");

    tree->Branch("eventSerial",&eventSerial,"eventSerial/I");
    tree->Branch("Year",&Year,"Year/s");
    tree->Branch("Month",&Month,"Month/s");
    tree->Branch("Day",&Day,"Day/s");
    tree->Branch("Hour",&Hour,"Hour/s"); 
    tree->Branch("Minute",&Minute,"Minute/s");
    tree->Branch("Second",&Second,"Second/s");
    tree->Branch("MillSecond",&MillSecond,"MillSecond/s");
    tree->Branch("Range",&Range,"Range/s");
    tree->Branch("triggerSell",&triggerSell,"triggerSell/s");
    tree->Branch("ch1wf",ch1wf,"ch1wf[1024]/s");
    tree->Branch("ch2wf",ch2wf,"ch2wf[1024]/s");
    tree->Branch("wf",wf,"wf[4][1024]/s");
    tree->Branch("ch1Scaler",&ch1Scaler,"ch1Scaler/I");
    tree->Branch("ch2Scaler",&ch2Scaler,"ch2Scaler/I");


    //Read Events
    ULong64_t ievents = 0;

    //Header check
    int index=0;
    while(true){
    //for(int i=0;i<20;i++){
      file.read((char*) &buf1b, 1);
      header[index]=buf1b;
      //cout << buf1b;
      index++;
      if(buf1b=='9'){
	for(int i=0;i<5;i++){//read 00000
	  file.read((char*) &buf1b, 1);
	  header[index]=buf1b;
	  index++;
	}
	break;
      }
    }

    for(int i=0;i<5;i++){
      file.read((char*) &buf1b, 1);
      header[index]=buf1b;
      if(buf1b==' ')break;//end of buffer
      index++;
    }
    cout<<" Header("<<index<<"bytes):";
    for(int i=0;i<index;i++){
      cout<<header[i];
    }
    cout<<endl;


    for(int i=0;i<400;i++){
      file.read((char*) &buf1b, 1);
      cout<<hex<<"0x"<<buf1b<<" ";
    }
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
            ++ievents;

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

            if(!file.read((char*) &header, 4)) break;
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
    }

    fileover:
	tree->Fill();
    tree->Write();
    fout->Close();
    cout << ievents << " Events written." << endl;

    return 0;
}
