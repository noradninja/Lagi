#pragma once
#include <memory>
struct sMonsterBody;
struct s_workArea;struct sSaturnPtr;struct sVec3_FP;
s_workArea* lagiCreateA5StoryWorm(s_workArea* parent,sSaturnPtr data);
void lagiTrackA5StoryWorm(s_workArea* actor,sSaturnPtr commands,int threshold,int bit);
sVec3_FP* lagiA5StoryWormCameraAnchor(s_workArea* actor);
void lagiStartA5StoryWormCamera(s_workArea* parent,sSaturnPtr script,unsigned callback,sVec3_FP* subject);
void lagiSpawnA5WormBreachSand(sVec3_FP* position);
void lagiSpawnA5WormSegmentSand(sVec3_FP* position);
void lagiSpawnA5WormSegmentDebris(const std::shared_ptr<sMonsterBody>& body,int part);
