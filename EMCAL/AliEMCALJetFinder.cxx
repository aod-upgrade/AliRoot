
/**************************************************************************
 * Copyright(c) 1998-1999, ALICE Experiment at CERN, All rights reserved. *
 *                                                                        *
 * Author: The ALICE Off-line Project.                                    *
 * Contributors are mentioned in the code where appropriate.              *
 *                                                                        *
 * Permission to use, copy, modify and distribute this software and its   *
 * documentation strictly for non-commercial purposes is hereby granted   *
 * without fee, provided that the above copyright notice appears in all   *
 * copies and that both the copyright notice and this permission notice   *
 * appear in the supporting documentation. The authors make no claims     *
 * about the suitability of this software for any purpose. It is          *
 * provided "as is" without express or implied warranty.                  *
 **************************************************************************/

/*
$Log$
Revision 1.7  2002/01/21 15:47:18  morsch
Bending radius correctly in cm.

Revision 1.6  2002/01/21 12:53:50  morsch
authors

Revision 1.5  2002/01/21 12:47:47  morsch
Possibility to include K0long and neutrons.

Revision 1.4  2002/01/21 11:03:21  morsch
Phi propagation introduced in FillFromTracks.

Revision 1.3  2002/01/18 05:07:56  morsch
- hadronic correction
- filling of digits
- track selection upon EMCAL information

*/

//*-- Authors: Andreas Morsch   (CERN)
//*            J.L. Klay        (LBL)
//*            Aleksei Pavlinov (WSU) 

// From root ...
#include <TClonesArray.h>
#include <TTree.h>
#include <TBranchElement.h>
#include <TFile.h>
#include <TH2.h>
#include <TAxis.h>
#include <TParticle.h>
#include <TParticlePDG.h>

// From AliRoot ...
#include "AliEMCALJetFinder.h"
#include "AliEMCALFast.h"
#include "AliEMCALGeometry.h"
#include "AliEMCALHit.h"
#include "AliEMCALDigit.h"
#include "AliEMCALDigitizer.h"
#include "AliEMCALHadronCorrection.h"
#include "AliRun.h"
#include "AliMagF.h"
#include "AliMagFCM.h"
#include "AliEMCAL.h"
#include "AliHeader.h"
#include "AliPDG.h"

// Interface to FORTRAN
#include "Ecommon.h"


ClassImp(AliEMCALJetFinder)

//____________________________________________________________________________
AliEMCALJetFinder::AliEMCALJetFinder()
{
// Default constructor
    fJets             = 0;
    fNjets            = 0;
    fLego             = 0;
    fLegoB            = 0;
    fTrackList        = 0;
    fPtT              = 0;
    fEtaT             = 0;
    fPhiT             = 0;
    fPtB              = 0;
    fEtaB             = 0;
    fPhiB             = 0;
    fHadronCorrector  = 0;
}

AliEMCALJetFinder::AliEMCALJetFinder(const char* name, const char *title)
    : TTask(name, title)
{
// Constructor 
    fJets  = new TClonesArray("AliEMCALJet",10000);
    fNjets = 0;
    for (Int_t i = 0; i < 30000; i++)
    {
	fEtCell[i]  = 0.;
	fEtaCell[i] = 0.;
        fPhiCell[i] = 0.;
    }
    fLego = 0;
    fTrackList  = 0;
    fPtT        = 0;
    fEtaT       = 0;
    fPhiT       = 0;
    fPtB        = 0;
    fEtaB       = 0;
    fPhiB       = 0;
    fHadronCorrector = 0;
    fBackground = 0;
//
    SetPtCut();
    SetMomentumSmearing();
    SetEfficiencySim();
    SetDebug();
    SetHadronCorrection();
    SetSamplingFraction();
    SetIncludeK0andN();
}




//____________________________________________________________________________
AliEMCALJetFinder::~AliEMCALJetFinder()
{
// Destructor
//
    if (fJets){
	fJets->Delete();
	delete fJets;
    }
}




#ifndef WIN32
# define jet_finder_ua1 jet_finder_ua1_
# define hf1 hf1_
# define type_of_call

#else
# define jet_finder_ua1 J
# define hf1 HF1
# define type_of_call _stdcall
#endif

