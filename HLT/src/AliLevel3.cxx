//Author:        Uli Frankenfeld
//Author:        Anders Strand Vestbo
//Last Modified: 13.12.2000

#include "AliL3Logging.h"

#include <TFile.h>
#include <TDirectory.h>
#include <TClonesArray.h>
#include <TStopwatch.h>
#include <iostream.h>

//#include <AliTPCcluster.h>
//#include "AliTPCParam.h"
#include "AliLevel3.h"
#include "AliL3ConfMapper.h"
#include "AliL3Vertex.h"
#include "AliL3VertexFinder.h"
#include "AliL3TrackMerger.h"
#include "AliL3GlobalMerger.h"
#include "AliL3InterMerger.h"
#include "AliL3ConfMapPoint.h"
#include "AliL3ConfMapTrack.h"
#include "AliL3Transform.h"
#include "AliL3ClustFinder.h"
#include "AliL3DigitData.h"
#include "AliL3TrackArray.h"
#include "AliL3MemHandler.h"
#include "AliL3FileHandler.h"
#include "AliL3Benchmark.h"

#include "AliL3DigitData.h"
#include "AliL3TrackSegmentData.h"
#include "AliL3SpacePointData.h"
#include "AliL3VertexData.h"

//_______________________________________
//
//  AliLevel3
//
//  Interface class for Level3 tracker.
//  Tracking is done by calling constructor with input,output 
//  given as argument. 
//  You must always remember to set the tracking parameters. E.g.:
// 
//  AliLevel3 *level3 = new AliLevel3(inputfile,outputfile);
//  level3->SetTrackerParam(); //Sets default tracking parameters
//  level3->ProcessSector(2,2);  //Does tracking on sector 2 (actually 2+38)

ClassImp(AliLevel3)

AliLevel3::AliLevel3(){
  fInputFile=0;
  fOutputFile=0;
  Init();
}

AliLevel3::AliLevel3(Char_t *infile,Char_t *outfile){
  //Constructor. Calls constructor of the tracker, vertexfinder and merger classes.

  fOutputFile = new TFile(outfile,"NEW");
  
  if(!fOutputFile->IsOpen())
    {
    LOG(AliL3Log::kWarning, "AliLevel3::AliLevel3","File Open")
    <<"Delete your old "<<outfile<<" file!"<<ENDLOG;
    }
  fInputFile = new TFile(infile,"READ");
  
  if(!fInputFile->IsOpen())
    {
    LOG(AliL3Log::kError,"AliLevel3::AliLevel3","File Open")
    <<"Inputfile "<<infile<<" does not exist"<<ENDLOG;
    return;
    }
  
  Init();
}

AliLevel3::AliLevel3(TFile *in, TFile *out){
  fOutputFile = out;
  fInputFile  =  in;
  if(!in){
    LOG(AliL3Log::kError,"AliLevel3::AliLevel3","File Open")
    <<"Pointer to InFile 0x0!"<<ENDLOG;
    return;
  }  
  if(!out){
    LOG(AliL3Log::kError,"AliLevel3::AliLevel3","File Open")
    <<"Pointer to OutFile 0x0!"<<ENDLOG;
    return;
  }  
  
  if(!fOutputFile->IsOpen())
    {
    LOG(AliL3Log::kWarning,"AliLevel3::AliLevel3","File Open")
    <<"no output file!"<<ENDLOG;
      return;
    }
  if(!fInputFile->IsOpen())
    {
    LOG(AliL3Log::kError,"AliLevel3::AliLevel3","File Open")
    <<"Inputfile does not exist"<<ENDLOG;
      return;
    }
  Init();
}

void AliLevel3::Init(){
  fWriteOut = kFALSE;
  fGlobalMerger=0;
  fTransformer = new AliL3Transform();
  fDoRoi = kFALSE;
  fUseBinary =kFALSE;
  SetPath("");
  fFindVertex =kTRUE;
  fNPatch = 5;   //number of patches change row in process 
  fVertexFinder = new AliL3VertexFinder();
  fVertex = new AliL3Vertex();
  fTracker = new AliL3ConfMapper();
  fTrackMerger = new AliL3TrackMerger(fNPatch);
  fInterMerger = new AliL3InterMerger();
  fFileHandler = new AliL3FileHandler();
  fFileHandler->SetAliInput(fInputFile);
  fBenchmark = new AliL3Benchmark();
}

void AliLevel3::DoBench(char* name){
  fBenchmark->Analyze(name);
}

void AliLevel3::DoMc(char* file){
  if(!fFileHandler->IsDigit())
    fFileHandler->SetMCOutput(file);
}

