#include <cassert>
#include <cstdio>
#include <array>
struct Vec{int x=0,y=0,z=0;Vec operator+(Vec b)const{return {x+b.x,y+b.y,z+b.z};}Vec& operator+=(Vec b){*this=*this+b;return *this;}};
struct s_cutsceneTask2{static void Update(s_cutsceneTask2*);int m0=0,m34=1,m38=0,m40=20,m20=0,m5C=0,m60=0;
 Vec m8,m14,m28,m44,m50;Vec* m24=nullptr;int starts=0;
 int UpdateSub0(){++starts;m40=20;return 1;}};
struct Camera{std::array<int,3>mC_rotation{};}camera;
Camera* getFieldCameraStatus(){return &camera;}
int trackCalls=0,ends=0;
void startCutsceneCameraTracking(Vec*,Vec*){++trackCalls;}
struct Script{int m5C=0;}script;
struct FieldData{Script* m34C_ptrToE=&script;}fieldData;
struct Field{FieldData* m8_pSubFieldData=&fieldData;int m2C_currentFieldIndex=1,m2E_currentSubFieldIndex=0;}field;
Field* getFieldTaskPtr(){return &field;}
struct Fade{bool m20_stopped=true;};struct Fades{Fade m0_fade0;}g_fadeControls;
namespace lagi::platform::logging{void writef(const char*,...) {}}
void lagiCutCompletedExcavationCamera(){}
void endCutsceneTask(s_cutsceneTask2*){++ends;}
// PRODUCTION_UPDATE
int main(){
 Vec subject{100,200,300};s_cutsceneTask2 t;t.m24=&subject;
 t.m8={1,2,3};t.m44={4,5,6};t.m28={10,20,30};t.m50={2,3,4};
 for(int i=0;i<10;++i){subject.x+=7;subject.y-=9;subject.z+=11;s_cutsceneTask2::Update(&t);
  assert(t.m14.x==t.m28.x+subject.x&&t.m14.y==t.m28.y+subject.y&&t.m14.z==t.m28.z+subject.z);
  assert(t.m8.x==1+4*(i+1)&&t.m60==i+1&&ends==0);}
 assert(t.m40==10);t.m24=nullptr;s_cutsceneTask2::Update(&t);
 assert(t.m14.x==t.m28.x&&t.m14.y==t.m28.y&&t.m14.z==t.m28.z);
 t.m40=1;s_cutsceneTask2::Update(&t);assert(t.starts==1&&t.m38==1);
 t.m34=-1;script.m5C=1;s_cutsceneTask2::Update(&t);assert(ends==0);
 script.m5C=0;s_cutsceneTask2::Update(&t);assert(ends==1);
 s_cutsceneTask2 init;init.m34=0;s_cutsceneTask2::Update(&init);assert(trackCalls==1&&init.starts==1);
 puts("PASS production shared camera: live moving subject, independent camera trajectory, fixed target, segment advance and held/completed lifecycle");
}
