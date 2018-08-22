#include <iostream>
#include "GlobalActorReader.hh"
#include <stdexcept>
#include "TCanvas.h"
#include "TH1F.h"
#include <string>
#include <TRandom3.h>
#include <TMath.h>
#include <algorithm>
#include <TGraph.h>
#include <TLegend.h>
#include <TFile.h>
#include <cmath>
#include <TLorentzVector.h>
#include <cassert>

using namespace std;

struct FullEvent {
  FullEvent()
  {
    reset();
  }

  int fEventID = -1;
  TLorentzVector gammaPrompt;
  TLorentzVector gamma1;
  TLorentzVector gamma2;
  //bool gammaPromptScatteredInWater = false;
  //bool gamma1ScatteredInWater = false;
  //bool gamma2ScatteredInWater = false;

  void reset()
  {
    gamma1.SetXYZT(0, 0, 0, -1);
    gamma2.SetXYZT(0, 0, 0, -1);
    gammaPrompt.SetXYZT(0, 0, 0, -1);
    //gammaPromptScatteredInWater = false;
    //gamma1ScatteredInWater = false;
    //gamma2ScatteredInWater = false;
  }
};

random_device rd;
mt19937 gen(rd());

Double_t sigmaE(Double_t E, Double_t coeff = 0.0444)
{
  return coeff / TMath::Sqrt(E) * E;
}

double r_norm(double mean, double sigmaE)
{
  normal_distribution<double> d(mean, sigmaE);
  return d(gen);
}

bool isEqual(double x, double y, double epsilon = 10e-9)
{
  return std::abs(x - y) < epsilon;
}

double smearEnergy(double energy)
{
  return r_norm(energy, 1000. * sigmaE((energy) * 1. / 1000.));
}

/*double calculateDistance(double x1, double y1, double x2, double y2, double radius)
{
double distance =0;
distance= sqrt(pow((radius/2),2)-pow((x2-x1),2)-pow((y2-y1),2));
return distance;
}*/

/// return true if full event has been read.
/// Track1 ==> prompt
/// Track2 ==> 511 keV
/// Track3 ==> 511 keV
bool readEvent(const GlobalActorReader& gar, FullEvent& outEvent)
{
  int trackID = gar.GetTrackID();
  //std::string volume_name = gar.GetVolumeName();
  double energyDeposition = gar.GetEnergyLossDuringProcess();
  double emissionEnergy = gar.GetEmissionEnergyFromSource();
  auto hitPosition = gar.GetProcessPosition(); /// I am not sure here

  int currentEventID = gar.GetEventID();
  int previousEventID = outEvent.fEventID;
  if ( previousEventID != currentEventID ) {
    if ( previousEventID > -1 ) {
      outEvent.fEventID = currentEventID; /// we save new eventID to outEvent
      return true;
    } else {
      outEvent.fEventID = currentEventID; /// this is the first event starting
      return false;
    }
  }

/// if there is multiple scattering we save only the last part
  if (isEqual(emissionEnergy, 511) || isEqual(emissionEnergy, 1157)) {
    /// scatter in detector
    if (trackID == 1) {
      assert(!isEqual(emissionEnergy, 511));
      assert(isEqual(emissionEnergy, 1157));
      outEvent.gammaPrompt = TLorentzVector(hitPosition, energyDeposition);
    }
    if (trackID == 2) {
      assert(isEqual(emissionEnergy, 511));
      assert(!isEqual(emissionEnergy, 1157));
      outEvent.gamma1 = TLorentzVector(hitPosition, energyDeposition);
    }
    if (trackID == 3) {
      assert(isEqual(emissionEnergy, 511));
      assert(!isEqual(emissionEnergy, 1157));
      outEvent.gamma2 = TLorentzVector(hitPosition, energyDeposition);
    }
  }
  return false;
}

TH1F* hAllPrompt = 0;
TH1F* hAll511 = 0;
TH1F* h3detPrompt = 0;
TH1F* h2det1Prompt = 0;
TH1F* h2det2Prompt = 0;
TGraph* purity_prompt = 0;
TGraph* efficiency_prompt = 0;
TGraph* ROC_prompt = 0;
TGraph* purity_511 = 0;
TGraph* efficiency_511 = 0;
TGraph* ROC_511 = 0;

void createHistograms()
{
  hAllPrompt = new TH1F( "hAllPrompt", "All ", 500, 0, 1200);
  hAll511 = new TH1F( "hAll511", "All ", 500, 0, 1200);
  h3detPrompt = new TH1F( "h3detPrompt", "Prompt with 3 det", 500, 0, 1200);
  h2det1Prompt = new TH1F( "h2det1Prompt", "Prompt with det prompt and gamma1 ", 500, 0, 1200);
  h2det2Prompt = new TH1F( "h2det2Prompt", "Prompt with det prompt and gamma2  ", 500, 0, 1200);

}

