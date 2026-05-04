#ifndef RAYCAST_HPP
#define RAYCAST_HPP

#include "ppm.hpp"

struct Vec3 
{
    double x, y, z;
};

Vec3 vec3(double x, double y, double z);
Vec3 vec3_sub(Vec3 a, Vec3 b);
double vec3_dot(Vec3 a, Vec3 b);
double vec3_length(Vec3 a);
Vec3 vec3_normalize(Vec3 a);

typedef enum 
{
    OBJ_CAMERA,
    OBJ_SPHERE,
    OBJ_PLANE
} ObjType;

typedef struct 
{
    ObjType type;
    Vec3 c_diff;      
    Vec3 position;    
    Vec3 normal;      
    double radius;      
    double cam_width;   
    double cam_height;  
} Object;

#define MAX_OBJECTS 128


double intersect_sphere(Vec3 origin, Vec3 dir, const Object *s);

double intersect_plane(Vec3 origin, Vec3 dir, const Object *p);

int load_scene(const char *filename, Object *objects, int *count,
               Object **camera);

uint8_t float_to_u8(double v);

#endif