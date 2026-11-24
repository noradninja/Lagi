from pathlib import Path
import struct, subprocess, tempfile, sys
r=Path(__file__).resolve().parents[1]
b=Path(sys.argv[1]).read_bytes(); base=0x6054000
u=lambda a: struct.unpack_from('>I',b,a-base)[0]
assert u(0x6091498)==0x609146c
counts={}
for cell in range(9):
 p=u(0x609146c+cell*4)
 while p and u(p):
  model=struct.unpack_from('>H',b,u(p)-base)[0]
  counts[model]=counts.get(model,0)+1
  p+=16
assert counts=={0xB30:5,0xB38:15,0xB34:14,0xADC:4,0xAE0:4,0xAE4:4,0xAE8:6,0xAEC:6,0xAF0:6}
assert u(0x60781C8)==0x601E424
for ea,word in [(0x60780A2,0x5EC4),(0x6078112,0x6441),(0x6078116,0x644D),(0x607813E,0x7E10)]:
 assert struct.unpack_from('>H',b,ea-base)[0]==word
fixture=r"""
#include <vector>
#include <cassert>
#include <cstdint>
using s32=int;using u16=uint16_t;
struct V{int a[3];int& operator[](unsigned i){return a[i];}};
struct Ptr{int model,collision;Ptr operator+(int)const{return {collision,0};}};
int readSaturnU16(Ptr p){return p.model;}
struct s_grid2{Ptr m0;V m4;};struct s_DataTable3{};
struct Model{int m4_numVertices=4;};struct Bundle{Model model;Model* get3DModel(unsigned){return &model;}};
struct s_visdibilityCellTask{int m14_index=0;struct{Bundle* m0_mainMemoryBundle;}m0_memoryLayout;};
struct Grid{s_DataTable3* m30;unsigned m12E0=0,m12E2=0,m12F0=0,m1300=1;int(*m12FC_isObjectClipped)(const V*,s32);};
struct Sub{Grid* m348_pFieldCameraTask1;};struct Field{Sub* m8_pSubFieldData;}field;
Field* getFieldTaskPtr(){return &field;}
struct Matrix{int m[3][4];}matrix,*pCurrentMatrix=&matrix;
Matrix stack[4];int sp=0,draws=0,collisions=0,scopes=0;
namespace lagi::azel_bridge{void begin_view_relative_submission_scope(){assert(!sp);++scopes;}void end_view_relative_submission_scope(){assert(!sp);--scopes;}}
void pushCurrentMatrix(){stack[sp++]=matrix;}void popMatrix(){matrix=stack[--sp];}
void translateCurrentMatrix(const V* p){for(int a=0;a<3;++a)matrix.m[a][3]+=p->a[a];}
unsigned gridCellDraw_GetDepthRange(int z){return z>100?2:0;}
void addObjectToDrawList(Model*){++draws;assert(scopes==1);assert(matrix.m[0][3]==10&&matrix.m[1][3]==20&&matrix.m[2][3]==30);for(int r=0;r<3;++r)for(int c=0;c<3;++c)assert(matrix.m[r][c]==(r==c?(r==0?-65536:65536):0));}
void callGridCellDraw_normalSub2(Bundle*,u16 offset){assert(offset==7);++collisions;}
// PRODUCTION
int main(){s_DataTable3 layout,other;Bundle bundle;s_visdibilityCellTask cell{0,{&bundle}};Grid grid{&layout,0,0,0,1,[](const V* p,s32 f)->int{assert(f==999);return p->a[0]<0;}};Sub sub{&grid};field.m8_pSubFieldData=&sub;
lagiScreenSceneryLayout=&layout;lagiScreenSceneryCells={{{{1,7},{{10,20,30}}},{{2,0},{{-1,0,0}}},{{3,0},{{0,0,200}}}}};
matrix.m[0][0]=123;Matrix initial=matrix;lagiDrawFieldScreenScenery(&cell,999);assert(draws==1&&collisions==1&&grid.m12E0==3&&grid.m12E2==2&&grid.m12F0==12&&!sp&&!scopes);assert(matrix.m[0][0]==initial.m[0][0]);grid.m30=&other;lagiDrawFieldScreenScenery(&cell,999);assert(draws==1);cell.m14_index=-1;lagiDrawFieldScreenScenery(&cell,999);}
""".replace('// PRODUCTION',(r/'src/integration/lagi_field_screen_scenery.inc').read_text())
with tempfile.TemporaryDirectory() as d:
 p=Path(d);(p/'test.cpp').write_text(fixture)
 subprocess.run(['cl','/nologo','/EHsc','/std:c++20',str(p/'test.cpp'),'/Fe:'+str(p/'test.exe'),'/Fo:'+str(p/'test.obj')],check=True)
 subprocess.run([str(p/'test.exe')],check=True)
print('PASS 64 original placements; production draw clipping, LOD, native basis, collision, layout guard and matrix restoration')

