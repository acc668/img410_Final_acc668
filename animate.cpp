#include "animate.hpp"
#include "ppm.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <sys/stat.h>


static Vec3 av3(double x, double y, double z) 
{ 
    Vec3 v={x,y,z}; 
    
    return v; 
}


static Vec3 av3_sub(Vec3 a, Vec3 b) 
{ 
    return av3(a.x-b.x,a.y-b.y,a.z-b.z); 
}


static double av3_dot(Vec3 a, Vec3 b) 
{ 
    return a.x*b.x+a.y*b.y+a.z*b.z; 
}


static double av3_len(Vec3 a) 
{ 
    return sqrt(a.x*a.x+a.y*a.y+a.z*a.z); 
}


static Vec3 av3_norm(Vec3 a)
{
    double l = av3_len(a);

    if (l < 1e-10) 
    {
        return av3(0,0,0);
    }

    return av3(a.x/l, a.y/l, a.z/l);
}


static void rtrim(char *s)
{
    int i = (int)strlen(s) - 1;

    while (i >= 0 && (s[i] == ' ' || s[i] == '\t' || s[i] == '\r' || s[i] == '\n' || s[i] == ';'))
    {
        s[i--] = '\0';
    }
}


static double catmull_rom(double p0, double p1, double p2, double p3, double u)
{
    double u2 = u * u;
    double u3 = u2 * u;

    return 0.5 * ((-u3 + 2.0*u2 - u) * p0 + ( 3.0*u3 - 5.0*u2 + 2.0 ) * p1 + (-3.0*u3 + 4.0*u2 + u) * p2 + (u3 - u2) * p3);
}


static Vec3 catmull_rom_vec3(Vec3 p0, Vec3 p1, Vec3 p2, Vec3 p3, double u)
{
    Vec3 result;

    result.x = catmull_rom(p0.x, p1.x, p2.x, p3.x, u);
    result.y = catmull_rom(p0.y, p1.y, p2.y, p3.y, u);
    result.z = catmull_rom(p0.z, p1.z, p2.z, p3.z, u);

    return result;
}


static void eval_anim_object(const AnimObject *ao, double t, Object *out)
{
    memset(out, 0, sizeof(Object));

    out->type = ao->type;

    int n = ao->kf_count;

    if (n == 0) 
    {
        return;
    }

    if (n == 1)
    {
        out->position = ao->keyframes[0].position;
        out->c_diff = ao->keyframes[0].c_diff;
        out->radius = ao->keyframes[0].radius;

        return;
    }

    if (t <= ao->keyframes[0].time)
    {
        out->position = ao->keyframes[0].position;
        out->c_diff = ao->keyframes[0].c_diff;
        out->radius = ao->keyframes[0].radius;
        
        return;
    }

    if (t >= ao->keyframes[n - 1].time)
    {
        out->position = ao->keyframes[n - 1].position;
        out->c_diff = ao->keyframes[n - 1].c_diff;
        out->radius = ao->keyframes[n - 1].radius;

        return;
    }

    int i1 = 0;

    for (int i = 0; i < n - 1; i++)
    {
        if (t >= ao->keyframes[i].time && t < ao->keyframes[i + 1].time)
        {
            i1 = i;

            break;
        }
    }

    int i2 = i1 + 1;
    int i0 = (i1 > 0) ? i1 - 1 : i1;
    int i3 = (i2 < n - 1) ? i2 + 1 : i2;

    double dt = ao->keyframes[i2].time - ao->keyframes[i1].time;
    double u = (dt > 1e-12) ? (t - ao->keyframes[i1].time) / dt : 0.0;

    const Keyframe *kf0 = &ao->keyframes[i0];
    const Keyframe *kf1 = &ao->keyframes[i1];
    const Keyframe *kf2 = &ao->keyframes[i2];
    const Keyframe *kf3 = &ao->keyframes[i3];

    out->position = catmull_rom_vec3(kf0->position, kf1->position, kf2->position, kf3->position, u);
    out->c_diff = catmull_rom_vec3(kf0->c_diff, kf1->c_diff, kf2->c_diff, kf3->c_diff, u);
    out->radius = catmull_rom(kf0->radius, kf1->radius, kf2->radius, kf3->radius, u);

    if (out->c_diff.x < 0.0) 
    {
        out->c_diff.x = 0.0;
    }

    if (out->c_diff.y < 0.0) 
    {
        out->c_diff.y = 0.0;
    }

    if (out->c_diff.z < 0.0) 
    {
        out->c_diff.z = 0.0;
    }

    if (out->c_diff.x > 1.0) 
    {
        out->c_diff.x = 1.0;
    }

    if (out->c_diff.y > 1.0) 
    {
        out->c_diff.y = 1.0;
    }

    if (out->c_diff.z > 1.0) 
    {
        out->c_diff.z = 1.0;
    }

    out->normal = ao->keyframes[0].normal;
}