extern "C" void type_of_call 
jet_finder_ua1(Int_t& ncell, Int_t& ncell_tot,
	       Float_t  etc[30000],  Float_t etac[30000],
	       Float_t  phic[30000], 
	       Float_t& min_move, Float_t& max_move, Int_t& mode, 
	       Float_t& prec_bg,  Int_t& ierror);

extern "C" void type_of_call hf1(Int_t& id, Float_t& x, Float_t& wgt);





void AliEMCALJetFinder::Find(Int_t ncell, Int_t ncell_tot, Float_t etc[30000], 
			     Float_t etac[30000], Float_t phic[30000],
			     Float_t min_move, Float_t max_move, Int_t mode, 
			     Float_t prec_bg,  Int_t   ierror)
{
// Wrapper for fortran coded jet finder
// Full argument list
    jet_finder_ua1(ncell, ncell_tot, etc, etac, phic, 
		   min_move, max_move, mode, prec_bg, ierror);
    // Write result to output
    WriteJets();
}

void AliEMCALJetFinder::Find()
{
// Wrapper for fortran coded jet finder using member data for 
// argument list

    Float_t min_move = 0;
    Float_t max_move = 0;
    Int_t   mode     = 0;
    Float_t prec_bg  = 0.;
    Int_t   ierror   = 0;

    jet_finder_ua1(fNcell, fNtot, fEtCell, fEtaCell, fPhiCell, 
		   min_move, max_move, mode, prec_bg, ierror);
    // Write result to output
    Int_t njet = Njets();
    
    for (Int_t nj=0; nj<njet; nj++)
    {

	
	fJetT[nj] = new AliEMCALJet(JetEnergy(nj),
				    JetPhiW(nj),
				    JetEtaW(nj));
    }
    FindTracksInJetCone();
    WriteJets();
}


Int_t AliEMCALJetFinder::Njets()
{
// Get number of reconstructed jets
    return EMCALJETS.njet;
}

Float_t AliEMCALJetFinder::JetEnergy(Int_t i)
{
// Get reconstructed jet energy
    return EMCALJETS.etj[i];
}

Float_t AliEMCALJetFinder::JetPhiL(Int_t i)
{
// Get reconstructed jet phi from leading particle
    return EMCALJETS.phij[0][i];
}

Float_t AliEMCALJetFinder::JetPhiW(Int_t i)
{
// Get reconstructed jet phi from weighting
    return EMCALJETS.phij[1][i];
}

Float_t  AliEMCALJetFinder::JetEtaL(Int_t i)
{
// Get reconstructed jet eta from leading particles
    return EMCALJETS.etaj[0][i];
}


Float_t  AliEMCALJetFinder::JetEtaW(Int_t i)  
{
// Get reconstructed jet eta from weighting
    return EMCALJETS.etaj[1][i];
}

void AliEMCALJetFinder::SetCellSize(Float_t eta, Float_t phi)
{
// Set grid cell size
    EMCALCELLGEO.etaCellSize = eta;
    EMCALCELLGEO.phiCellSize = phi;    
    fDeta = eta;
    fDphi = phi;
}

void AliEMCALJetFinder::SetConeRadius(Float_t par)
{
// Set jet cone radius
    EMCALJETPARAM.coneRad = par;
    fConeRadius = par;
}

void AliEMCALJetFinder::SetEtSeed(Float_t par)
{
// Set et cut for seeds
    EMCALJETPARAM.etSeed = par;
    fEtSeed = par;
}

void AliEMCALJetFinder::SetMinJetEt(Float_t par)
{
// Set minimum jet et
    EMCALJETPARAM.ejMin = par;
    fMinJetEt = par;
}

void AliEMCALJetFinder::SetMinCellEt(Float_t par)
{
// Set et cut per cell
    EMCALJETPARAM.etMin = par;
    fMinCellEt = par;
}

void AliEMCALJetFinder::SetPtCut(Float_t par)
{
// Set pT cut on charged tracks
    fPtCut = par;
}


void AliEMCALJetFinder::Test()
{
//
// Test the finder call
//
    const Int_t nmax = 30000;
    Int_t ncell      = 10;
    Int_t ncell_tot  = 100;

    Float_t etc[nmax];
    Float_t etac[nmax];
    Float_t phic[nmax];
    Float_t min_move = 0;
    Float_t max_move = 0;
    Int_t   mode = 0;
    Float_t prec_bg = 0;
    Int_t   ierror = 0;

    
    Find(ncell, ncell_tot, etc, etac, phic, 
	 min_move, max_move, mode, prec_bg, ierror);

}

