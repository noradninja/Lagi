#include <array>
#include <vector>
#include <cassert>
#include <cstdio>
using s32=int;
struct Task {int finishes=0;void markFinished(){++finishes;}};
struct s_workArea {Task task;Task* getTask(){return &task;}};
using sWorldGridCellTask=s_workArea;
struct sCellObjectListNode {sCellObjectListNode* m0_next=nullptr;int destroys=0;};
struct CheckedList:std::vector<sCellObjectListNode*> {
 sCellObjectListNode*& operator[](size_t n){assert(n<size());return std::vector<sCellObjectListNode*>::operator[](n);}
};
struct sWorldGrid {
 int m0_sizeX=0,m4_sizeY=0,m8=0,mC=0,m10_currentX=0,m14_currentY=0;
 void* m34_dataBuffer=nullptr;
 void(*m18_createCell)(s32,sWorldGrid*)=nullptr;void(*m1C_createCellColumn)(s32,sWorldGrid*)=nullptr;
 void(*m20_deleteCell)(s32,sWorldGrid*)=nullptr;void(*m24_deleteCellColumn)(s32,sWorldGrid*)=nullptr;
 std::array<std::array<sWorldGridCellTask*,8>,8> m40_cellTasks{};
 CheckedList m140_perCellObjectList;int* m3C=nullptr;
} gWorldGrid;
void createGridRow(s32,sWorldGrid*){}void createGridColumn(s32,sWorldGrid*){}void deleteGridColumn(s32,sWorldGrid*){}
void destroyCellObject(sCellObjectListNode* node){++node->destroys;}
#include "delete_cells.inc"
#include "delete_row.inc"
#include "init.inc"
#include "drain.inc"
int main(){
 std::array<sWorldGridCellTask,64> oldCells;
 // Every row may contain a battle cell, independent of the current ring origin.
 for(int y=0;y<8;++y)for(int x=0;x<8;++x)gWorldGrid.m40_cellTasks[y][x]=&oldCells[y*8+x];
 gWorldGrid.m0_sizeX=2;gWorldGrid.m4_sizeY=2;gWorldGrid.m8=6;gWorldGrid.mC=7;
 gWorldGrid.m10_currentX=1;gWorldGrid.m14_currentY=1;
 gWorldGrid.m140_perCellObjectList.resize(4);
 initTownGrid();
 assert(gWorldGrid.m0_sizeX==0&&gWorldGrid.m4_sizeY==0&&gWorldGrid.m140_perCellObjectList.empty());
 for(auto& row:gWorldGrid.m40_cellTasks)for(auto* cell:row)assert(!cell);
 deleteAllGridCells(&gWorldGrid); // exact empty-list transition that formerly indexed stale cells
 for(auto& cell:oldCells)assert(cell.task.finishes==0); // parent owns outgoing task teardown
 // Normal live-cell retirement still destroys objects and finishes its task.
 gWorldGrid.m0_sizeX=4;gWorldGrid.m4_sizeY=4;gWorldGrid.m8=0;gWorldGrid.mC=0;
 gWorldGrid.m10_currentX=1;gWorldGrid.m14_currentY=1;gWorldGrid.m140_perCellObjectList.resize(16);
 sCellObjectListNode node;gWorldGrid.m140_perCellObjectList[5]=&node;
 gWorldGrid.m40_cellTasks[0][0]=&oldCells[0];deleteGridRow(0,&gWorldGrid);
 assert(node.destroys==1&&oldCells[0].task.finishes==1&&!gWorldGrid.m40_cellTasks[0][0]);
 puts("PASS actual town ring init/delete-row/drain: all 64 stale slots cleared before empty-list reset; live cells still retire normally; object destruction mocked");
}
