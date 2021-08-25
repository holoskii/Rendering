#include "scene.h"

#include<iostream>

int main(int argc, char** argv)
{
	std::string scenePath;
	if (argc > 1)
		scenePath = argv[1];
	else {
		scenePath = std::string("D:\\dev\\CG\\RayTracing\\input\\shotgun.scene");
	}
	Scene(scenePath).render();
}