//
//  I/O
//	

void AliEMCALJetFinder::AddJet(const AliEMCALJet& jet)
{
    //
    // Add a jet 
    //
    TClonesArray &lrawcl = *fJets;
    new(lrawcl[fNjets++]) AliEMCALJet(jet);
}

void AliEMCALJetFinder::ResetJets()
{
    //
    // Reset Jet List 
    //
    fJets->Clear();
    fNjets = 0;
}

void AliEMCALJetFinder::WriteJets()
{
//
// Add all jets to the list
//
    const Int_t kBufferSize = 4000;
    TTree *pK = gAlice->TreeK();
    const char* file = (pK->GetCurrentFile())->GetName();
// I/O
    AliEMCAL* pEMCAL = (AliEMCAL* )gAlice->GetModule("EMCAL");
 
    if (fDebug > 1)
    printf("Make Branch - TreeR address %p %p\n",gAlice->TreeR(), pEMCAL);

    if (fJets && gAlice->TreeR()) {
	pEMCAL->MakeBranchInTree(gAlice->TreeR(), 
				 "EMCALJets", 
				 &fJets, 
				 kBufferSize, 
				 file);
    }
    Int_t njet = Njets();
    for (Int_t nj = 0; nj < njet; nj++)
    {
	AddJet(*fJetT[nj]);
	delete fJetT[nj];
    }

    Int_t nev = gAlice->GetHeader()->GetEvent();
    gAlice->TreeR()->Fill();
    char hname[30];
    sprintf(hname,"TreeR%d", nev);
    gAlice->TreeR()->Write(hname);
    gAlice->TreeR()->Reset();
    ResetJets();        
}

void AliEMCALJetFinder::BookLego()
{
//
//  Book histo for discretisation
//
//
//  Get geometry parameters from 
    AliEMCAL* pEMCAL = (AliEMCAL*) gAlice->GetModule("EMCAL");
    AliEMCALGeometry* geom = 
	AliEMCALGeometry::GetInstance(pEMCAL->GetTitle(), "");
    fNbinEta = geom->GetNZ();
    fNbinPhi = geom->GetNPhi();
    const Float_t  phiMin  = geom->GetArm1PhiMin()*TMath::Pi()/180.;
    const Float_t  phiMax  = geom->GetArm1PhiMax()*TMath::Pi()/180.;
    fDphi   = (phiMax-phiMin)/fNbinEta;
    fDeta   = 1.4/fNbinEta;
    fNtot   = fNbinPhi*fNbinEta;
    
//    
    fLego = new TH2F("legoH","eta-phi",
			   fNbinEta, -0.7,  0.7, 
			   fNbinPhi, phiMin, phiMax);
}

void AliEMCALJetFinder::DumpLego()
{
//
// Dump lego histo into array
//
    fNcell = 0;
    for (Int_t i = 1; i < fNbinEta; i++) {
	for (Int_t j = 1; j < fNbinPhi; j++) {
	    Float_t e    = fLego->GetBinContent(i,j);
	    TAxis* Xaxis = fLego->GetXaxis();
	    TAxis* Yaxis = fLego->GetYaxis();
	    Float_t eta  = Xaxis->GetBinCenter(i);
	    Float_t phi  = Yaxis->GetBinCenter(j);	    
	    fEtCell[fNcell]  = e;
	    fEtaCell[fNcell] = eta;
	    fPhiCell[fNcell] = phi;
	    fNcell++;
	} // phi
    } // eta
    fNcell--;
}

void AliEMCALJetFinder::ResetMap()
{
//
// Reset eta-phi array

    for (Int_t i=0; i<30000; i++)
    {
	fEtCell[i]  = 0.;
	fEtaCell[i] = 0.;
	fPhiCell[i] = 0.;
    }
}


