// classed describing scene and camera
#include "scene.h"

#include <thread>
#include <map>
#include <fstream>

#include "timer.h"
#include "util.h"
#include "options.h"
#include "stats.h"

Camera::Camera(const Vec3f& a_pos, const Vec3f& a_rot)
	: pos(a_pos), rot(a_rot) {}

Ray Camera::getRay(const float xPix, const  float yPix)
{
	if (!cameraRotated) {
		cameraRotated = true;
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

	Vec3f dir = rMatrix.multVecMatrix(Vec3f(xPix, yPix, -1).normalize());
	return Ray{ this->pos , dir };
}


Scene::Scene(const std::string& sceneName)
{
	sceneLoadSuccess = this->loadScene(sceneName);
}

bool Scene::loadScene(const std::string& sceneName)
{
	if (options::enableOutput) {
		std::cout << "Loading scene " << sceneName << '\n';
	}

    enum class BlockType { None, Options, Light, Object };
    std::map<std::string, BlockType> blockMap;
    blockMap["[options]"] = BlockType::Options;
    blockMap["[light]"] = BlockType::Light;
    blockMap["[object]"] = BlockType::Object;
    blockMap["[end]"] = BlockType::None;
    BlockType blockType = BlockType::None;

    std::string scenePath = options.rootPath + "\\" + sceneName;
    std::string str;
    std::ifstream ifs(scenePath, std::ifstream::in);

	if (!ifs.good()) LOG_ERROR

    Light* light = nullptr;
    Object* object = nullptr;

    while (ifs.good()) {
        std::getline(ifs, str);
        if (str.length() == 0)
            continue;

        // finish previous block
        if (strContains(str, "[")) {
            if (blockType == BlockType::Light) {
				if (light == nullptr)
					LOG_ERROR
                lights.push_back(std::unique_ptr<Light>(light));
            }
            else if (blockType == BlockType::Object) {
				if (object == nullptr)
					LOG_ERROR
                objects.push_back(std::unique_ptr<Object>(object));
            }
        }

        // skip commented block
        if (strContains(str, "#[")) {
            do {
                std::getline(ifs, str);
            } while (!strContains(str, "[") || strContains(str, "#["));
            if (!ifs.good())
                break;
        }

        // skip comments
        if (strContains(str, "#"))
            str.erase(str.find('#'));
        if (str.length() == 0)
            continue;

        // select block
        if (str[0] == '[') {
			if (blockMap.find(str) == blockMap.end()) LOG_ERROR
            blockType = blockMap[str];
            if (blockType == BlockType::None)
                break;
            continue;
        }

        // parse the line
        if (blockType == BlockType::Options) {
			if (!strContains(str, "=")) LOG_ERROR
            std::string_view key(str.c_str(), str.find('='));
            std::string_view value(str.c_str() + str.find('=') + 1);

            if (strEquals(key, "width"))
                options.width = strToInt(value);
            else if (strEquals(key, "height"))
                options.height = strToInt(value);
            else if (strEquals(key, "fov"))
                camera.fov = strToFloat(value);
            else if (strEquals(key, "image_name"))
                options.imageName = std::string(value);
            else if (strEquals(key, "n_workers="))
                options.nWorkers = strToInt(value);
            else if (strEquals(key, "max_ray_depth"))
                options.maxRayDepth = strToInt(value);
            else if (strEquals(key, "ac_penalty"))
                options.acPenalty = strToInt(value);
            else if (strEquals(key, "background_color"))
                options.backgroundColor = str3ToFloat(splitString(value, ','));
            else if (strEquals(key, "position"))
                camera.pos = str3ToFloat(splitString(value, ','));
            else if (strEquals(key, "rotation"))
                camera.rot = str3ToFloat(splitString(value, ','));
			else if (strEquals(key, "skyboxes")) {
				auto res = splitString(value, ',');
				if (res.size() < 6) LOG_ERROR
				for (int i = 0; i < 6; i++) {
					strncpy(options.names[i], res[i].c_str(), 64);
				}
			}
        }
        else if (blockType == BlockType::Light) {
			if (!strContains(str, "=")) LOG_ERROR
            std::string_view key(str.c_str(), str.find('='));
            std::string_view value(str.c_str() + str.find('=') + 1);

            if (strEquals(key, "type")) {
                if (strEquals(value, "distant"))
                    light = new DistantLight();
                else if (strEquals(value, "point"))
                    light = new PointLight();
				else if (strEquals(value, "area"))
					light = new AreaLight();
            }
            else if (light == nullptr)
                std::cout << "Error, light type missing\n";
            else if (strEquals(key, "color"))
                light->color = str3ToFloat(splitString(value, ','));
            else if (strEquals(key, "intensity"))
                light->intensity = strToFloat(value);
            if (strEquals(key, "direction")) {
				if (light->type != LightType::DistantLight) LOG_ERROR
                static_cast<DistantLight*>(light)->dir = str3ToFloat(splitString(value, ','));
            }
            else if (strEquals(key, "position")) {
				if (light->type != LightType::PointLight) LOG_ERROR
                static_cast<PointLight*>(light)->pos = str3ToFloat(splitString(value, ','));
            }
			else if (strEquals(key, "pos")) {
				if (light->type != LightType::AreaLight) LOG_ERROR
				static_cast<AreaLight*>(light)->pos = str3ToFloat(splitString(value, ','));
			}
			else if (strEquals(key, "i")) {
				if (light->type != LightType::AreaLight) LOG_ERROR
				static_cast<AreaLight*>(light)->i = str3ToFloat(splitString(value, ','));
			}
			else if (strEquals(key, "j")) {
				if (light->type != LightType::AreaLight) LOG_ERROR
				static_cast<AreaLight*>(light)->j = str3ToFloat(splitString(value, ','));
			}
			else if (strEquals(key, "samples")) {
				if (light->type != LightType::AreaLight) LOG_ERROR
				static_cast<AreaLight*>(light)->samples = strToInt(value);
			}
        }
        else if (blockType == BlockType::Object) {
           if (!strContains(str, "=")) LOG_ERROR
            std::string_view key(str.c_str(), str.find('='));
            std::string_view value(str.c_str() + str.find('=') + 1);

            if (strEquals(key, "type")) {
                if (strEquals(value, "plane"))
                    object = new Plane();
                else if (strEquals(value, "sphere"))
                    object = new Sphere();
                else if (strEquals(value, "mesh"))
                    object = new Mesh();
            }
            else if (object == nullptr) {
                std::cout << "Error, object type missing\n";
            }
            else if (strEquals(key, "color")) {
                object->color = str3ToFloat(splitString(value, ','));
            }
            else if (strEquals(key, "pos")) {
                object->pos = str3ToFloat(splitString(value, ','));
            }
            else if (strEquals(key, "material")) {
                auto res = splitString(value, ',');
                if (strEquals(res[0], "transparent")) {
                    object->materialType = MaterialType::Transparent;
					object->indexOfRefraction = strToFloat(res[1]);
                }
                else if (strEquals(res[0], "reflective")) {
                    object->materialType = MaterialType::Reflective;
                }
                if (strEquals(res[0], "phong")) {
                    object->materialType = MaterialType::Phong;
                    object->ambient = strToFloat(res[1]);
                    object->difuse = strToFloat(res[2]);
                    object->specular = strToFloat(res[3]);
                    object->nSpecular = strToFloat(res[4]);
                }
            }
            else if (object->objectType == ObjectType::Sphere) {
                Sphere* sphere = static_cast<Sphere*>(object);
                if (strEquals(key, "radius")) {
                    sphere->r = strToFloat(value);
                    sphere->r2 = powf(sphere->r, 2);
                }
            }
            else if (object->objectType == ObjectType::Plane) {
                Plane* plane = static_cast<Plane*>(object);
                if (strEquals(key, "normal")) {
                    plane->normal = str3ToFloat(splitString(value, ','));
                }
            }
            else if (object->objectType == ObjectType::Mesh) {
                Mesh* mesh = static_cast<Mesh*>(object);
                if (strEquals(key, "size")) {
                    mesh->size = str3ToFloat(splitString(value, ','));
                }
                else if (strEquals(key, "rot")) {
                    mesh->rot = str3ToFloat(splitString(value, ','));
                }
                else if (strEquals(key, "name")) {
                    mesh->loadOBJ(std::string(value), options);
                }
            }
        }
    }

    ifs.close();

	if (options::useSkybox) {
		loadSkybox();
	}
    return true;
}

void Scene::loadSkybox()
{
	if (options::useSkybox) {
		unsigned char* u_skyboxes[6];
		int width, height;
		for (int i = 0; i < 6; i++) {
			u_skyboxes[i] = loadBMP((options.rootPath + std::string("\\") + std::string(options.names[i])).c_str(), width, height);
		}
		skyboxHeight = height;
		skyboxWidth = width;

		for (int k = 0; k < 6; k++) {
			skyboxes[k] = new Vec3f[width * (size_t)height];
			for (int i = 0; i < height; i++) {
				for (int j = 0; j < width; j++) {
					int v = 3 * (i * width + j);
					float x = u_skyboxes[k][v], y = u_skyboxes[k][v + 1], z = u_skyboxes[k][v + 2];
					x /= 256; y /= 256; z /= 256;
					skyboxes[k][i * width + j] = Vec3f{ x, y, z };
				}
			}
		}
	}
}

Vec3f Scene::getSkybox(const Vec3f& dir) const
{
	if (!options::useSkybox) {
		return options.backgroundColor;
	}

	auto toPixel = [](const float& v, const int& max)
	{
		int val = (int)((v + 1.0f) / 2.0f * max);
		if (val >= max) val = max - 1;
		return val;
	};

	Vec3f adir = dir;
	float max = std::max(fabs(adir.x), std::max(fabs(adir.y), fabs(adir.z)));
	if (max == fabs(adir.z)) {
		if (adir.z < 0) {
			adir = dir * (1 / -dir.z);
			int i = toPixel(adir.y, skyboxHeight);
			int j = toPixel(adir.x, skyboxWidth);
			return skyboxes[1][i * skyboxWidth + j];
		}
		else {
			adir = dir * (1 / dir.z);
			int i = toPixel(adir.y, skyboxHeight);
			int j = toPixel(-adir.x, skyboxWidth);
			return skyboxes[3][i * skyboxWidth + j];
		}
	}
	else if (max == fabs(adir.x)) {
		if (adir.x < 0) {
			adir = dir * (1 / -dir.x);
			int i = toPixel(adir.y, skyboxHeight);
			int j = toPixel(-adir.z, skyboxWidth);
			return skyboxes[0][i * skyboxWidth + j];
		}
		else {
			adir = dir * (1 / dir.x);
			int i = toPixel(adir.y, skyboxHeight);
			int j = toPixel(adir.z, skyboxWidth);
			return skyboxes[2][i * skyboxWidth + j];
		}
	}
	else {
		if (adir.y < 0) {
			adir = dir * (1 / -dir.y);
			int i = toPixel(adir.z, skyboxHeight);
			int j = toPixel(adir.x, skyboxWidth);
			return skyboxes[5][i * skyboxWidth + j];
		}
		else {
			adir = dir * (1 / dir.y);
			int i = toPixel(adir.z, skyboxHeight);
			int j = toPixel(adir.x, skyboxWidth);
			return skyboxes[4][i * skyboxWidth + j];
		}
	}

	return options.backgroundColor;
}

void Scene::renderWorker(Vec3f* frameBuffer, size_t y0, size_t y1)
{
#ifdef _DEBUG
	auto lastStat = std::chrono::high_resolution_clock::now();
#endif // _DEBUG

	const float scale = tanf(camera.fov * 0.5f / 180.0f * (float)(M_PI));
	const float imageAspectRatio = (options.width) / (float)options.height;
	for (size_t y = y0; y < y1; y++) {
		for (size_t x = 0; x < options.width; x++) {
			/*if (x == 165 && y == 115) {
				frameBuffer[x + y * options.width] = { 1,0,0 };
				continue;
			}*/
			float xPix = (2 * (x + 0.5f) / (float)options.width - 1) * scale * imageAspectRatio;
			float yPix = -(2 * (y + 0.5f) / (float)options.height - 1) * scale;
			Ray ray = this->camera.getRay(xPix, yPix);
			frameBuffer[x + y * options.width] = Render::castRay(ray, *this, 0);
			finishedPixels.store(finishedPixels.load() + 1);
		}
#ifdef _DEBUG
		// in debug mode we only have one thread, so we need to check for progress updates manually
		if (options::outputProgress) {
			if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - lastStat).count() >= 1000ll) {
				const float progressCoef = 100.0f / (options.width * options.height);
				if (y % 10 == 0) std::cout << std::fixed << std::setw(2) << std::setprecision(0) << progressCoef * finishedPixels.load() << "%\n";
				lastStat = std::chrono::high_resolution_clock::now();
			}
		}
#endif // _DEBUG
	}
	finishedWorkers.store(finishedWorkers.load() + 1);
}