void fillHistograms(const FullEvent& event)
{
  bool isSmearingOn = false;  //change it to true to turn on the smearing
  bool isLowEnergyCutOn = false; //chane it to true to turn on the low energy cut
  const double kLowEnergyThreshold = 100; /// can be also 50 keV
  /// if for gamma energy is less than 0, that means that this this gamma was not registered at all in the scanner
  double promptEnergy = event.gammaPrompt.Energy();
  double gamma1Energy = event.gamma1.Energy();
  double gamma2Energy = event.gamma2.Energy();
  if (isSmearingOn) {
    if (promptEnergy >= 0) {
      promptEnergy = smearEnergy(promptEnergy);

    }
    if (gamma1Energy  >= 0) {
      gamma1Energy = smearEnergy(gamma1Energy);

    }
    if (gamma2Energy  >= 0) {
      gamma2Energy = smearEnergy(gamma2Energy);

    }
  }

  if (isLowEnergyCutOn) {
    if (promptEnergy < kLowEnergyThreshold) {
      promptEnergy = -1;

    }
    if (gamma1Energy  < kLowEnergyThreshold ) {
      gamma1Energy = -1;

    }
    if (gamma2Energy  < kLowEnergyThreshold) {
      gamma2Energy = -1;

    }
  }


  if (promptEnergy > 0) {
    hAllPrompt->Fill(promptEnergy);
  }

  if (gamma1Energy > 0) {
    hAll511->Fill(gamma1Energy);
  }
  if (gamma2Energy > 0) {
    hAll511->Fill(gamma2Energy);
  }

  if (promptEnergy > 0 && gamma1Energy > 0 && gamma2Energy > 0) {
    h3detPrompt->Fill(promptEnergy);
  }
  if (promptEnergy > 0 && gamma1Energy > 0) {
    h2det1Prompt->Fill(promptEnergy);
  }
  if (promptEnergy > 0 && gamma2Energy > 0) {
    h2det2Prompt->Fill(promptEnergy);
  }

  //new TH1F if(promptEnergy>100&&gamma1Energy>100&&gamma2Energy>100)

}

void saveHistograms()
{

  TCanvas c1("c", "c", 2000, 2000);
  hAll511->SetLineColor(kBlack);
  hAll511->Draw();
  hAllPrompt->SetLineColor(kRed);
  hAllPrompt->Draw("same");
  c1.SaveAs("all.png");

  TCanvas c12("c", "c", 2000, 2000);
  h3detPrompt->SetLineColor(kBlack);
  h3detPrompt->Draw();
  h3detPrompt->SetLineColor(kRed);
  c12.SaveAs("h3detPrompt.png");

  TCanvas c13("c", "c", 2000, 2000);
  h2det1Prompt->SetLineColor(kBlack);
  h2det1Prompt->Draw();
  c13.SaveAs("h2det1Prompt.png");

  TCanvas c14("c", "c", 2000, 2000);
  h2det2Prompt->SetLineColor(kBlack);
  h2det2Prompt->Draw();
  c14.SaveAs("h2det2Prompt.png");

  TFile f("histograms_all.root", "recreate");
  hAllPrompt->Write();
  hAll511->Write();
  h3detPrompt->Write();
  h2det1Prompt->Write();
  h2det2Prompt->Write();
  f.Close();
}
void drawPlot()
{
  const double kEnergyMin = 0;
  const double kEnergyMax = 1158;
  const int kNumberOfSteps = 1158;
  const double kEnergyStep = (kEnergyMax - kEnergyMin) / kNumberOfSteps;
  Double_t TPR_prompt[kNumberOfSteps]; 
  Double_t PPV_prompt[kNumberOfSteps];
  Double_t FPR_prompt[kNumberOfSteps]; 
  Double_t TPR_511[kNumberOfSteps]; 
  Double_t PPV_511[kNumberOfSteps]; 
  Double_t FPR_511[kNumberOfSteps]; 
  Double_t energies[kNumberOfSteps];


  int pbin0 = hAllPrompt->GetXaxis()->FindBin(kEnergyMin); /// min bin for Promtp
  int pbinn = hAllPrompt->GetXaxis()->FindBin(kEnergyMax); /// max bin for Prompt
  int bin0 = hAll511->GetXaxis()->FindBin(kEnergyMin); // min bin for 511 
  int binn = hAll511->GetXaxis()->FindBin(kEnergyMax); /// max bin for 511
  for (Int_t i = 0; i < kNumberOfSteps ; i++) {
    double currentEnergy =  kEnergyStep * i;
    energies[i] = currentEnergy; 

    int bini = hAll511->GetXaxis()->FindBin(currentEnergy);
    int pbini = hAllPrompt->GetXaxis()->FindBin(currentEnergy);//zwraca numer binu dla ktrego energia jest i keV

    PPV_prompt[i] = hAllPrompt->Integral(pbini, pbinn) / (hAllPrompt->Integral(pbini, pbinn) + hAll511->Integral(bini, binn)); //purity
    TPR_prompt[i] = hAllPrompt->Integral(pbini, pbinn) / hAllPrompt->Integral(pbin0, pbinn); //effi
    FPR_prompt[i] = (hAll511->Integral(bin0, bini)) / (hAll511->Integral(bin0, binn));

    PPV_511[i] = hAll511->Integral(pbin0, pbini) / (hAllPrompt->Integral(pbin0, pbini) + hAll511->Integral(bin0, bini)); //purity
    TPR_511[i] = hAll511->Integral(pbin0, pbini) / hAll511->Integral(pbin0, pbinn); //effi
    FPR_511[i] = (hAllPrompt->Integral(bin0, bini)) / (hAllPrompt->Integral(bin0, binn));
  }

  ROC_prompt = new TGraph(kNumberOfSteps, TPR_prompt, FPR_prompt);
  efficiency_prompt = new TGraph(kNumberOfSteps, energies, TPR_prompt);
  purity_prompt = new TGraph(kNumberOfSteps, energies, PPV_prompt);

  ROC_511 = new TGraph(kNumberOfSteps, TPR_511, FPR_511);
  efficiency_511 = new TGraph(kNumberOfSteps, energies, TPR_511);
  purity_511 = new TGraph(kNumberOfSteps, energies, PPV_511);
}

