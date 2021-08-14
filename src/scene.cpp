// classed describing scene and camera
#include "scene.h"

#include <thread>

#include "timer.h"

Camera::Camera(const Vec3f& a_pos, const Vec3f& a_rot)
	: pos(a_pos), rot(a_rot)
{
	const float& x = degToRad(rot.x);
	Matrix44f mx(
		1, 0, 0, 0,
		0, cosf(x), -sinf(x), 0,
		0, sinf(x), cosf(x), 0,
		0, 0, 0, 1
	);

	const float& y = degToRad(rot.y);
	Matrix44f my(
		cosf(y), 0, sinf(y), 0,
		0, 1, 0, 0,
		-sinf(y), 0, cosf(y), 0,
		0, 0, 0, 1
	);

	const float& z = degToRad(rot.z);
	Matrix44f mz(
		cosf(z), -sinf(z), 0, 0,
		sinf(z), cosf(z), 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	);

	rMatrix = mz * my * mx;
}

Ray Camera::getRay(const float xPix, const  float yPix)
{
	Vec3f dir = rMatrix.multVecMatrix(Vec3f(xPix, yPix, -1).normalize());
	return Ray{ this->pos , dir };
}

void Scene::renderWorker(Vec3f* frameBuffer, size_t y0, size_t y1)
{
#ifdef _DEBUG
#ifdef _PROGRESS_OUTPUT
	auto lastStat = std::chrono::high_resolution_clock::now();
#endif // _PROGRESS_OUTPUT
#endif // _DEBUG && _PROGRESS_OUTPUT

	const float scale = tanf(options.fov * 0.5f / 180.0f * M_PI);
	const float imageAspectRatio = (options.width) / (float)options.height;
	for (size_t y = y0; y < y1; y++) {
		for (size_t x = 0; x < options.width; x++) {
			float xPix = (2 * (x + 0.5f) / (float)options.width - 1) * scale * imageAspectRatio;
			float yPix = -(2 * (y + 0.5f) / (float)options.height - 1) * scale;
			Ray ray = this->camera.getRay(xPix, yPix);
			frameBuffer[x + y * options.width] = Renderer::castRay(ray, *this, 0);
			finishedPixels.store(finishedPixels.load() + 1);
		}
#ifdef _DEBUG
#ifdef _PROGRESS_OUTPUT
		if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - lastStat).count() >= 1000ll) {
			const float progressCoef = 100.0f / (options.width * options.height);
			if (y % 10 == 0) std::cout << std::fixed << std::setw(2) << std::setprecision(0) << progressCoef * finishedPixels.load() << "%\n";
			lastStat = std::chrono::high_resolution_clock::now();
		}
#endif // _PROGRESS_OUTPUT
#endif // _DEBUG && _PROGRESS_OUTPUT
	}
	finishedWorkers.store(finishedWorkers.load() + 1);
}

Scene::Scene(const std::string& sceneName)
{
	sceneLoadSuccess = loadScene(*this, this->options, sceneName);
}

int Scene::launchWorkers(Vec3f* frameBuffer)
{
	Timer t("Render scene");
#ifndef _DEBUG
	std::vector<std::unique_ptr<std::thread>> threadPool;
	for (size_t i = 0; i < options.nWorkers; i++) {
		size_t y0 = options.height / options.nWorkers * i;
		size_t y1 = options.height / options.nWorkers * (i + 1);
		if (i + 1 == options.nWorkers) y1 = options.height;
		threadPool.push_back(std::make_unique<std::thread>(&Scene::renderWorker, this, frameBuffer, y0, y1));
	}

#ifdef _PROGRESS_OUTPUT
	while (finishedWorkers.load() != options.nWorkers) {
		auto start = std::chrono::high_resolution_clock::now();
		for (int i = 0; i < 100; i++) {
			if (finishedWorkers.load() == options.nWorkers)
				break;
			if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count() >= 1000ll)
				break;
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count() >= 1000ll) {
			const float progressCoef = 100.0f / (options.width * options.height);
			std::cout << std::fixed << std::setw(2) << std::setprecision(0) << progressCoef * finishedPixels.load() << "%\n";
		}
	}
#endif // _PROGRESS_OUTPUT
	for (auto& thread : threadPool)
		thread->join();
#else
	renderWorker(frameBuffer, 0, options.height);
#endif
	return 0;
}

long long Scene::render()
{
	if (!sceneLoadSuccess) return -1;
	Timer t("Total time");
	Vec3f* frameBuffer = new Vec3f[options.height * options.width];

	launchWorkers(frameBuffer);

#ifdef _SHOW_AC
	const Vec3f orig = { 0, 0, 0 };
	const float scale = tanf(options.fov * 0.5f / 180.0f * M_PI);
	float imageAspectRatio = (options.width) / (float)options.height;

	int acMax = 0;
	int* acBuffer = new int[options.width * options.height];

	for (size_t y = 0; y < options.height; y++) {
		for (size_t x = 0; x < options.width; x++) {
			float xPix = (2 * (x + 0.5f) / (float)options.width - 1) * scale * imageAspectRatio;
			float yPix = -(2 * (y + 0.5f) / (float)options.height - 1) * scale;
			Ray ray = this->camera.getRay(xPix, yPix);
			int val = AccelerationStructure::countAC(ray, *this);
			if (val > acMax) acMax = val;
			acBuffer[x + y * options.width] = val;
		}
	}

	for (size_t y = 0; y < options.height; y++) {
		for (size_t x = 0; x < options.width; x++) {
			float val = acBuffer[x + y * options.width];
			val /= acMax;
			frameBuffer[x + y * options.width] = Vec3f{ val };
		}
	}
	delete[] acBuffer;
#endif // _SHOW_AC

#ifndef _NO_IMAGE_OUTPUT
	savePPM(frameBuffer, options);
#endif // _NO_OUTPUT
	delete[] frameBuffer;

#ifdef _STATS
	stats::printStats();
#endif // _STATS

#ifndef _NO_OUTPUT
	std::cout << '\n';
#endif // _NO_OUTPUT
	return t.stop();
}