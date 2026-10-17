#include "particle_mocks.inc"
using s_vdp1Command=Command;
struct sVdp1Quad{
 u16 m2_CMDCTRL=0,m4_CMDPMOD=8,m6_CMDSRCA=0x20,m8_CMDSIZE=0x808,mA_CMDCOLR=0x40;
 fixedPoint mC_width,m10_height,m14_X,m18_Y;
};
struct sAnimatedQuad{const std::vector<sVdp1Quad>* m0_quad;u16 m4_vdp1Memory=0x100;unsigned m7_currentFrame=0;};
namespace lagi::platform::logging{template<class... T>void writef(const char*,T...) {}}
#include "particle_body.inc"
void reset(){auto& c=graphicEngineStatus.m14_vdp1Context[0];commands.fill({});c.m0_currentVdp1WriteEA=&commands[6];c.m20_pCurrentVdp1Packet=packets.data();c.m14[0].assign(128,{});c.m10=c.m14[0].begin();c.mC=c.m1C=0;depth.fill(-1);}
int main(){
 std::vector<sVdp1Quad> frames(2);
 frames[0].mC_width=0x20000;frames[0].m10_height=0x10000;
 frames[0].m14_X=-0x18000;frames[0].m18_Y=0xC000;
 frames[1]=frames[0];frames[1].m14_X=0x8000;frames[1].m18_Y=-0x4000;
 sAnimatedQuad p{&frames};sVec3_FP pos{{0,0,0x40000}};
 quadColor colors{0x4210,0x4010,0x3c10,0x2010};
 for(unsigned frame=0;frame<2;++frame)for(int scale:{0x10000,0x8000})for(bool shaded:{false,true}){
  reset();p.m7_currentFrame=frame;
  assert(drawQuadInternal(&p,&pos,scale,shaded?&colors:nullptr)==1);
  const auto& q=frames[frame];const auto& cmd=commands[6];const auto inv=FP_Div(0x10000,pos[2]);
  // Independently projected authored top-left and full dimensions, not two
  // rounded half-extents around a forced center.
  int x=MTH_Mul_5_6(288,MTH_Mul(q.m14_X,scale),inv).asS32();
  int y=MTH_Mul_5_6(224,MTH_Mul(q.m18_Y,scale),inv).asS32();
  int w=MTH_Mul_5_6(288,MTH_Mul(q.mC_width,scale),inv).asS32();
  int h=MTH_Mul_5_6(224,MTH_Mul(q.m10_height,scale),inv).asS32();
  assert(cmd.mC_CMDXA==x&&cmd.m10_CMDXB==x+w&&cmd.m18_CMDXD==x&&cmd.m14_CMDXC==x+w);
  assert(cmd.mE_CMDYA==-y&&cmd.m12_CMDYB==-y&&cmd.m16_CMDYC==h-y&&cmd.m1A_CMDYD==h-y);
  assert(cmd.m8_CMDSRCA==0x120&&cmd.m6_CMDCOLR==0x140&&cmd.mA_CMDSIZE==0x808);
  assert(bool(cmd.m4_CMDPMOD&4)==shaded);
  if(shaded)assert(graphicEngineStatus.m14_vdp1Context[0].m14[0][0]==colors);
  assert(depth[0]==4&&depth[3]==4);
 }
 reset();pos[2]=0x1000;assert(drawQuadInternal(&p,&pos,0x10000)==0);
 puts("PASS: production particle frame offsets, scaled dimensions, Gouraud, source/LUT, depth and clipping. Not hardware validation.");
}
