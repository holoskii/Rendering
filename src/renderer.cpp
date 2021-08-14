#include "renderer.h"

#include <fstream>
#include <chrono>
#include <map>
#include <thread>

#include "stats.h"
#include "timer.h"
#include "options.h"
#include "util.h"


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



Vec3f Renderer::reflect(const Vec3f& dir, const Vec3f& normal)
{
	return dir - 2 * dir.dotProduct(normal) * normal;
}

Vec3f Renderer::refract(const Vec3f& dir, const Vec3f& normal, const float& indexOfRefraction)
{
	float n1 = 1, n2 = indexOfRefraction;   // n1 - air, n2 - object
	float cosi = clamp(-1, 1, dir.dotProduct(normal));
	Vec3f modNormal = normal;
	if (cosi < 0) {
		// outside the medium
		cosi = -cosi;
	}
	else {
		// inside the medium
		std::swap(n1, n2);
		modNormal = -normal;
	}
	float rri = n1 / n2; // relative refraction index
	float k = 1 - rri * rri * (1 - cosi * cosi); // critial angle test
	if (k < 0)
		return 0;
	return rri * dir + (rri * cosi - sqrtf(k)) * modNormal;
}

float Renderer::fresnel(const Vec3f& dir, const Vec3f& normal, const float& indexOfRefraction)
{
	float n1 = 1, n2 = indexOfRefraction;   // n1 - air, n2 - object
	float cosi = clamp(-1, 1, dir.dotProduct(normal));
	if (cosi > 0) {
		// outside the medium
		std::swap(n1, n2);
	}
	float sint = n1 / n2 * sqrtf(std::max(0.f, 1 - cosi * cosi));  // Snell's law

	float kr;
	if (sint >= 1) {
		// total internal reflection
		kr = 1;
	}
	else {
		// fresnell equation
		float cost = sqrtf(std::max(0.f, 1 - sint * sint));
		cosi = fabsf(cosi);
		float rs = ((n2 * cosi) - (n1 * cost)) / ((n2 * cosi) + (n1 * cost));
		float rp = ((n1 * cosi) - (n2 * cost)) / ((n1 * cosi) + (n2 * cost));
		kr = (rs * rs + rp * rp) / 2;
	}
	return kr;
}

bool Renderer::trace(const Ray& ray, const ObjectVector& objects, IntersectInfo& intrInfo)
{
	intrInfo.hitObject = nullptr;
	for (auto i = objects.begin(); i != objects.end(); i++) {
		// transparent objects do not cast shadows
		if (ray.rayType == RayType::ShadowRay && (*i).get()->materialType == MaterialType::Transparent)
			continue;
		float tNear = std::numeric_limits<float>::max();
		const Triangle* ptr = nullptr;
		Vec2f uv;

		if ((*i).get()->objectType == ObjectType::Mesh) {
			if (dynamic_cast<Mesh*>((*i).get())->intersectMesh(ray, tNear, ptr, uv) && tNear < intrInfo.tNear) {
				intrInfo.hitObject = (*i).get();
				intrInfo.tNear = tNear;
				intrInfo.triPtr = ptr;
				intrInfo.uv = uv;
			}
		}
		else {
			if ((*i).get()->intersectObject(ray.orig, ray.dir, tNear, uv) && tNear < intrInfo.tNear) {
				intrInfo.hitObject = (*i).get();
				intrInfo.tNear = tNear;
				intrInfo.uv = uv;
			}
		}
	}
	return (intrInfo.hitObject != nullptr);
}

