//-------------------------------------------------------------------------
//    Description: 
//    This class include into LRC library for Long-Range Correlation analysis
//    it is the PtN class
//    calculates PtN correlations for abs and rel var
//    Origin: Petr Naumenko, SPbSU-CERN, Petr.Naoumenko@cern.ch
//-------------------------------------------------------------------------

#ifndef ALILRCPTN_H
#define ALILRCPTN_H
/*  See cxx source for full Copyright notice */


/* $Id$ */

//-------------------------------------------------------------------------
//         LRC library for Long-Range Correlation analysis
//
//    Origin: Petr Naumenko, SPbSU-CERN, Petr.Naoumenko@cern.ch
//-------------------------------------------------------------------------

#include "AliLRCAnalysis.h"

class TProfile;
class TList;
class TH2D;


class AliLRCPtN : public AliLRCAnalysis {
    public:
	AliLRCPtN();
	~AliLRCPtN();
	AliLRCPtN(char *name, TH2D* sourceHist, double ptd, TProfile* nb);
	AliLRCPtN(char *name, TH2D* sourceHist, double ptd, TH2D* nb);
	AliLRCPtN(char *fileHistname, char *histname, char *profname, double ptd, char *errhistname);
	AliLRCPtN(TList *LHist, char *histname, char *profname, char *ptdname, char *errhistname);	
	void MakeHistogramm(char *name, TH2D* sourceHist, double ptd, TH2D* nb);
	void MakeHistogramm(char *name, TH2D* sourceHist, double ptd, TProfile* nb);
	void MakeHistogramm(char *fileHistname, char *histname, char *profname, double ptd, char *errhistname);	
	void MakeHistogramm(TList *LHist, char *histname, char *profname, char *ptdname, char *errhistname);	
	ClassDef(AliLRCPtN,0)                 // macro for rootcint
};



#endif