AliLevel3::~AliLevel3(){
  //Destructor
  if(fVertexFinder)  delete fVertexFinder;
  if(fVertex)  delete fVertex;
  if(fTracker) delete fTracker;
  if(fTransformer) delete fTransformer;
  if(fTrackMerger) delete fTrackMerger;
  if(fInterMerger) delete fInterMerger;
  if(fFileHandler) delete fFileHandler;
}

void AliLevel3::SetTrackerParam(Int_t phi_segments, Int_t eta_segments,
				   Int_t trackletlength, Int_t tracklength,
				   Int_t rowscopetracklet, Int_t rowscopetrack,
				   Double_t min_pt_fit, Double_t maxangle,
				   Double_t goodDist, Double_t hitChi2Cut,
				   Double_t goodHitChi2, Double_t trackChi2Cut,
				   Int_t maxdist)
{
  //Set parameters input to the tracker
  //If no arguments are given, default parameters will be used
  
  fTracker->SetNSegments(phi_segments,eta_segments);
  fTracker->MainVertexSettings(trackletlength,tracklength,rowscopetracklet,rowscopetrack);
  fTracker->SetMaxDca(min_pt_fit);
  fTracker->SetTrackletCuts(maxangle,goodDist,true);
  fTracker->SetTrackCuts(hitChi2Cut,goodHitChi2,trackChi2Cut,maxdist);

  fTracker->SetParamDone(true);
}

void AliLevel3::ProcessEvent(Int_t first,Int_t last){
  //Do tracking on all slices in region [first,last]
  //Slices numbering in TPC goes from 0-35, which means that 1 slice
  //corresponds to inner+outer sector.E.g. slice 2 corresponds to
  //inner=2 + outer=38.
  fGlobalMerger= new AliL3GlobalMerger(first,last);  
  for(Int_t i=first; i<=last; i++){
    ProcessSlice(i);
    fGlobalMerger->SetVertex(fVertex);
    fGlobalMerger->SetTransformer(fTransformer);
    fGlobalMerger->InitSlice(i);
    fBenchmark->Start("Fill Global Merger");
    fGlobalMerger->FillTracks(fNTrackData,fTrackData);
    fBenchmark->Stop("Fill Global Merger");
    fFileHandler->Free();   //free the memory
    fNTrackData=0;
    fTrackData=0;
  }
  fBenchmark->Start("Global Merger");
  fGlobalMerger->Merge();
//  fGlobalMerger->SlowMerge();
  fBenchmark->Stop("Global Merger");

  if(fWriteOut) WriteResults(); 
  delete fGlobalMerger; fGlobalMerger = 0;
}

