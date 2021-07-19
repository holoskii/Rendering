#pragma once

#include <iostream>
#include <iomanip>
#include <atomic>

#ifdef _STATS
namespace stats
{
	inline std::atomic<int> rayTriTests{ 0 };
	inline std::atomic<int> accelStructTests{ 0 };
	inline std::atomic<int> triCopiesCount{ 0 };
	inline std::atomic<int> meshCount{ 0 };
	inline std::atomic<int> acCount{ 0 };

	inline void clearStats()
	{
		rayTriTests.store(0);
		accelStructTests.store(0);
		triCopiesCount.store(0);
		meshCount.store(0);
	}

	inline void printStats()
	{
		std::cout.precision(2);
		std::cout << "Statistics:\n";
		std::cout << "Ray triangle tests:                 " << std::setw(10) << std::scientific << rayTriTests.load() << '\n';
		std::cout << "Ray acceleration structure tests:   " << std::setw(10) << std::scientific << accelStructTests.load() << '\n';
		std::cout << "Total intersection test:            " << std::setw(10) << std::scientific
			<< (float)rayTriTests.load() + accelStructTests.load() << '\n';
		std::cout << "Total triangle copies:              " << std::setw(10) << triCopiesCount.load() << '\n';
		std::cout << "Total triangle count:               " << std::setw(10) << meshCount.load() << '\n';
		std::cout << "Acceleration structure count:       " << std::setw(10) << acCount.load() << '\n';
		

		rayTriTests.store(0);
		accelStructTests.store(0);
		triCopiesCount.store(0);
		meshCount.store(0);
		acCount.store(0);
	}
}
#endif
