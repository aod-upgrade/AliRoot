// @(#)root/eve:$Id$
// Author: Matevz Tadel 2007

/**************************************************************************
 * Copyright(c) 1998-2008, ALICE Experiment at CERN, all rights reserved. *
 * See http://aliceinfo.cern.ch/Offline/AliRoot/License.html for          *
 * full copyright notice.                                                 *
 **************************************************************************/

#include "AliEveV0Editor.h"
#include "AliEveV0.h"

#include "TVirtualPad.h"
#include "TColor.h"

// Cleanup these includes:
#include "TGLabel.h"
#include "TGButton.h"
#include "TGNumberEntry.h"
#include "TGColorSelect.h"
#include "TGDoubleSlider.h"


//______________________________________________________________________________
// GUI editor for AliEveV0.
//

ClassImp(AliEveV0Editor)

//______________________________________________________________________________
AliEveV0Editor::AliEveV0Editor(const TGWindow *p, Int_t width, Int_t height,
                               UInt_t options, Pixel_t back) :
  TGedFrame(p, width, height, options | kVerticalFrame, back),
  fM(0),
  fInfoLabel0(0),
  fInfoLabel1(0),
  fXButton(0)
  // Initialize widget pointers to 0
{
  // Constructor.

  MakeTitle("AliEveV0");

  fInfoLabel0 = new TGLabel(this);
  fInfoLabel0->SetTextJustify(kTextLeft);
  AddFrame(fInfoLabel0, new TGLayoutHints(kLHintsLeft|kLHintsExpandX,
                                          8, 0, 2, 0));

  fInfoLabel1 = new TGLabel(this);
  fInfoLabel1->SetTextJustify(kTextLeft);
  AddFrame(fInfoLabel1, new TGLayoutHints(kLHintsLeft|kLHintsExpandX,
                                          8, 0, 2, 0));

  fXButton = new TGTextButton(this, "Detailed View");
  AddFrame(fXButton, new TGLayoutHints(kLHintsLeft|kLHintsExpandX, 1, 1, 0, 0));
  fXButton->Connect("Clicked()", "AliEveV0Editor", this, "DisplayDetailed()");
}

/******************************************************************************/

//______________________________________________________________________________
void AliEveV0Editor::SetModel(TObject* obj)
{
  // Set model object.

  fM = dynamic_cast<AliEveV0*>(obj);

  // Set values of widgets
  fInfoLabel0->SetText(Form("DCA = %f, Radius = %f", fM->GetDaughterDCA(), fM->GetRadius()));
  fInfoLabel1->SetText(Form("Some other relevant stuff."));
}

/******************************************************************************/

// Implements callback/slot methods

//______________________________________________________________________________
// void AliEveV0Editor::DoXYZZ()
// {
//    // Slot for XYZZ.
//
//    fM->SetXYZZ(fXYZZ->GetValue());
//    Update();
// }

void AliEveV0Editor::DisplayDetailed()
{
  printf("Hura!\n");
}
