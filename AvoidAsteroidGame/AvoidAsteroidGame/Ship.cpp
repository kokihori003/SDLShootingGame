// From Game Programming in C++ by Sanjay Madhav
// Copyright (C) 2017 Sanjay Madhav. All rights reserved.
#include "Ship.h"
#include "SpriteComponent.h"
#include "InputComponent.h"
#include "CircleComponent.h"
#include "Game.h"
#include "Laser.h"
#include "Asteroid.h"
#include "RepairItem.h"

Ship::Ship(Game* game)
	:Actor(game)
	, mLaserCooldown(0.0f)
	, mDownSpeed(0.0f)
{
	mGame = game;
	// Create a sprite component
	spriteComponent = new SpriteComponent(this, 150);
	mTexture = game->GetTexture("Assets/ShipWithThrust.png");
	spriteComponent->SetTexture(mTexture);

	// Create a circle component (for collision)
	mCircle = new CircleComponent(this);
	mCircle->SetRadius(30.0f);

	// Create an input component and set keys/speed
	InputComponent* ic = new InputComponent(this);
	/*ic->SetForwardKey(SDL_SCANCODE_W);
	ic->SetBackKey(SDL_SCANCODE_S);
	ic->SetClockwiseKey(SDL_SCANCODE_A);
	ic->SetCounterClockwiseKey(SDL_SCANCODE_D);
	ic->SetMaxForwardSpeed(300.0f);
	ic->SetMaxAngularSpeed(Math::TwoPi);*/

	ic->SetForwardKey(SDL_SCANCODE_D);
	ic->SetBackKey(SDL_SCANCODE_A);
	ic->SetMaxForwardSpeed(300.0f);
}

Ship::~Ship()
{
	mGame->CheckShip();
}

void Ship::UpdateActor(float deltaTime)
{
	if (endShipCheck)
	{
		EndShip(deltaTime);
		return;
	}

	mLaserCooldown -= deltaTime;
	if (reload)
	{
		reloadTime += deltaTime;
	}
	if (reloadTime >= waitReloadTime && reload)
	{
		//リロード終了音を１回再生
		mGame->PlayEndReload();
		reload = false;
		laserNum = 10;
		reloadTime = 0.0f;
	}
	//衝突判定
	CheckCollision(deltaTime);

	//Update lane position
	Vector2 pos = GetPosition();
	laneNum = mDownSpeed;
	//Restrict position to left half of screen
	if (pos.x < 25.0f)
	{
		pos.x = 25.0f;
	}
	else if (pos.x > 500.0f)
	{
		pos.x = 500.0f;
	}
	if (laneNum < 0)
	{
		laneNum = 0;
	}
	else if (laneNum > 2)
	{
		laneNum = 2;
	}
	pos.y = lane[laneNum];
	SetPosition(pos);
	if (moveCheck)
	{
		count += deltaTime;
		if (count > 0.17)
		{
			moveCheck = false;
			count = 0;
		}
	}
}

void Ship::ActorInput(const uint8_t* keyState)
{
	if (keyState == nullptr)
	{
		return;
	}
	if (keyState[SDL_SCANCODE_SPACE] && mLaserCooldown <= 0.0f && !reload)
	{
		// Create a laser and set its position/rotation to mine
		Fire();
		// Reset laser cooldown (half second)
		mLaserCooldown = 0.5f;
	}
	mDownSpeed = laneNum;

	//up/down
	if (!moveCheck && keyState[SDL_SCANCODE_S])
	{
		mDownSpeed += 1.0f;
		moveCheck = true;
	}

	if (!moveCheck && keyState[SDL_SCANCODE_W])
	{
		mDownSpeed -= 1.0f;
		moveCheck = true;
	}
}

void Ship::Fire()
{
	// Create a laser and set its position/rotation to mine
	Laser* laser = new Laser(GetGame());
	mGame->PlayLaserSE();
	laser->SetPosition(GetPosition());
	laser->SetRotation(GetRotation());
	//弾切れかどうかを判定
	if (--laserNum == 0)
	{
		//リロード開始音を一回再生
		mGame->PlayStartReload();
		reload = true;
	}

	// Reset laser cooldown (half second)
	mLaserCooldown = 0.5f;
}

void Ship::CheckCollision(float deltaTime)
{
	//RepairItemとの衝突判定
	for (auto rep : GetGame()->GetRepairItems())
	{
		if (Intersect(*mCircle, *(rep->GetCircle())))
		{
			// The first asteroid we intersect with,
			// set ourselves and the asteroid to dead
			rep->SetState(EDead);
			OnCollisionRepairItems();
			break;
		}
	}

	//無敵時間適用中はこれ以下の衝突判定を行わない
	if (noDamageTime > noDamageTimecount)
	{
		noDamageTimecount += deltaTime;
		SetDamageTex(deltaTime);
		return;
	}
	else if (noDamageTimecount > noDamageTime && noDamage)
	{
		SetShipTex();
		noDamage = false;
	}

	//Asteroidとの衝突判定
	// Do we intersect with an asteroid?
	for (auto ast : GetGame()->GetAsteroids())
	{
		if (Intersect(*mCircle, *(ast->GetCircle())))
		{
			// The first asteroid we intersect with,
			// set ourselves and the asteroid to dead
			ast->SetState(EDead);
			OnCollisionAsteroids();
			break;
		}
	}

}

void Ship::OnCollisionAsteroids()
{
	
	//ダメージ計算処理
	if (mHealth <= 1) 
	{
		endShipCheck = true;
	}
	else
	{
		mHealth--;
	}

	//SEの再生
	mGame->PlayDamageSE();

	//無敵時間を適用する
	noDamage = true;
	noDamageTimecount = 0.0f;

}

void Ship::OnCollisionRepairItems()
{
	//HP回復処理
	if (++mHealth > 3) { mHealth = 3; }
	mGame->AddGetRepairItemsCount();
	//SEの再生
	mGame->PlayRepairSE();
	
}

void Ship::SetDamageTex(float deltaTime)
{
	//0.2秒に一回フラグを切り替え
	//tureならtextureにnullptr,falseなら従来の画像アドレスを代入
	countInterval += deltaTime;
	if (countInterval >= 0.2 && showImage) {
		showImage = false;
		countInterval = 0.0f;
	}
	else if(countInterval >= 0.2 && !showImage)
	{
		showImage = true;
		countInterval = 0.0f;
	}

	if (showImage)
	{
		spriteComponent->SetTexture(mTexture);
	}
	else
	{
		spriteComponent->SetTexture(nullptr);
	}
	
}

void Ship::SetShipTex()
{
	spriteComponent->SetTexture(mTexture);
}

void Ship::SetExplosionTex()
{
	mTexture = mGame->GetTexture("Assets/bakuhatsu.png");
	spriteComponent->SetTexture(mTexture);
}

void Ship::EndShip(float deltaTime)
{
	if (exCheck)
	{
		//効果音再生
		mGame->PlayDestroySE();

		//爆発のイラスト表示
		SetExplosionTex();

		exCheck = false;
	}
	else
	{
		endCountTime += deltaTime;
	}
	if (3.0f <= endCountTime)
	{
		SetState(EDead);
	}
}