int load_animation(const char *filename, Animation *anim)
{
    if (!anim) 
    {
        return 0;
    }

    memset(anim, 0, sizeof(Animation));

    FILE *fp = fopen(filename, "r");

    if (!fp)
    {
        fprintf(stderr, "Error: cannot open animation file '%s'\n", filename);

        return 0;
    }

    char line[1024];

    if (!fgets(line, sizeof(line), fp))
    {
        fclose(fp);

        return 0;
    }

    rtrim(line);

    if (strcmp(line, "img410anim") != 0)
    {
        fprintf(stderr, "Error: missing 'img410anim' header in '%s'\n", filename);

        fclose(fp);

        return 0;
    }

    anim->total_frames = 30;

    anim->fps = 24.0;

    anim->img_width = 400;

    anim->img_height = 300;

    AnimObject *cur_obj = NULL;

    while (fgets(line, sizeof(line), fp))
    {
        rtrim(line);

        if (strlen(line) == 0 || line[0] == '#') 
        {
            continue;
        }

        if (strcmp(line, "end") == 0) 
        {
            break;
        }

        if (cur_obj)
        {
            if (strcmp(line, "end") == 0)
            {
                cur_obj = NULL;

                continue;
            }

            char *tok = strtok(line, " \t\r\n");

            if (!tok || strcmp(tok, "keyframe") != 0) 
            {
                continue;
            }

            if (cur_obj->kf_count >= MAX_KEYFRAMES)
            {
                fprintf(stderr, "Warning: keyframe limit reached for '%s'\n", cur_obj->name);

                continue;
            }

            Keyframe *kf = &cur_obj->keyframes[cur_obj->kf_count];

            kf->time = 0.0;

            kf->position = av3(0.0, 0.0, 0.0);

            kf->c_diff = av3(1.0, 1.0, 1.0);

            kf->radius = 1.0;

            while ((tok = strtok(NULL, " \t\r\n")) != NULL)
            {
                size_t tlen = strlen(tok);

                if (tlen > 0 && tok[tlen - 1] == ':')
                {
                    tok[tlen - 1] = '\0';

                    if (strcmp(tok, "time") == 0)
                    {
                        char *v = strtok(NULL, " \t\r\n");

                        if (v) 
                        {
                            kf->time = atof(v);
                        }
                    }

                    else if (strcmp(tok, "position") == 0)
                    {
                        char *x = strtok(NULL, " \t\r\n");

                        char *y = strtok(NULL, " \t\r\n");

                        char *z = strtok(NULL, " \t\r\n");

                        if (x && y && z)
                        {
                            kf->position = av3(atof(x), atof(y), atof(z));
                        }
                    }

                    else if (strcmp(tok, "c_diff") == 0)
                    {
                        char *r = strtok(NULL, " \t\r\n");

                        char *g = strtok(NULL, " \t\r\n");

                        char *b = strtok(NULL, " \t\r\n");

                        if (r && g && b)
                        {
                            kf->c_diff = av3(atof(r), atof(g), atof(b));
                        }
                    }

                    else if (strcmp(tok, "radius") == 0)
                    {
                        char *v = strtok(NULL, " \t\r\n");

                        if (v) 
                        {
                            kf->radius = atof(v);
                        }
                    }

                    else if (strcmp(tok, "normal") == 0)
                    {
                        char *x = strtok(NULL, " \t\r\n");

                        char *y = strtok(NULL, " \t\r\n");

                        char *z = strtok(NULL, " \t\r\n");

                        if (x && y && z)
                        {
                            kf->normal = av3(atof(x), atof(y), atof(z));
                        }
                    }
                }
            }

            cur_obj->kf_count++;

            continue;
        }

        char *tok = strtok(line, " \t\r\n");

        if (!tok) 
        {
            continue;
        }

        size_t tlen = strlen(tok);

        if (tlen > 0 && tok[tlen - 1] == ':')
        {
            tok[tlen - 1] = '\0';

            char *val = strtok(NULL, " \t\r\n");

            if (!val) 
            {
                continue;
            }

            if (strcmp(tok, "frames") == 0)
            {
                anim->total_frames = atoi(val);
            }

            else if (strcmp(tok, "fps") == 0)
            {
                anim->fps = atof(val);
            }

            else if (strcmp(tok, "width") == 0)
            {
                anim->img_width = atoi(val);
            }

            else if (strcmp(tok, "height") == 0)
            {
                anim->img_height = atoi(val);
            }

            continue;
        }

        if (strcmp(tok, "camera") == 0)
        {
            memset(&anim->camera, 0, sizeof(Object));

            anim->camera.type = OBJ_CAMERA;

            char *k;

            while ((k = strtok(NULL, " \t\r\n")) != NULL)
            {
                size_t klen = strlen(k);

                if (klen > 0 && k[klen - 1] == ':')
                {
                    k[klen - 1] = '\0';

                    if (strcmp(k, "width") == 0)
                    {
                        char *v = strtok(NULL, " \t\r\n");

                        if (v) 
                        {
                            anim->camera.cam_width = atof(v);
                        }
                    }

                    else if (strcmp(k, "height") == 0)
                    {
                        char *v = strtok(NULL, " \t\r\n");

                        if (v) 
                        {
                            anim->camera.cam_height = atof(v);
                        }
                    }
                }
            }

            anim->has_camera = 1;

            continue;
        }

        if (strcmp(tok, "sphere") == 0 || strcmp(tok, "plane") == 0)
        {
            if (anim->obj_count >= MAX_ANIM_OBJS)
            {
                fprintf(stderr, "Warning: animated object limit reached\n");

                continue;
            }

            ObjType otype = (strcmp(tok, "sphere") == 0) ? OBJ_SPHERE : OBJ_PLANE;

            char *name = strtok(NULL, " \t\r\n");

            cur_obj = &anim->objects[anim->obj_count++];

            memset(cur_obj, 0, sizeof(AnimObject));

            cur_obj->type = otype;

            if (name)
            {
                strncpy(cur_obj->name, name, sizeof(cur_obj->name) - 1);
            }

            else
            {
                snprintf(cur_obj->name, sizeof(cur_obj->name), "obj%d", anim->obj_count - 1);
            }

            continue;
        }
    }

    fclose(fp);

    if (!anim->has_camera)
    {
        fprintf(stderr, "Error: no camera found in '%s'\n", filename);

        return 0;
    }

    return 1;
}


