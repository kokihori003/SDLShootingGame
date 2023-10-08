#include "RepairItem.h"
#include "SpriteComponent.h"
#include "MoveComponent.h"
#include "Game.h"
#include "Random.h"
#include "CircleComponent.h"

RepairItem::RepairItem(Game* game)
	:Actor(game)
	, mCircle(nullptr)
{
	// Initialize to random position/orientation
	Vector2 randPos = Random::GetVector(Vector2::Zero,
		Vector2(1024.0f, 768.0f));

	//Spawn positionをlaneの右端三か所に変更
	laneNum = rand() % 3; //乱数で生成するレーンを決定
	Vector2 pos = Vector2(1000.0f, lane[laneNum]);
	SetPosition(pos);

	SetRotation(0.0f);

	// Create a sprite component
	SpriteComponent* sc = new SpriteComponent(this);
	sc->SetTexture(game->GetTexture("Assets/RepairItem.png"));

	// Create a move component, and set a forward speed
	MoveComponent* mc = new MoveComponent(this);
	mc->SetForwardSpeed(-150.0f);

	// Create a circle component (for collision)
	mCircle = new CircleComponent(this);
	mCircle->SetRadius(35.0f);

	// Add to mAsteroids in game
	game->AddRepairItem(this);
}

RepairItem::~RepairItem()
{
	GetGame()->RemoveRepairItem(this);
}