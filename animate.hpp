#ifndef ANIMATE_HPP
#define ANIMATE_HPP

#include "raycast.hpp"


#define MAX_KEYFRAMES  64
#define MAX_ANIM_OBJS  64


struct Keyframe
{
    double time;
    Vec3 position;
    Vec3 c_diff;
    Vec3 normal;
    double radius;
};

struct AnimObject
{
    char name[64];
    ObjType type;
    int kf_count;
    Keyframe keyframes[MAX_KEYFRAMES];
};

struct Animation
{
    int total_frames;
    double fps;
    int img_width;
    int img_height;

    int obj_count;
    AnimObject objects[MAX_ANIM_OBJS];
    Object camera;
    int has_camera;
};

int load_animation(const char *filename, Animation *anim);

void eval_animation(const Animation *anim, double t, Object *out_objects, int *out_count, Object *out_camera);

int render_animation(const Animation *anim, const char *out_dir, const char *prefix);

#endif