void AliLevel3::ProcessSlice(Int_t slice){
  char name[256];
  Bool_t isdigit = fFileHandler->IsDigit();
//  Int_t row[5][2] = {{ 0,173}};
  Int_t row[5][2] = {{ 0, 45},{46,77},{78,109},{110,141},{142,173}};
  const Int_t maxpoints=100000;
  const Int_t pointsize = maxpoints * sizeof(AliL3SpacePointData);
  AliL3MemHandler *memory = new AliL3MemHandler();

  fTrackMerger->Reset();
  fTrackMerger->SetTransformer(fTransformer);
  fTrackMerger->SetRows(row[0]);
  for(Int_t patch=fNPatch-1;patch>=0;patch--){
    fFileHandler->Init(slice,patch,row[patch]);
    fFileHandler->Init(fTransformer);
    UInt_t npoints=0;
    AliL3SpacePointData *points =0;
    UInt_t ndigits=0;
    AliL3DigitRowData *digits =0;
    if(fUseBinary){

      if(!fDoRoi){ 
        if(0){     //Binary to Memory
          fFileHandler->Free();
          sprintf(name,"%sdigits_%d_%d.raw",fPath,slice,patch);
          fFileHandler->SetBinaryInput(name);
          digits= (AliL3DigitRowData *)fFileHandler->CompBinary2Memory(ndigits);
          fFileHandler->CloseBinaryInput(); 
        }

        if(1){     //Binary to Memory with Benchmark 
          fFileHandler->Free();
          sprintf(name,"%sdigits_%d_%d.raw",fPath,slice,patch);
          memory->SetBinaryInput(name);
          UInt_t compsize=memory->GetFileSize();
          UInt_t *comp=(UInt_t *)memory->Allocate(compsize);
          memory->CompBinary2CompMemory(ndigits,comp);
          memory->CloseBinaryInput();
          UInt_t datasize=memory->GetMemorySize(ndigits,comp);
          digits=(AliL3DigitRowData *)fFileHandler->Allocate(datasize);
          fBenchmark->Start("Unpacker"); 
          fFileHandler->CompMemory2Memory(ndigits,digits,comp); 
          fBenchmark->Stop("Unpacker");
          memory->Free();
        }
  
        if(0){     //Binary to Memory with Random
          fFileHandler->Free();
          fFileHandler->ResetRandom();
          fFileHandler->SetRandomCluster(100);
          fFileHandler->SetNGenerate(100);
          sprintf(name,"%sdigits_%d_%d.raw",fPath,slice,patch);
          memory->SetBinaryInput(name);
          UInt_t compsize=memory->GetFileSize();
          UInt_t *comp=(UInt_t *)memory->Allocate(compsize);
          memory->CompBinary2CompMemory(ndigits,comp);
          memory->CloseBinaryInput();
          UInt_t dsize=memory->GetMemorySize(ndigits,comp);
          UInt_t rsize=fFileHandler->GetRandomSize();       
          digits=(AliL3DigitRowData*)fFileHandler->Allocate(dsize+rsize);
          fBenchmark->Start("Unpacker");
          fFileHandler->CompMemory2Memory(ndigits,digits,comp); 
          fBenchmark->Stop("Unpacker");
          memory->Free();
          if(1)
            cerr<<endl<<dsize/1024<<" "<<rsize/1024<<" "
                                    <<100.*rsize/dsize<<"%"<<endl;
        }
      }

      else{     //Binary to Memory with Roi
        fFileHandler->Free();
        Int_t sli[2]={0,0};
        fFileHandler->SetROI(fEta,sli);
        sprintf(name,"%sdigits_%d_%d.raw",fPath,slice,patch);
        memory->SetBinaryInput(name);
        UInt_t compsize=memory->GetFileSize();
        UInt_t *comp=(UInt_t *)memory->Allocate(compsize);
        memory->CompBinary2CompMemory(ndigits,comp);
        memory->CloseBinaryInput();
        UInt_t datasize=memory->GetMemorySize(ndigits,comp);
        digits=(AliL3DigitRowData *)fFileHandler->Allocate(datasize);
        fBenchmark->Start("Unpacker"); 
        datasize = fFileHandler->CompMemory2Memory(ndigits,digits,comp); 
        fBenchmark->Stop("Unpacker"); 
        memory->Free();
        if(1)
         cerr<<endl<<datasize/1024<<" "<<endl;
      }


      points = (AliL3SpacePointData *) memory->Allocate(pointsize);
  
      fClusterFinder = new AliL3ClustFinder(fTransformer);
      fClusterFinder->InitSlice(slice,patch,row[patch][0],row[patch][1]
                                                               ,maxpoints);
      fClusterFinder->SetXYError(0.1);
      fClusterFinder->SetZError(0.2);
      fClusterFinder->SetOutputArray(points);
      fBenchmark->Start("Read Cluster Finder");
      fClusterFinder->Read(ndigits,digits);
      fBenchmark->Stop("Read Cluster Finder");
      fBenchmark->Start("Cluster Finder");
      fClusterFinder->ProcessDigits();
      fBenchmark->Stop("Cluster Finder");
      npoints = fClusterFinder->GetNumberOfClusters();
      delete fClusterFinder;
      fClusterFinder =0;
      fFileHandler->Free();

    LOG(AliL3Log::kInformational,"AliLevel3::ProcessSlice","Cluster Finder")
      <<AliL3Log::kDec<<"Found "<<npoints<<" Points"<<ENDLOG;
    }

    else{
      if(isdigit){
        sprintf(name,"digits_%d_%d.raw",slice,patch);
   
        if(0){    //Ali to Binary
          fFileHandler->SetBinaryOutput(name);
          fFileHandler->AliDigits2CompBinary();
          fFileHandler->CloseBinaryOutput();
        }
  
        if(1){     //Ali to Memory
          digits=(AliL3DigitRowData *)fFileHandler->AliDigits2Memory(ndigits);
          if(fWriteOut){   //Memory to Binary
            fFileHandler->SetBinaryOutput(name);
            fFileHandler->Memory2CompBinary(ndigits,digits);
            fFileHandler->CloseBinaryOutput();
          }
        }
      }
      else   points = fFileHandler->AliPoints2Memory(npoints);
    }

    if(patch == fNPatch-1){
      // Vertex
      if(fFindVertex){
      // Vertex Finder
      
        fBenchmark->Start("Vertex Finder Read");
        fVertexFinder->Reset();
        fVertexFinder->Read(npoints,points);
        fBenchmark->Stop("Vertex Finder Read"); 
        fBenchmark->Start("Vertex Finder");
        fVertexFinder->Analyze();
        AliL3VertexData vertex[1];
        fVertexFinder->Write(vertex);
        fVertex->Read(vertex);
        fBenchmark->Stop("Vertex Finder"); 
      }
      else{
        //use 0,0,0 for vertex
        fVertex->SetZero();
      }
      fTrackMerger->SetVertex(fVertex);
    }
    fTracker->InitSector(slice,row[patch]);
    fTracker->SetVertex(fVertex);
    fBenchmark->Start("Tracker Read Hits");
    fTracker->ReadHits(npoints,points);
    fBenchmark->Stop("Tracker Read Hits");
    fBenchmark->Start("MainVertexTracking A"); 
    fTracker->MainVertexTracking_a();
    fBenchmark->Stop("MainVertexTracking A");
    fBenchmark->Start("MainVertexTracking B"); 
    fTracker->MainVertexTracking_b();
    fBenchmark->Stop("MainVertexTracking B");
    fBenchmark->Start("Tracking fit");
    fTracker->FillTracks();
    fBenchmark->Stop("Tracking fit");

    if(fWriteOut) 
       WriteSpacePoints(npoints, points, slice, patch); //do after Tracking
    fFileHandler->Free();

    UInt_t ntracks0 =0;
    AliL3TrackSegmentData *trackdata0  = 
         (AliL3TrackSegmentData *) memory->Allocate(fTracker->GetTracks());
    memory->TrackArray2Memory(ntracks0,trackdata0,fTracker->GetTracks());
    
    //write tracks
    if(fWriteOut){
      sprintf(name,"tracks_tr_%d_%d.raw",slice,patch);
      memory->SetBinaryOutput(name);
      memory->Memory2Binary(ntracks0,trackdata0);
      memory->CloseBinaryOutput();
    }
    
    fInterMerger->Reset();
    fInterMerger->SetTransformer(fTransformer);
    fInterMerger->Init(row[patch],patch);

    fBenchmark->Start("Fill Inter Merger");
    fInterMerger->FillTracks(ntracks0,trackdata0);
    fBenchmark->Stop("Fill Inter Merger");
    fBenchmark->Start("Inter Merger");
    fInterMerger->Merge();
//    fInterMerger->SlowMerge();
    
    fBenchmark->Stop("Inter Merger");

    //write inter merged tracks
    if(fWriteOut){
      sprintf(name,"tracks_im_%d_%d.raw",slice,patch);
      WriteTracks(name,fInterMerger,'i'); //write output of intermerger
    }
    memory->Free();
    
    UInt_t ntracks1 =0;
    AliL3TrackSegmentData *trackdata1 =
      (AliL3TrackSegmentData *) memory->Allocate(fInterMerger->GetInTracks(0));
    memory->TrackArray2Memory(ntracks1,trackdata1,fInterMerger->GetInTracks(0));

    fTrackMerger->InitSector(slice,patch);
    fBenchmark->Start("Fill Patch Merger");
    fTrackMerger->FillTracks(ntracks1,trackdata1);
    fBenchmark->Stop("Fill Patch Merger");

    memory->Free();
  }
  fBenchmark->Start("Patch Merger");
//  fTrackMerger->SlowMerge();
  fTrackMerger->Merge();
  fBenchmark->Stop("Patch Merger");
  //write merged tracks
  if(fWriteOut){
    sprintf(name,"tracks_tm_%d.raw",slice);
    WriteTracks(name,fTrackMerger,'o'); //write output of trackmerger
  }
 
  fTrackData = (AliL3TrackSegmentData *) 
                         fFileHandler->Allocate(fTrackMerger->GetOutTracks());

  fFileHandler->TrackArray2Memory(fNTrackData,fTrackData,
                                                fTrackMerger->GetOutTracks());

  delete memory;
}