void eval_animation(const Animation *anim, double t, Object *out_objects, int *out_count, Object *out_camera)
{
    *out_count = 0;

    if (out_camera)
    {
        *out_camera = anim->camera;
    }

    for (int i = 0; i < anim->obj_count; i++)
    {
        Object *obj = &out_objects[*out_count];

        eval_anim_object(&anim->objects[i], t, obj);

        (*out_count)++;
    }
}


static uint8_t ftou8(double v)
{
    int iv = (int)(v * 255.0 + 0.5);

    if (iv < 0) 
    {
        return 0;
    }

    if (iv > 255) 
    {
        return 255;
    }

    return (uint8_t)iv;
}


static double isect_sphere(Vec3 o, Vec3 d, const Object *s)
{
    Vec3 oc = av3_sub(s->position, o);

    double tc = av3_dot(oc, d);

    double oc2 = av3_dot(oc, oc);

    if (tc < 0.0 && oc2 > s->radius * s->radius) 
    {
        return -1.0;
    }

    double d2 = oc2 - tc * tc;

    double r2 = s->radius * s->radius;

    if (d2 > r2) 
    {
        return -1.0;
    }

    double a = sqrt(r2 - d2);

    double t1 = tc - a;

    double t2 = tc + a;

    if (t1 > 1e-6) 
    {
        return t1;
    }

    if (t2 > 1e-6) 
    {
        return t2;
    }

    return -1.0;
}


