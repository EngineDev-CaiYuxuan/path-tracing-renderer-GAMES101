//
// Created by Göksu Güvendiren on 2019-05-14.
//

#include "Scene.hpp"
//#include "Triangle.hpp"//调试：强行接入子类//  !!!OBJ_Loader头文件被重复包含 内部实现重复定义!!!


void Scene::buildBVH() {
    printf(" - Generating BVH...\n\n");
    this->bvh = new BVHAccel(objects, 1, BVHAccel::SplitMethod::NAIVE);
}

Intersection Scene::intersect(const Ray &ray) const
{
    return this->bvh->Intersect(ray);
}

void Scene::sampleLight(Intersection &pos, float &pdf) const
{
    float emit_area_sum = 0;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        if (objects[k]->hasEmit()){
            emit_area_sum += objects[k]->getArea();
        }
    }
    float p = get_random_float() * emit_area_sum;
    emit_area_sum = 0;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        if (objects[k]->hasEmit()){
            emit_area_sum += objects[k]->getArea();
            if (p <= emit_area_sum){
                objects[k]->Sample(pos, pdf);
                break;
            }
        }
    }
}

bool Scene::trace(
        const Ray &ray,
        const std::vector<Object*> &objects,
        float &tNear, uint32_t &index, Object **hitObject)
{
    *hitObject = nullptr;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        float tNearK = kInfinity;
        uint32_t indexK;
        Vector2f uvK;
        if (objects[k]->intersect(ray, tNearK, indexK) && tNearK < tNear) {
            *hitObject = objects[k];
            tNear = tNearK;
            index = indexK;
        }
    }


    return (*hitObject != nullptr);
}

// Implementation of Path Tracing
Vector3f Scene::castRay(const Ray &ray, int depth) const
{
    // TO DO Implement Path Tracing Algorithm here

    Vector3f ref_color(0.f);
    Intersection inter = this->intersect(ray);
    if (!inter.happened)
    {
        return ref_color;
    }
    //std::cout << "相交" << std::endl;

    if ((inter.emit).norm() > EPSILON)
    {
        ref_color += inter.m->getEmission();
    }

    ref_color += compute_directlighting(Ray(ray), inter);

    if (depth >= 6 || get_random_float() > this->RussianRoulette)
    {
        return ref_color;
    }

    Vector3f out_dir = inter.m->sample(normalize(ray.direction), normalize(inter.normal));
    float pdf = inter.m->pdf(normalize(ray.direction), normalize(out_dir), normalize(inter.normal));
    Vector3f f_r = inter.m->eval(normalize(ray.direction), normalize(out_dir), normalize(inter.normal));
    float cos_alpha = dotProduct(normalize(out_dir), normalize(inter.normal));

    Vector3f L_indir = castRay(Ray(inter.coords + out_dir * EPSILON, out_dir), ++depth)
                       * cos_alpha * f_r / (pdf * RussianRoulette);
    ref_color += L_indir;

    return ref_color;
}


Vector3f Scene::compute_directlighting(Ray& ray, Intersection& inter) const
{
    Intersection light_pos;
    float pdf_light;
    sampleLight(light_pos, pdf_light);

    Vector3f light_dir(light_pos.coords - inter.coords);
    Ray shadow_ray(inter.coords + light_dir * EPSILON, light_dir);
    Intersection shadow_inter = intersect(shadow_ray);
    if (shadow_inter.happened && ((shadow_inter.coords - light_pos.coords).norm() > EPSILON))
    {
        return Vector3f(0);
    }
    
    float dist_sq = dotProduct(light_pos.coords - inter.coords, light_pos.coords - inter.coords);
    Vector3f L_i = light_pos.emit;
    Vector3f f_r = inter.m->eval(normalize(-ray.direction), normalize(light_dir), normalize(inter.normal));
    float cos_alpha = dotProduct(normalize(inter.normal), normalize(light_dir));
    float cos_theta = dotProduct(normalize(-light_pos.normal), normalize(light_pos.normal));

    Vector3f directlighting = L_i * f_r * cos_alpha * cos_theta / dist_sq / pdf_light;

    return directlighting;
}