void AliLevel3::WriteSpacePoints(UInt_t npoints,AliL3SpacePointData *points,
                                                      Int_t slice,Int_t patch){
  char name[256];
  sprintf(name,"points_%d_%d.raw",slice,patch);
  AliL3MemHandler * memory = new AliL3MemHandler();
  memory->SetBinaryOutput(name);
  memory->Transform(npoints,points,slice,fTransformer);
  memory->Memory2Binary(npoints,points);
  memory->CloseBinaryOutput();
  delete  memory;
}


Int_t AliLevel3::WriteTracks(char *filename,AliL3Merger *merger,char opt){
  AliL3FileHandler *memory = new AliL3FileHandler();
  memory->SetBinaryOutput(filename);
  if(opt=='a'||opt=='i'){  //add intracks
    for(Int_t i=0;i<merger->GetNIn();i++){
      AliL3TrackArray *tr=merger->GetInTracks(i);
      memory->TrackArray2Binary(tr);
    }
  }

  if(opt=='o'||opt=='a'){
    AliL3TrackArray *tr=merger->GetOutTracks();
    memory->TrackArray2Binary(tr);
  }

  memory->CloseBinaryOutput();
 
  return 1;
}

void AliLevel3::WriteResults()
{
  //Write the resulting tracks to outputfile
  WriteTracks("tracks.raw",fGlobalMerger,'a');
}
