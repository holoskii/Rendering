// timer to measure execution time in milliseconds
#pragma once

#include <string>
#include <chrono>
#include <iostream>

class Timer
{
public:
	Timer(std::string a_name = "Unnamed timer:")
		: name{ a_name }
	{
		startTime = std::chrono::high_resolution_clock::now();
		running = true;
	}

	~Timer()
	{
		stop();
	}

	long long stop()
	{
		if (!running)
			return 0;
		running = false;
#ifndef _NO_OUTPUT 
		auto stopTime = std::chrono::high_resolution_clock::now();
		long long duration = std::chrono::duration_cast<std::chrono::milliseconds>(stopTime - startTime).count();
		std::cout << name << " \t" << duration << " ms" << std::endl;
		return duration;
#endif // _NO_OUTPUT
	}

private:
	std::string name;
	std::chrono::steady_clock::time_point startTime;
	bool running;
};