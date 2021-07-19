#pragma once

#include "options.h"

#include <iostream>
#include <iomanip>
#include <vector>

void test_ac(const std::string& path)
{
	options::useGlobal_ac_max_depth = true;
	options::useGlobal_ac_min_batch = true;
	int render_trials = 1;
	int batch_sizes[] = { 10000, 3000, 1000, 300, 100, 30, 10, 3, 1 };
	int max_depth[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20 };
	std::vector<int> batch_sizes_vec(std::begin(batch_sizes), std::end(batch_sizes));
	std::vector<int> max_depth_vec(std::begin(max_depth), std::end(max_depth));

	std::vector<int> res_val;
	std::vector<int> res_time;
#if 0
	int count = 0;
	for (int val : batch_sizes_vec) {
		int result = 0;
		for (int i = 0; i < render_trials; i++) {
			std::cout << "Trial " << count++ << " out of " << batch_sizes_vec.size() * render_trials << ", batch " << val << '\n';
			options::globalOptions.maxDepthAccelStruct = 20;
			options::globalOptions.minBatchSizeAccelStruct = val;
			result += Scene(path).render();
		}
		result /= render_trials;
		res_val.push_back(val);
		res_time.push_back(result);
	}

	for (int i = 0; i < res_val.size(); i++) {
		std::cout << "Batch size : " << std::setw(4) << res_val.at(i) <<
			" Render time: " << std::setw(6) << res_time.at(i) << "ms\n";
	}
#else
	int count = 0;
	for (int val : max_depth_vec) {
		int result = 0;
		for (int i = 0; i < render_trials; i++) {
			std::cout << "Trial " << count++ << " out of " << max_depth_vec.size() * render_trials << '\n';
			options::globalOptions.maxDepthAccelStruct = val;
			options::globalOptions.minBatchSizeAccelStruct = 1;
			result += Scene(path).render();
		}
		result /= render_trials;
		res_val.push_back(val);
		res_time.push_back(result);
	}
	std::cout << '\n';
	for (int i = 0; i < res_val.size(); i++) {
		std::cout << "Max depth: " << std::setw(4) << res_val.at(i) <<
			" Render time: " << std::setw(6) << res_time.at(i) << "ms\n";
	}

	/*for (int val : res_val)
		std::cout << val << '\n';

	for (int time: res_time)
		std::cout << time << '\n';*/
#endif
}