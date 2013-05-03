{

  TStyle *mystyle=new TStyle("mystyle","mystyle");
  *mystyle = *(gROOT->GetStyle("Plain"));
  mystyle->SetName("mystyle");
  gROOT->SetStyle("mystyle");
mystyle->SetCanvasColor(kWhite);

mystyle->SetTitleFillColor(kWhite);

mystyle->SetFuncWidth(2);
mystyle->SetHistLineWidth(2);
mystyle->SetLegendBorderSize(0);
//mystyle->SetOptFit(1111);
mystyle->SetStatBorderSize(0);
mystyle->SetTitleBorderSize(0);
mystyle->SetDrawBorder(0);
mystyle->SetLabelSize(.04,"xyz");
mystyle->SetTitleSize(.04,"xyz");
mystyle->SetLabelFont(102,"xyz");
mystyle->SetOptStat("");
mystyle->SetStatFont(102);
mystyle->SetTitleFont(102,"xyz");
mystyle->SetTitleFont(102,"pad");
mystyle->SetStatStyle(0);
mystyle->SetStatX(1);
mystyle->SetStatY(1);
mystyle->SetStatW(.2);
mystyle->SetStatH(.15);
mystyle->SetTitleStyle(0);
mystyle->SetTitleX(.2);
mystyle->SetTitleW(.65);
mystyle->SetTitleY(.98);
mystyle->SetTitleH(.07);
mystyle->SetStatColor(0);
mystyle->SetStatBorderSize(0);
mystyle->SetFillColor(10);
mystyle->SetFillStyle(0);
mystyle->SetTextFont(102);
mystyle->SetCanvasBorderMode(0);
mystyle->SetPadBorderMode(1);
mystyle->SetFrameBorderMode(0);
mystyle->SetDrawBorder(0);

mystyle->SetPalette(1,0);
const Int_t NRGBs = 5;
 const Int_t NCont = 255;
 
 Double_t stops[NRGBs] = { 0.00, 0.34, 0.61, 0.84, 1.00 };
 Double_t red[NRGBs]   = { 0.00, 0.00, 0.87, 1.00, 0.51 };
 Double_t green[NRGBs] = { 0.00, 0.81, 1.00, 0.20, 0.00 };
 Double_t blue[NRGBs]  = { 0.51, 1.00, 0.12, 0.00, 0.00 };
 TColor::CreateGradientColorTable(NRGBs, stops, red, green, blue, NCont);
 mystyle->SetNumberContours(NCont);


//gROOT->ForceStyle(true);

//TBrowser browser;
}