void AliEMCALJetFinder::FillFromTracks(Int_t flag, Int_t ich)
{
//
// Fill Cells with track information
//
//
    ResetMap();
    
    if (!fLego) BookLego();
// Reset
    if (flag == 0) fLego->Reset();
//
// Access particle information    
    Int_t npart = (gAlice->GetHeader())->GetNprimary();
// Create track list
//
// 0: not selected
// 1: selected for jet finding
// 2: inside jet
// ....
    if (fTrackList) delete[] fTrackList;
    if (fPtT)       delete[] fPtT;
    if (fEtaT)      delete[] fEtaT;
    if (fPhiT)      delete[] fPhiT;
    
    fTrackList = new Int_t  [npart];
    fPtT       = new Float_t[npart];
    fEtaT      = new Float_t[npart];
    fPhiT      = new Float_t[npart];

    fNt        = npart;
    fNtS  = 0;

    for (Int_t part = 2; part < npart; part++) {
	TParticle *MPart = gAlice->Particle(part);
	Int_t mpart   = MPart->GetPdgCode();
	Int_t child1  = MPart->GetFirstDaughter();
	Float_t pT    = MPart->Pt();
	Float_t p     = MPart->P();
	Float_t phi   = MPart->Phi();
	Float_t eta   = MPart->Eta();

	if (fDebug > 0) {
	    if (part == 6 || part == 7)
	    {
		printf("\n Simulated Jet (pt, eta, phi): %d %f %f %f", 
		       part-5, pT, eta, phi);
	    }

//	    if (mpart == 21)
		
//		printf("\n Simulated Jet (pt, eta, phi): %d %d %f %f %f",
//		       part, mpart, pT, eta, phi); 
	}
	
	
	fTrackList[part] = 0;
	fPtT[part]       = pT;
	fEtaT[part]      = eta;
	fPhiT[part]      = phi;

	if (part < 2) continue;
	if (pT == 0 || pT < fPtCut) continue;
	TParticlePDG* pdgP = 0;
// charged or neutral 
	if (ich == 0) {
	    pdgP = MPart->GetPDG();
	    if (pdgP->Charge() == 0) {
		if (!fK0N) {
		    continue;
		} else {
		    if (mpart != kNeutron    &&
			mpart != kNeutronBar &&
			mpart != kK0Long) continue;
		}
	    }
	} 
// skip partons
	if (TMath::Abs(mpart) <= 6         ||
	    mpart == 21                    ||
	    mpart == 92) continue;
// acceptance cut
	if (TMath::Abs(eta) > 0.7)         continue;
	if (phi*180./TMath::Pi() > 120.)   continue;
// final state only
	if (child1 >= 0 && child1 < npart) continue;
//
	if (fDebug > 1) 
	printf("\n sel:%5d %5d %5d %8.2f %8.2f %8.2f",
	part, mpart, child1, eta, phi, pT);
//
//
// phi propagation for hadronic correction

	Bool_t curls = kFALSE;
	Float_t dphi = PropagatePhi(pT, pdgP->Charge(), curls);
	if (fDebug) printf("\n Delta phi %f pT%f", dphi, pT);
	if (curls && fDebug) printf("\n Track is curling %f", pT);
	Float_t phiHC = phi + dphi;
//
// Momentum smearing goes here ...
//
	if (fSmear) {
	    p = AliEMCALFast::SmearMomentum(1,p);
	}
//
// Tracking Efficiency and TPC acceptance goes here ...
	if (fEffic) {
	    Float_t eff =  AliEMCALFast::Efficiency(1,p);
	    if (AliEMCALFast::RandomReject(eff)) continue;
	}
//
// Correction of Hadronic Energy goes here ...
//
	Float_t dpH = 0.;
	
	if (fHCorrection) {
	    if (!fHadronCorrector)
		Fatal("AliEMCALJetFinder",
		    "Hadronic energy correction required but not defined !");
	    dpH = fHadronCorrector->GetEnergy(p, eta, 7);
	}
//
//  More to do ? Just think about it !
//
	fTrackList[part] = 1;
//
	fNtS++;
	
	fLego->Fill(eta, phi, p);
	if (fHCorrection) fLego->Fill(eta, phiHC, dpH);
    } // primary loop
    DumpLego();
}

