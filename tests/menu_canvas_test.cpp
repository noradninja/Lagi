#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
int width=960,height=544;
int viewerRenderWidth(){return width;} int viewerRenderHeight(){return height;}
#include "menu_canvas_production.inc"
int main(){
 for(auto dimensions:{std::pair{960,544},std::pair{480,272},std::pair{640,384},std::pair{640,480},std::pair{320,480}}){
  width=dimensions.first;height=dimensions.second;
  float edge=menuCanvasHalfWidth();
  float expected=std::min(1.0f,4.0f*height/(3.0f*width));
  assert(std::abs(edge-expected)<0.000001f);
  assert(edge>0 && edge<=1);
 }
 puts("PASS: actual menu canvas bounds at full/half/movie/4:3/portrait sizes; GPU masking untested");
}
