// From Game Programming in C++ by Sanjay Madhav
// Copyright (C) 2017 Sanjay Madhav. All rights reserved.
#include "Game.h"

int main(int argc, char** argv)
{
	Game game;
	bool success;
	bool loop;

	success = true;

	while (success)
	{
		success = game.Initialize();
		if (!success)
		{
			break;
		}
		game.RunLoop();
		loop = game.Shutdown();
		if (!loop)
		{
			break;
		}
	}
	return 0;
}