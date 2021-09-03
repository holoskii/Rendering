#include "scene.h"

#include<iostream>

int main(int argc, char** argv)
{
	std::string scenePath;
	if (argc > 1)
		scenePath = argv[1];
	else {
		scenePath = std::string("input/shotgun.scene");
	}
	Scene(scenePath).render();
}