static double isect_plane(Vec3 o, Vec3 d, const Object *p)
{
    double denom = av3_dot(p->normal, d);

    if (fabs(denom) < 1e-10) 
    {
        return -1.0;
    }

    Vec3 op = av3_sub(p->position, o);

    double t = av3_dot(p->normal, op) / denom;

    return (t > 1e-6) ? t : -1.0;
}


static int render_frame(const Animation *anim, double t, const char *path)
{
    Object objects[MAX_OBJECTS];

    int obj_count = 0;

    Object camera;

    eval_animation(anim, t, objects, &obj_count, &camera);

    int W = anim->img_width;

    int H = anim->img_height;

    Image img;

    img.width = W;

    img.height = H;

    img.maxVal = 255;

    img.pixels = (Color *)calloc((size_t)(W * H), sizeof(Color));

    if (!img.pixels)
    {
        fprintf(stderr, "Error: out of memory for frame\n");

        return 0;
    }

    Vec3 origin = av3(0.0, 0.0, 0.0);

    double vw = camera.cam_width;

    double vh = camera.cam_height;

    for (int py = 0; py < H; py++)
    {
        for (int px = 0; px < W; px++)
        {
            double u = vw * ((px + 0.5) / W - 0.5);

            double v = vh * (0.5 - (py + 0.5) / H);

            Vec3 dir = av3_norm(av3(u, v, -1.0));

            double t_min = DBL_MAX;

            Object *hit = NULL;

            for (int i = 0; i < obj_count; i++)
            {
                Object *obj = &objects[i];

                double ht  = -1.0;

                if (obj->type == OBJ_SPHERE)
                {
                    ht = isect_sphere(origin, dir, obj);
                }

                else if (obj->type == OBJ_PLANE)
                {
                    ht = isect_plane(origin, dir, obj);
                }

                if (ht > 0.0 && ht < t_min)
                {
                    t_min = ht;

                    hit = obj;
                }
            }

            if (hit)
            {
                int idx = py * W + px;

                img.pixels[idx].red = ftou8(hit->c_diff.x);

                img.pixels[idx].green = ftou8(hit->c_diff.y);

                img.pixels[idx].blue = ftou8(hit->c_diff.z);
            }
        }
    }

    int ok = (write_ppm(path, &img) == 0);

    free_image(&img);

    return ok;
}


int render_animation(const Animation *anim, const char *out_dir,const char *prefix)
{
    #ifdef _WIN32
        _mkdir(out_dir);
    #else
        mkdir(out_dir, 0755);
    #endif

    int rendered = 0;

    int N = anim->total_frames;

    for (int f = 0; f < N; f++)
    {
        double t = (N > 1) ? (double)f / (double)(N - 1) : 0.0;
        char path[512];

        snprintf(path, sizeof(path), "%s/%s_%04d.ppm", out_dir, prefix, f);

        printf("Rendering frame %4d / %d  (t=%.4f) -> %s\n",f + 1, N, t, path);

        if (render_frame(anim, t, path))
        {
            rendered++;
        }

        else
        {
            fprintf(stderr, "Warning: frame %d failed\n", f);
        }
    }

    return rendered;
}