void AliEMCALJetFinder::FillFromHits(Int_t flag)
{
//
// Fill Cells with hit information
//
//
    ResetMap();
    
    if (!fLego) BookLego();
//  Reset eta-phi map if needed
    if (flag == 0)    fLego->Reset();
//  Initialize from background event if available
    if (fBackground)  InitFromBackground();
    

//
// Access hit information    
    AliEMCAL* pEMCAL = (AliEMCAL*) gAlice->GetModule("EMCAL");
    
    TTree *treeH = gAlice->TreeH();
    Int_t ntracks = (Int_t) treeH->GetEntries();
//
//   Loop over tracks
//
    Int_t nbytes = 0;

    
    for (Int_t track=0; track<ntracks;track++) {
	gAlice->ResetHits();
	nbytes += treeH->GetEvent(track);
//
//  Loop over hits
//
	for(AliEMCALHit* mHit=(AliEMCALHit*) pEMCAL->FirstHit(-1); 
	    mHit;
	    mHit=(AliEMCALHit*) pEMCAL->NextHit()) 
	{
	    Float_t x      =    mHit->X();         // x-pos of hit
	    Float_t y      =    mHit->Y();         // y-pos
	    Float_t z      =    mHit->Z();         // z-pos
	    Float_t eloss  =    mHit->GetEnergy(); // deposited energy

	    Float_t r      =    TMath::Sqrt(x*x+y*y);
	    Float_t theta  =    TMath::ATan2(r,z);
	    Float_t eta    =   -TMath::Log(TMath::Tan(theta/2.));
	    Float_t phi    =    TMath::ATan2(y,x);
	    fLego->Fill(eta, phi, fSamplingF*eloss);
	} // Hit Loop
    } // Track Loop
    DumpLego();
}

void AliEMCALJetFinder::FillFromDigits(Int_t flag)
{
//
// Fill Cells with digit information
//
//
    ResetMap();
    
    if (!fLego) BookLego();
    if (flag == 0) fLego->Reset();
    Int_t nbytes;
    

//
//  Connect digits    
//
    TClonesArray* digs      = new TClonesArray("AliEMCALDigit", 10000);
    TTree *treeD = gAlice->TreeD();
    TBranchElement* branchDg = (TBranchElement*)
	treeD->GetBranch("EMCAL");

    if (!branchDg) Fatal("AliEMCALJetFinder", 
			 "Reading digits requested but no digits in file !");
    
    branchDg->SetAddress(&digs);
    Int_t nent = (Int_t) branchDg->GetEntries();
//
//  Connect digitizer
//
    AliEMCALDigitizer* digr = new AliEMCALDigitizer();
    TBranchElement* branchDr = (TBranchElement*) 
	treeD->GetBranch("AliEMCALDigitizer");
    branchDr->SetAddress(&digr);
//
//
    nbytes += branchDg->GetEntry(0);
    nbytes += branchDr->GetEntry(0);
//
//  Get digitizer parameters
    Float_t towerADCped  = digr->GetTowerpedestal();
    Float_t towerADCcha  = digr->GetTowerchannel();
    Float_t preshoADCped = digr->GetPreShopedestal();
    Float_t preshoADCcha = digr->GetPreShochannel();

    AliEMCAL* pEMCAL = (AliEMCAL*) gAlice->GetModule("EMCAL");
    AliEMCALGeometry* geom = 
	AliEMCALGeometry::GetInstance(pEMCAL->GetTitle(), "");
    
    if (fDebug) {
	Int_t   ndig = digs->GetEntries();
	printf("\n Number of Digits: %d %d\n", ndig, nent);
	printf("\n Parameters: %f %f %f %f\n", 
	       towerADCped, towerADCcha, preshoADCped, preshoADCcha );
	printf("\n Geometry: %d %d\n", geom->GetNEta(), geom->GetNPhi());
    }
    
//
//  Loop over digits
    AliEMCALDigit* sdg;
    TIter next(digs);
    while ((sdg = (AliEMCALDigit*)(next())))
    {
	Double_t pedestal;
	Double_t channel;
	if (sdg->GetId() > (geom->GetNZ() * geom->GetNPhi())) 
	{
	    pedestal = preshoADCped;
	    channel  = preshoADCcha; 
	} else {
	    pedestal = towerADCped;
	    channel  = towerADCcha; 
	}
	
	Float_t eta = sdg->GetEta();
	Float_t phi = sdg->GetPhi() * TMath::Pi()/180.;
	Float_t amp = (Float_t) (channel*(sdg->GetAmp())-pedestal);
	
	if (fDebug > 1) printf("\n Digit: eta %8.3f phi %8.3f amp %8.3f %8d",
			   eta, phi, amp, sdg->GetAmp());
	
	fLego->Fill(eta, phi, fSamplingF*amp);
    } // digit loop
// 
//  Dump lego hist
    DumpLego();
}


