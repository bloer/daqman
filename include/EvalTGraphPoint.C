#include "TROOT.h"
#include "TGraph.h"

double EvalTGraphPoint(const TGraph gr, const double x_pt) {
  const int N = gr.GetN();
  double * x_arr = gr.GetX();
  double * y_arr = gr.GetY();
  for(int i = 0; i < N; ++i)
    if(x_pt == x_arr[i]) {
      return y_arr[i];
    }
  return gr.Eval(x_pt);
}
