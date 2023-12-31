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

	std::vector<float>  lane = { 200.0f, 380.0f, 560.0f }; //レーンのy座標一覧
	int laneNum = 0;
};