void AliEMCALJetFinder::FillFromHitFlaggedTracks(Int_t flag)
{
//
// Fill Cells with hit information
//
//
    ResetMap();
    
    if (!fLego) BookLego();
// Reset
    if (flag == 0) fLego->Reset();

// Flag charged tracks passing through TPC which 
// are associated to EMCAL Hits
    BuildTrackFlagTable();

//
// Access particle information    
    TTree *treeH = gAlice->TreeH();
    Int_t ntracks = (Int_t) treeH->GetEntries();

    if (fPtT)       delete[] fPtT;   
    if (fEtaT)      delete[] fEtaT;    
    if (fPhiT)      delete[] fPhiT;   
   
    fPtT       = new Float_t[ntracks];
    fEtaT      = new Float_t[ntracks];
    fPhiT      = new Float_t[ntracks];

    fNt   = ntracks;
    fNtS  = 0;
    
    for (Int_t track = 0; track < ntracks; track++) {
	  TParticle *MPart = gAlice->Particle(track);
	  Float_t pT    = MPart->Pt();
	  Float_t phi   = MPart->Phi();
	  Float_t eta   = MPart->Eta();

	if(fTrackList[track]) {
	  fPtT[track]       = pT;
	  fEtaT[track]      = eta;
	  fPhiT[track]      = phi;

	  if (track < 2) continue;	//Colliding particles?
	  if (pT == 0 || pT < fPtCut) continue;
	  fNtS++;
	  fLego->Fill(eta, phi, pT);
	}
      } // track loop
    DumpLego();
}

void AliEMCALJetFinder::BuildTrackFlagTable() {

// Method to generate a lookup table for TreeK particles
// which are linked to hits in the EMCAL
//
// --Author: J.L. Klay
//
    printf("\n Building track flag table.\n");
    
    
// Access hit information    
    AliEMCAL* pEMCAL = (AliEMCAL*) gAlice->GetModule("EMCAL");
    
    TTree *TK = gAlice->TreeK();		// Get the number of entries in the kine tree
    Int_t nKTrks = (Int_t) TK->GetEntries();	// (Number of particles created somewhere)
    
    if(fTrackList) delete[] fTrackList;		//Make sure we get rid of the old one
    fTrackList = new Int_t[nKTrks];		//before generating a new one
    
    for (Int_t i = 0; i < nKTrks; i++) {	//Initialize members to 0
	fTrackList[i] = 0;
    }
    
    TTree *treeH = gAlice->TreeH();
    Int_t ntracks = (Int_t) treeH->GetEntries();
//
//   Loop over tracks
//
    Int_t nbytes = 0;
    
    for (Int_t track=0; track<ntracks;track++) {
	gAlice->ResetHits();
	nbytes += treeH->GetEvent(track);
        if (pEMCAL)  {
	    
//
//  Loop over hits
//
	    for(AliEMCALHit* mHit=(AliEMCALHit*) pEMCAL->FirstHit(-1); 
		mHit;
		mHit=(AliEMCALHit*) pEMCAL->NextHit()) 
	    {
		Int_t   iTrk   =    mHit->Track();       // track number
		Int_t   idprim =    mHit->GetPrimary();  // primary particle
		
		//Determine the origin point of this particle - it made a hit in the EMCAL
		TParticle 	  *trkPart = gAlice->Particle(iTrk);
		TParticlePDG  *trkPdg  = trkPart->GetPDG();
		Int_t	   trkCode = trkPart->GetPdgCode();
		Double_t 	   trkChg;
		if (trkCode < 10000) {		//Big Ions cause problems for 
		    trkChg  = trkPdg->Charge();	//this function.  Since they aren't
		} else {				//likely to make it very far, set
		    trkChg  = 0.0;			//their charge to 0 for the Flag test
		}
		Float_t 	   vxTrk   = trkPart->Vx();
		Float_t 	   vyTrk   = trkPart->Vy();
		Float_t 	   vrTrk   = TMath::Sqrt(vxTrk*vxTrk+vyTrk*vyTrk); 
		fTrackList[iTrk] = SetTrackFlag(vrTrk,trkCode,trkChg);
		
		//Loop through the ancestry of the EMCAL entrance particles
		Int_t ancestor = trkPart->GetFirstMother();  //Get track's Mother
		while (ancestor != -1) {
		    TParticle     *ancPart = gAlice->Particle(ancestor);  //get particle info on ancestor
		    TParticlePDG  *ancPdg  = ancPart->GetPDG();
		    Int_t	      ancCode = ancPart->GetPdgCode();
		    Double_t       ancChg;
		    if (ancCode < 10000) {
			ancChg  = ancPdg->Charge();
		    } else {
			ancChg  = 0.0;
		    }
		    Float_t 	      vxAnc   = ancPart->Vx();
		    Float_t 	      vyAnc   = ancPart->Vy();
		    Float_t 	      vrAnc   = TMath::Sqrt(vxAnc*vxAnc+vyAnc*vyAnc);
		    fTrackList[ancestor] = SetTrackFlag(vrAnc,ancCode,ancChg);
		    ancestor = ancPart->GetFirstMother();           //Get the next ancestor
		}
		
		//Determine the origin point of the primary particle associated with the hit
		TParticle     *primPart = gAlice->Particle(idprim);
		TParticlePDG  *primPdg  = primPart->GetPDG();
		Int_t	   primCode = primPart->GetPdgCode();
		Double_t 	   primChg;
		if (primCode < 10000) {
		    primChg  = primPdg->Charge();
		} else {
		    primChg  = 0.0;
		}
		Float_t	   vxPrim = primPart->Vx();
		Float_t	   vyPrim = primPart->Vy();
		Float_t	   vrPrim = TMath::Sqrt(vxPrim*vxPrim+vyPrim*vyPrim);
		fTrackList[idprim] = SetTrackFlag(vrPrim,primCode,primChg);
	    } // Hit Loop
	} //if (pEMCAL)
    } // Track Loop
}

