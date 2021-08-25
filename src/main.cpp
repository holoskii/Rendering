#include "scene.h"

#include<iostream>

int main(int argc, char** argv)
{
	std::string scenePath;
	if (argc > 1)
		scenePath = argv[1];
	else {
		scenePath = std::string(argv[0]);
		size_t loc = scenePath.rfind('\\');
		if (loc >= scenePath.length())
			loc = 0;
		scenePath.erase(loc);
		scenePath = scenePath + std::string("\\input\\v0.scene");
	}
	Scene(scenePath).render();
}
