// Timer to measure execution time in milliseconds
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
		auto stopTime = std::chrono::high_resolution_clock::now();
		long long duration = std::chrono::duration_cast<std::chrono::milliseconds>(stopTime - startTime).count();
		if (options::enableOutput) {
			std::cout << name << " \t" << duration << " ms" << std::endl;
		}
		return duration;
	}

private:
	std::string name;
	std::chrono::high_resolution_clock::time_point startTime;
	bool running;
};