Int_t AliEMCALJetFinder
::SetTrackFlag(Float_t radius, Int_t code, Double_t charge) {

    Int_t flag    = 0; 
    Int_t parton  = 0; 
    Int_t neutral = 0;
    
    if (charge == 0) neutral = 1;
    
    if (TMath::Abs(code) <= 6   ||	
	code == 21              ||   
	code == 92) parton = 1;
    
    //It's not a parton, it's charged and it went through the TPC
    if (!parton && !neutral && radius <= 84.0) flag = 1;
    
    return flag;
}



void AliEMCALJetFinder::SaveBackgroundEvent()
{
// Saves the eta-phi lego and the tracklist
//
    if (fLegoB) delete fLegoB;
    
    fLegoB = new TH2F(*fLego);
    
    if (fPtB)        delete[] fPtB;   
    if (fEtaB)       delete[] fEtaB;    
    if (fPhiB)       delete[] fPhiB;   
    if (fTrackListB) delete[] fTrackListB;   
   
    fPtB          = new Float_t[fNtS];
    fEtaB         = new Float_t[fNtS];
    fPhiB         = new Float_t[fNtS];
    fTrackListB   = new Int_t  [fNtS];

    for (Int_t i = 0; i < fNt; i++) {
	if (!fTrackList[i]) continue;
	fPtB [fNtB]      = fPtT [i];
	fEtaB[fNtB]      = fEtaT[i];
	fPhiB[fNtB]      = fPhiT[i];
	fTrackList[fNtB] = 1;
	fNtB++;
    }
}

