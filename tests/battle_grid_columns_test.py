from pathlib import Path
import subprocess,tempfile,sys,struct
r=Path(__file__).resolve().parents[1];b=Path(sys.argv[1]).read_bytes();base=0x6000000
for ea,word in [(0x60148C8,0x2B79),(0x60148CC,0x2869),(0x601492C,0x1423),(0x6014932,0x1435),(0x6014A60,0x24C2)]:
 assert struct.unpack_from('>H',b,ea-base)[0]==word
fixture=r"""
#include <cassert>
#include <cstdint>
#include <vector>
using s32=int32_t;using u32=uint32_t;
struct Vec{int a[3];int& operator[](unsigned n){return a[n];}};
struct Task{bool finished=false;void markFinished(){finished=true;}};
struct sWorldGridCellTask{struct TypedTaskDefinition{void(*init)(sWorldGridCellTask*,Vec);void* update;void(*draw)(sWorldGridCellTask*);void* del;};Vec mC_position;Task task;Task* getTask(){return &task;}};
void sBattleEnvironmentGridCell_Init(sWorldGridCellTask* c,Vec p){c->mC_position=p;}
void sBattleEnvironmentGridCell_Draw(sWorldGridCellTask*){}
struct Setup{Vec cells[2][4];};
struct sWorldGrid{int m10_currentX,m14_currentY,m0_sizeX=2,m4_sizeY=4,m28_cellSize=100,mC=6,m8=7;Setup* m38_EnvironmentSetup;void* m34_dataBuffer=nullptr;sWorldGridCellTask* m40_cellTasks[8][8]{};};
std::vector<sWorldGridCellTask*> owned;int children=0;
template<class T>T* createSubTaskWithArgWithCopy(void*,Vec p,const typename T::TypedTaskDefinition* d){auto* c=new T;d->init(c,p);owned.push_back(c);return c;}
void battleGrid_createCellSub0(sWorldGridCellTask* c,int x,int y,int ox,int oy){assert(c->mC_position[0]==50+x*100+ox&&c->mC_position[2]==50+y*100+oy);++children;}
// PRODUCTION
int main(){Setup setup;for(int x=0;x<2;++x)for(int y=0;y<4;++y)setup.cells[x][y]={{50+x*100,0,50+y*100}};
for(int cx=-9;cx<=9;++cx)for(int cy=-9;cy<=9;++cy)for(int index:{-2,2}){sWorldGrid g;g.m10_currentX=cx;g.m14_currentY=cy;g.m38_EnvironmentSetup=&setup;sWorldGridCellTask untouched;g.m40_cellTasks[0][0]=&untouched;auto n=owned.size();battleGrid_cellFunc1(index,&g);assert(owned.size()==n+5);
for(int row=-2;row<=2;++row){auto* c=g.m40_cellTasks[(g.mC+row)&7][(g.m8+index)&7];assert(c&&c->mC_position[0]==50+(cx+index)*100&&c->mC_position[2]==50+(cy+row)*100&&!c->task.finished);}
battleGrid_cellFunc2(index,&g);for(auto i=n;i<owned.size();++i)assert(owned[i]->task.finished);assert(!untouched.task.finished);for(int row=-2;row<=2;++row)assert(!g.m40_cellTasks[(g.mC+row)&7][(g.m8+index)&7]);battleGrid_cellFunc2(index,&g);}
for(auto* c:owned)delete c;assert(children==19*19*2*5);}
""".replace('// PRODUCTION',(r/'src/integration/lagi_battle_grid_columns.inc').read_text())
with tempfile.TemporaryDirectory() as d:
 p=Path(d);(p/'test.cpp').write_text(fixture)
 subprocess.run(['cl','/nologo','/EHsc','/std:c++20',str(p/'test.cpp'),'/Fe:'+str(p/'test.exe'),'/Fo:'+str(p/'test.obj')],check=True)
 subprocess.run([str(p/'test.exe')],check=True)
print('PASS native wrap/translation/retirement anchors; production streaming 722 columns across negative and positive coordinates')
