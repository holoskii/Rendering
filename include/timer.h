#pragma once

#include <string>
#include <chrono>
#include <iostream>

class Timer 
{
public:
	Timer(std::string a_name = "Unnamed timer:")
		: name{ a_name } {
		start = std::chrono::high_resolution_clock::now();
	}
	~Timer() {
		auto stop = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
		std::cout << name  << " \t" << duration << " ms" << std::endl;
	}

private:
	std::string name;
	std::chrono::steady_clock::time_point start;
};