void savePlot()
{
  TCanvas c2("c", "c", 2000, 2000);
  purity_prompt->SetLineColor(kBlack);
  purity_prompt->SetTitle("purity prompt");
  purity_prompt->GetXaxis()->SetTitle("n");
  purity_prompt->GetYaxis()->SetTitle("PPV");
  purity_prompt->Draw();
  c2.SaveAs("purity_prompt.png");

  TCanvas c3("c", "c", 2000, 2000);
  purity_511->SetLineColor(kBlack);
  purity_511->SetTitle("purity 511");
  purity_511->GetXaxis()->SetTitle("n");
  purity_511->GetYaxis()->SetTitle("PPV");
  purity_511->Draw();
  c3.SaveAs("purity_511.png");

  TCanvas c4("c", "c", 2000, 2000);
  efficiency_511->SetLineColor(kRed);
  efficiency_511->SetTitle("efficiency 511");
  efficiency_511->GetXaxis()->SetTitle("n");
  efficiency_511->GetYaxis()->SetTitle("TPR");
  efficiency_511->Draw();
  c4.SaveAs("efficiency_511.png");

  TCanvas c5("c", "c", 2000, 2000);
  efficiency_prompt->SetLineColor(kRed);
  efficiency_prompt->SetTitle("efficiency prompt");
  efficiency_prompt->GetXaxis()->SetTitle("n");
  efficiency_prompt->GetYaxis()->SetTitle("TPR");
  efficiency_prompt->Draw();
  c5.SaveAs("efficiency_prompt.png");

  TCanvas c6("c", "c", 2000, 2000);
  ROC_prompt->SetLineColor(kBlack);
  ROC_prompt->SetTitle("ROC prompt");
  ROC_prompt->GetXaxis()->SetTitle("TPR");
  ROC_prompt->GetYaxis()->SetTitle("FPR");
  ROC_prompt->Draw();
  c6.SaveAs("ROc_prompt.png");

  TCanvas c7("c", "c", 2000, 2000);
  ROC_511->SetLineColor(kBlack);
  ROC_511->SetTitle("ROC 511");
  ROC_511->GetXaxis()->SetTitle("TPR");
  ROC_511->GetYaxis()->SetTitle("FPR");
  ROC_511->Draw();
  c7.SaveAs("ROc_511.png");

  TFile f("histograms_all.root", "recreate");
  efficiency_511->Write();
  purity_511->Write();
  ROC_511->Write();
  efficiency_prompt->Write();
  purity_prompt->Write();
  ROC_prompt->Write();
  f.Close();
}

void clearHistograms()
{
  if (hAllPrompt) {
    delete hAllPrompt;
    hAllPrompt = 0;
  }

  if (hAll511) {
    delete hAll511;
    hAll511 = 0;
  }

  if (efficiency_prompt) {
    delete efficiency_prompt;
    efficiency_prompt = 0;
  }

  if (purity_prompt) {
    delete purity_prompt;
    purity_prompt = 0;
  }

  if (ROC_prompt) {
    delete ROC_prompt;
    ROC_prompt = 0;
  }

  if (efficiency_511) {
    delete efficiency_511;
    efficiency_511 = 0;
  }

  if (purity_511) {
    delete purity_511;
    purity_511 = 0;
  }

  if (ROC_511) {
    delete ROC_511;
    ROC_511 = 0;
  }

}

int main(int argc, char* argv[])
{
  if (argc != 3) {
    std::cerr << "Invalid number of variables." << std::endl;
  } else {
    std::string file_name( argv[1] );
    std::string emissionEnergy(argv[2]);


    FullEvent event;
    bool isNewEvent = false;

    createHistograms();
    try {
      GlobalActorReader gar;
      if (gar.LoadFile(file_name)) {
        while (gar.Read()) {
          isNewEvent = readEvent(gar, event);
          if (isNewEvent) {
            fillHistograms(event);
            event.reset();
          }
        }
      } else {
        std::cerr << "Loading file failed." << std::endl;
      }
      drawPlot();
      saveHistograms();
      savePlot();
      clearHistograms();
    } catch (const std::logic_error& e ) {
      std::cerr << e.what() << std::endl;
    } catch (...) {
      std::cerr << "Udefined exception" << std::endl;
    }
  }
  return 0;
}
