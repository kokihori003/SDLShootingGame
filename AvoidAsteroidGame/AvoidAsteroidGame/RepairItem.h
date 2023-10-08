#pragma once
#include "Actor.h"
class RepairItem : public Actor
{
public:
	RepairItem(class Game* game);
	~RepairItem();

	class CircleComponent* GetCircle() { return mCircle; }
private:
	class CircleComponent* mCircle;

	std::vector<float>  lane = { 200.0f, 380.0f, 560.0f }; //ƒŒ[ƒ“‚ÌyÀ•Wˆê——
	int laneNum = 0;
};