int Scene::launchWorkers(Vec3f* frameBuffer)
{
	Timer t("Render scene");

#ifdef _DEBUG
	// in debug mode we do not create any additional threads
	renderWorker(frameBuffer, 0, options.height);
#else
	std::vector<std::unique_ptr<std::thread>> threadPool;
	for (size_t i = 0; i < options.nWorkers; i++) {
		size_t y0 = options.height / options.nWorkers * i;
		size_t y1 = options.height / options.nWorkers * (i + 1);
		if (i + 1 == options.nWorkers) y1 = options.height;
		threadPool.push_back(std::make_unique<std::thread>(&Scene::renderWorker, this, frameBuffer, y0, y1));
	}

	if (options::outputProgress) {
		// until all workers finish we will print progress
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
	}

	for (auto& thread : threadPool)
		thread->join();

#endif
	return 0;
}

long long Scene::render()
{
	if (!sceneLoadSuccess) return -1;
	Timer t("Total time");
	Vec3f* frameBuffer = new Vec3f[options.height * options.width];

	if (options::showAC) {
		const Vec3f orig = { 0, 0, 0 };
		const float scale = tanf(camera.fov * 0.5f / 180.0f * (float)(M_PI));
		float imageAspectRatio = (options.width) / (float)options.height;

		int acMax = 0;
		int* acBuffer = new int[options.width * options.height];

		for (size_t y = 0; y < options.height; y++) {
			for (size_t x = 0; x < options.width; x++) {
				float xPix = (2 * (x + 0.5f) / (float)options.width - 1) * scale * imageAspectRatio;
				float yPix = -(2 * (y + 0.5f) / (float)options.height - 1) * scale;
				Ray ray = this->camera.getRay(xPix, yPix);
				int val = countAC(ray);
				if (val > acMax) acMax = val;
				acBuffer[x + y * options.width] = val;
			}
		}

		for (size_t y = 0; y < options.height; y++) {
			for (size_t x = 0; x < options.width; x++) {
				float val = (float)acBuffer[x + y * options.width];
				val /= acMax;
				frameBuffer[x + y * options.width] = Vec3f{ val };
			}
		}
		delete[] acBuffer;
	}
	else {
		launchWorkers(frameBuffer);
	}

	if (options::imageOutput) {
		saveImage(frameBuffer, options);
	}

	delete[] frameBuffer;

	if (options::useSkybox) {
		for (int k = 0; k < 6; k++)
			if (skyboxes[k])
				delete[] skyboxes[k];
	}

	if (options::collectStatistics) {
		stats::printStats();
	}

	if (options::enableOutput) {
		std::cout << '\n';
	}
	return t.stop();
}