Vec3f Renderer::castRay(const Ray& ray, const Scene& scene, const int depth)
{
	if (depth > scene.options.maxRayDepth) return scene.options.backgroundColor;
	IntersectInfo intrInfo;
	if (trace(ray, scene.objects, intrInfo)) {
		//return Vec3f{ 1.0f };
		/*if (intrInfo.hitObject->objectType == ObjectType::Mesh) {
			const Mesh* mesh = dynamic_cast<const Mesh*>(intrInfo.hitObject);
			AccelerationStructure ac = static_cast<Mesh*>()->accelStruct;
		}*/
		Vec3f hitColor = intrInfo.hitObject->color;
		Vec3f hitNormal;
		Vec2f hitTexCoordinates;
		Vec3f hitPoint = ray.orig + ray.dir * intrInfo.tNear;
		intrInfo.hitObject->getSurfaceData(hitPoint, intrInfo.triPtr, intrInfo.uv, hitNormal, hitTexCoordinates);

		Vec3f objectColor = intrInfo.hitObject->color;
		hitColor = { 0 };

		if (intrInfo.hitObject->materialType == MaterialType::Diffuse) {
			// for diffuse objects collect light from all visible sources
			for (size_t i = 0; i < scene.lights.size(); ++i) {
				IntersectInfo intrShadInfo;
				Vec3f lightDir, lightIntensity;
				scene.lights[i]->illuminate(hitPoint, lightDir, lightIntensity, intrShadInfo.tNear);
				bool vis = !trace(Ray{ hitPoint + hitNormal * scene.options.bias, -lightDir, RayType::ShadowRay }, scene.objects, intrShadInfo);
				float pattern = intrInfo.hitObject->getPattern(hitTexCoordinates);
				hitColor += (objectColor * lightIntensity) * pattern * vis * std::max(0.f, hitNormal.dotProduct(-lightDir));
			}
		}
		else if (intrInfo.hitObject->materialType == MaterialType::Reflective) {
			// get info from reflected ray
			Ray reflectedRay { hitPoint + scene.options.bias * hitNormal, ray.dir - 2 * ray.dir.dotProduct(hitNormal) * hitNormal };
			hitColor = 0.8f * castRay(reflectedRay, scene, depth + 1);
		}
		else if (intrInfo.hitObject->materialType == MaterialType::Transparent) {
			float kr = fresnel(ray.dir, hitNormal, intrInfo.hitObject->indexOfRefraction);
			bool outside = ray.dir.dotProduct(hitNormal) < 0;
			Vec3f biasVec = scene.options.bias * hitNormal;
			hitColor = { 0 };
			if (kr < 1) {
				// compute refraction if it is not a case of total internal reflection
				Vec3f refractionDirection = refract(ray.dir, hitNormal, intrInfo.hitObject->indexOfRefraction).normalize();
				Vec3f refractionRayOrig = outside ? hitPoint - biasVec : hitPoint + biasVec; // add bias
				Vec3f refractionColor = castRay(Ray{ refractionRayOrig, refractionDirection }, scene, depth + 1);
				hitColor += refractionColor * (1 - kr);
			}

			Vec3f reflectionDirection = reflect(ray.dir, hitNormal).normalize();
			Vec3f reflectionRayOrig = outside ? hitPoint + biasVec : hitPoint - biasVec;    // add bias
			Vec3f reflectionColor = castRay(Ray{ reflectionRayOrig, reflectionDirection }, scene, depth + 1);
			hitColor += reflectionColor * kr;
		}
		else if (intrInfo.hitObject->materialType == MaterialType::Phong) {
			Vec3f diffuseLight = 0, specularLight = 0;
			for (uint32_t i = 0; i < scene.lights.size(); ++i) {
				Vec3f lightDir, lightIntensity;
				IntersectInfo isectShad;
				scene.lights[i]->illuminate(hitPoint, lightDir, lightIntensity, isectShad.tNear);

				bool vis = !trace(Ray{ hitPoint + hitNormal * scene.options.bias, -lightDir, RayType::ShadowRay }, scene.objects, isectShad);
				// if (isectShad.tNear > lights[i])

				// compute the diffuse component
				diffuseLight += vis * intrInfo.hitObject->difuse * lightIntensity * std::max(0.f, hitNormal.dotProduct(-lightDir));

				// compute the specular component
				// what would be the ideal reflection direction for this light ray
				Vec3f R = reflect(lightDir, hitNormal);
				specularLight += vis * intrInfo.hitObject->specular * lightIntensity * std::pow(std::max(0.f, R.dotProduct(-ray.dir)), intrInfo.hitObject->nSpecular); // * intrInfo.hitObject->kReflect)
					// + (intrInfo.hitObject->color * (1 - intrInfo.hitObject->kReflect));
			}
			hitColor = objectColor * intrInfo.hitObject->ambient + diffuseLight + specularLight;
			hitColor = hitColor * intrInfo.hitObject->getPattern(hitTexCoordinates);
		}
		return hitColor;
	}
	return scene.options.backgroundColor;
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

int Scene::render()
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
			Vec3f dir = Vec3f(xPix, yPix, -1).normalize();
			ray.transform
			int val = interAC(orig, dir, objects, options);
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