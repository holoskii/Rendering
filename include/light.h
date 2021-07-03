#pragma once

enum class LightType { BaseLight, DistantLight, PointLight };

class Light
{
public:
    Light(const Vec3f& a_color = 1, const float& a_intensity = 1)
        : color(a_color), intensity(a_intensity) {}
    
    virtual ~Light() {}
    
    virtual void illuminate(const Vec3f& P, Vec3f&, Vec3f&, float&) const = 0;

    Vec3f color;
    float intensity;
    LightType type = LightType::BaseLight;
};

class DistantLight : public Light
{
public:
    DistantLight() 
        : Light(), dir(Vec3f{0, -1, 0}) 
    {
        type = LightType::DistantLight;
    }
    
    DistantLight(const Vec3f& a_dir, const Vec3f& a_color = 1, const float& a_intensity = 1)
        : Light(a_color, a_intensity), dir(a_dir) 
    {
        type = LightType::DistantLight;
        dir.normalize();
    }
    
    void illuminate(const Vec3f& P, Vec3f& lightDir, Vec3f& lightIntensity, float& distance) const 
    {
        lightDir = dir;
        lightIntensity = color * intensity;
        distance = std::numeric_limits<float>::max();
    }

    Vec3f dir;
};

class PointLight : public Light
{
public:
    PointLight(const Vec3f& a_pos, const Vec3f& a_color = 1, const float& a_intensity = 1)
        : Light(a_color, a_intensity), pos(a_pos) 
    {
        type = LightType::PointLight;
    }
    
    void illuminate(const Vec3f& P, Vec3f& lightDir, Vec3f& lightIntensity, float& distance) const 
    {
        lightDir = P - pos;
        lightIntensity = color * std::min(1.0, intensity / (4 * M_PI * lightDir.length2() / 1000));
        lightDir.normalize();
    }

    Vec3f pos;
};