int Scene::countAC(const Ray& ray)
{
	int sum = 0;
	for (auto& obj : objects) {
		if (obj->objectType == ObjectType::Mesh) {
			Mesh* mesh = dynamic_cast<Mesh*>(obj.get());
			sum += mesh->ac.get()->recCountAC(ray);
		}
	}
	return sum;
}


Vec3f Render::reflect(const Vec3f& dir, const Vec3f& normal)
{
	return dir - 2 * dir.dotProduct(normal) * normal;
}

Vec3f Render::refract(const Vec3f& dir, const Vec3f& normal, const float& indexOfRefraction)
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

float Render::fresnel(const Vec3f& dir, const Vec3f& normal, const float& indexOfRefraction)
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

bool Render::trace(const Ray& ray, const ObjectVector& objects, const LightsVector& lights, IntersectInfo& intrInfo)
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
			if ((*i).get()->intersectObject(ray, tNear, uv) && tNear < intrInfo.tNear) {
				intrInfo.hitObject = (*i).get();
				intrInfo.tNear = tNear;
				intrInfo.uv = uv;
			}
		}
	}
	return (intrInfo.hitObject != nullptr);
}

Vec3f Render::castRay(const Ray& ray, const Scene& scene, const int depth)
{
	if (depth > scene.options.maxRayDepth) return scene.getSkybox(ray.dir);
	if (options::collectStatistics) {
		stats::raysCasted.store(stats::raysCasted.load() + 1);
	}
	IntersectInfo intrInfo;
	if (trace(ray, scene.objects, scene.lights, intrInfo)) {
		Vec2f hitTexCoordinates;
		Vec3f hitNormal, hitColor = { 0 };
		// get point coordinate and normal
		Vec3f hitPoint = ray.orig + ray.dir * intrInfo.tNear;
		intrInfo.hitObject->getSurfaceData(hitPoint, intrInfo.triPtr, intrInfo.uv, hitNormal, hitTexCoordinates);

		Vec3f diffuseComponent = 0, specularComponent = 0;
		IntersectInfo intrShadInfo;
		Vec3f lightDir, lightIntensity;
		if (intrInfo.hitObject->materialType == MaterialType::Diffuse) {
			// for diffuse objects collect light from all visible sources
			for (size_t i = 0; i < scene.lights.size(); ++i) {
				if (scene.lights[i]->type != LightType::AreaLight) {
					// get light direction, intensity and distance
					scene.lights[i]->illuminate(hitPoint, lightDir, lightIntensity, intrShadInfo.tNear);
					// check that light source is visible
					bool vis = !trace(Ray{ hitPoint + hitNormal * scene.options.bias, -lightDir, RayType::ShadowRay }, scene.objects, scene.lights, intrShadInfo);
					diffuseComponent += lightIntensity * (vis * std::max(0.f, hitNormal.dotProduct(-lightDir)));
				}
				else {
					// area light requires different routine
					AreaLight* light = dynamic_cast<AreaLight*>(scene.lights[i].get());

					light->setPoints();
					float diffuseSum = 0;
					lightIntensity = light->color * std::min(1.0f, (float)(light->intensity / (4 * M_PI * (hitPoint - light->pos).length2() / 1000)));

					// add all light samples
					for (const auto& p : light->points) {
						lightDir = hitPoint - p;
						intrShadInfo.tNear = lightDir.length();
						bool vis = !Render::trace(Ray{ hitPoint + hitNormal * scene.options.bias, -lightDir.normalize(), RayType::ShadowRay }, scene.objects, scene.lights, intrShadInfo);
						diffuseSum += vis * std::max(0.f, hitNormal.dotProduct(-lightDir));
					}
					diffuseComponent += diffuseSum / light->points.size() * lightIntensity;
				}
			}
			hitColor = intrInfo.hitObject->color * diffuseComponent;
		}
		else if (intrInfo.hitObject->materialType == MaterialType::Phong) {
			// for Phong object we will combine colors of object color, difuse and specular
			for (uint32_t i = 0; i < scene.lights.size(); ++i) {
				if (scene.lights[i]->type != LightType::AreaLight) {
					// get light direction, intensity and distance
					scene.lights[i]->illuminate(hitPoint, lightDir, lightIntensity, intrShadInfo.tNear);
					// check that light source is visible
					bool vis = !trace(Ray{ hitPoint + hitNormal * scene.options.bias, -lightDir, RayType::ShadowRay }, scene.objects, scene.lights, intrShadInfo);

					// compute the diffuse component
					diffuseComponent += vis * lightIntensity * std::max(0.f, hitNormal.dotProduct(-lightDir));

					// compute the specular component
					Vec3f reflectedRay = reflect(lightDir, hitNormal);
					specularComponent += vis * lightIntensity * std::pow(std::max(0.f, reflectedRay.dotProduct(-ray.dir)), intrInfo.hitObject->nSpecular);
				}
				else {
					// area light requires different routine
					AreaLight* light = dynamic_cast<AreaLight*>(scene.lights[i].get());

					light->setPoints();
					float areaIntensity = 0;
					lightIntensity = light->color * std::min(1.0f, (float)(light->intensity / (4 * M_PI * (hitPoint - light->pos).length2() / 1000)));
					
					float specularSum = 0;
					float diffuseSum = 0;
					// add all light samples
					for (const auto& p : light->points) {
						lightDir = hitPoint - p;
						intrShadInfo.tNear = lightDir.length();
						bool vis = !trace(Ray{ hitPoint + hitNormal * scene.options.bias, -lightDir.normalize(), RayType::ShadowRay }, scene.objects, scene.lights, intrShadInfo);
						diffuseSum += vis * std::max(0.f, hitNormal.dotProduct(-lightDir));
						Vec3f reflectedRay = reflect(lightDir, hitNormal);
						specularSum += vis * std::max(0.f, reflectedRay.dotProduct(-ray.dir));
					}
					diffuseComponent += diffuseSum / light->points.size() * lightIntensity;
					specularComponent += std::pow(specularSum / light->points.size(), intrInfo.hitObject->nSpecular) * lightIntensity;
				}
			}
			hitColor = intrInfo.hitObject->color * intrInfo.hitObject->ambient + diffuseComponent * intrInfo.hitObject->difuse + specularComponent * intrInfo.hitObject->specular;
		}
		else if (intrInfo.hitObject->materialType == MaterialType::Reflective) {
			// get info from reflected ray
			Ray reflectedRay{ hitPoint + scene.options.bias * hitNormal, ray.dir - 2 * ray.dir.dotProduct(hitNormal) * hitNormal };

			hitColor = 0.8f * castRay(reflectedRay, scene, depth + 1);

			specularComponent = 0;
			for (uint32_t i = 0; i < scene.lights.size(); ++i) {
				if (scene.lights[i]->type != LightType::AreaLight) {
					scene.lights[i]->illuminate(hitPoint, lightDir, lightIntensity, intrShadInfo.tNear);
					bool vis = !trace(Ray{ hitPoint + hitNormal * scene.options.bias, -lightDir, RayType::ShadowRay }, scene.objects, scene.lights, intrShadInfo);
					Vec3f reflectedRay = reflect(lightDir, hitNormal);
					specularComponent += vis * lightIntensity * std::pow(std::max(0.f, reflectedRay.dotProduct(-ray.dir)), intrInfo.hitObject->nSpecular);
				}
				else {
					// area light requires different routine
					AreaLight* light = dynamic_cast<AreaLight*>(scene.lights[i].get());

					light->setPoints();
					float areaIntensity = 0;
					lightIntensity = light->color * std::min(1.0f, (float)(light->intensity / (4 * M_PI * (hitPoint - light->pos).length2() / 1000)));

					float specularSum = 0;
					float diffuseSum = 0;
					// add all light samples
					for (const auto& p : light->points) {
						lightDir = hitPoint - p;
						intrShadInfo.tNear = lightDir.length();
						bool vis = !trace(Ray{ hitPoint + hitNormal * scene.options.bias, -lightDir.normalize(), RayType::ShadowRay }, scene.objects, scene.lights, intrShadInfo);
						Vec3f reflectedRay = reflect(lightDir, hitNormal);
						specularSum += vis * std::max(0.f, reflectedRay.dotProduct(-ray.dir));
					}
					specularComponent += std::pow(specularSum / light->points.size(), intrInfo.hitObject->nSpecular) * lightIntensity;
				}
			}
			hitColor += specularComponent;
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
		
		return hitColor;
	}
	return scene.getSkybox(ray.dir);
}