void AliEMCALJetFinder::InitFromBackground()
{
//
//    
    if (fLego) delete fLego;
    fLego = new TH2F(*fLegoB);
}


    
void AliEMCALJetFinder::FindTracksInJetCone()
{
//
//  Build list of tracks inside jet cone
//
//  Loop over jets
    Int_t njet = Njets();
    for (Int_t nj = 0; nj < njet; nj++)
    {
	Float_t etaj = JetEtaW(nj);
	Float_t phij = JetPhiW(nj);	
	Int_t   nT   = 0;           // number of associated tracks
	
// Loop over particles in current event 
	for (Int_t part = 0; part < fNt; part++) {
	    if (!fTrackList[part]) continue;
	    Float_t phi      = fPhiT[part];
	    Float_t eta      = fEtaT[part];
	    Float_t dr       = TMath::Sqrt((etaj-eta)*(etaj-eta) +
					   (phij-phi)*(phij-phi));
	    if (dr < fConeRadius) {
		fTrackList[part] = nj+2;
		nT++;
	    } // < ConeRadius ?
	} // particle loop
	
// Same for background event if available
	Int_t nTB = 0;
	if (fBackground) {
	    for (Int_t part = 0; part < fNtB; part++) {
		Float_t phi      = fPhiB[part];
		Float_t eta      = fEtaB[part];
		Float_t dr       = TMath::Sqrt((etaj-eta)*(etaj-eta) +
					       (phij-phi)*(phij-phi));
		if (dr < fConeRadius) {
		    fTrackList[part] = nj+2;
		    nTB++;
		} // < ConeRadius ?
	    } // particle loop
	} // Background available ?
	
	Int_t nT0 = nT + nTB;
	
	if (nT0 > 50) nT0 = 50;
	
	Float_t* ptT  = new Float_t[nT];
	Float_t* etaT = new Float_t[nT];
	Float_t* phiT = new Float_t[nT];
	Int_t iT = 0;
	Int_t j;
	
	for (Int_t part = 0; part < fNt; part++) {
	    if (fTrackList[part] == nj+2) {
		Int_t index = 0;
		for (j=iT-1; j>=0; j--) {
		    if (fPtT[part] > ptT[j]) {
			index = j+1;
			break;
		    }
		}
		for (j=iT-1; j>=index; j--) {
		    ptT [j+1]  = ptT [j];
		    etaT[j+1]  = etaT[j];
		    phiT[j+1]  = phiT[j];
		}
		ptT [index] = fPtT [part];
		etaT[index] = fEtaT[part];
		phiT[index] = fPhiT[part];
		iT++;
	    } // particle associated
	    if (iT > nT0) break;
	} // particle loop
	
	if (fBackground) {
	    for (Int_t part = 0; part < fNtB; part++) {
		if (fTrackList[part] == nj+2) {
		    Int_t index = 0;
		    for (j=iT-1; j>=0; j--) {
			if (fPtB[part] > ptT[j]) {
			    index = j+1;
			    break;
			}
		    }
		    for (j=iT-1; j>=index; j--) {
			ptT [j+1]  = ptT [j];
			etaT[j+1]  = etaT[j];
			phiT[j+1]  = phiT[j];
		    }
		    ptT [index] = fPtB [part];
		    etaT[index] = fEtaB[part];
		    phiT[index] = fPhiB[part];
		    iT++;
		} // particle associated
		if (iT > nT0) break;
	    } // particle loop
	} // Background available ?

	fJetT[nj]->SetTrackList(nT, ptT, etaT, phiT);
	delete[] ptT;
	delete[] etaT;
	delete[] phiT;
    } // jet loop loop
}

Float_t AliEMCALJetFinder::PropagatePhi(Float_t pt, Float_t charge, Bool_t& curls)
{
// Propagates phi angle to EMCAL radius
//
    Float_t dPhi = 0.;
// Get field
    Float_t b =  ((AliMagFCM*) gAlice->Field())->SolenoidField();
// Get EMCAL radius 
    Float_t rEMCAL = AliEMCALGeometry::GetInstance()->GetIPDistance();
//
//
// bending radies
    Float_t rB = 333.56 * pt / b;  // [cm]
//
// check if particle is curling below EMCAL
    if (2.*rB < rEMCAL) {
	curls = kTRUE;
	return dPhi;
    }
//
// if not calculate delta phi
    Float_t phi = TMath::ACos(1.-rEMCAL*rEMCAL/(2.*rB*rB));
    dPhi = TMath::ATan2(1.-TMath::Cos(phi), TMath::Sin(phi));
    dPhi = TMath::Sign(dPhi, charge);
//    
    return dPhi;
    
}


void hf1(Int_t& id, Float_t& x, Float_t& wgt)
{
// dummy for hbook calls
    ;
}








