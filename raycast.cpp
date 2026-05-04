#include "raycast.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

Vec3 vec3(double x, double y, double z) 
{
    Vec3 v = {x, y, z};
    
    return v;
}

Vec3 vec3_sub(Vec3 a, Vec3 b) 
{
    return vec3(a.x - b.x, a.y - b.y, a.z - b.z);
}

double vec3_dot(Vec3 a, Vec3 b) 
{
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

double vec3_length(Vec3 a) 
{
    return sqrt(a.x*a.x + a.y*a.y + a.z*a.z);
}

Vec3 vec3_normalize(Vec3 a) 
{
    double len = vec3_length(a);

    if (len < 1e-10) return vec3(0.0, 0.0, 0.0);

    return vec3(a.x / len, a.y / len, a.z / len);
}

double intersect_sphere(Vec3 origin, Vec3 dir, const Object *s) 
{
    Vec3 oc = vec3_sub(s->position, origin);
    double t_close = vec3_dot(oc, dir);   
    double oc_len2 = vec3_dot(oc, oc);

    if (t_close < 0.0 && oc_len2 > s->radius * s->radius)
        return -1.0;

    double d2 = oc_len2 - t_close * t_close;
    double r2 = s->radius * s->radius;

    if (d2 > r2) return -1.0;   

    double a  = sqrt(r2 - d2);
    double t1 = t_close - a;    
    double t2 = t_close + a;

    if (t1 > 1e-6) return t1;

    if (t2 > 1e-6) return t2;

    return -1.0;
}

double intersect_plane(Vec3 origin, Vec3 dir, const Object *p) 
{
    double denom = vec3_dot(p->normal, dir);
    
    if (fabs(denom) < 1e-10) return -1.0;

    Vec3 op = vec3_sub(p->position, origin);
    double t = vec3_dot(p->normal, op) / denom;

    return (t > 1e-6) ? t : -1.0;
}

static void parse_object_line(char *line, Object *obj) 
{
    char *tok = strtok(line, " \t\r\n");

    while (tok) 
    {
        size_t len = strlen(tok);
        
        if (len > 0 && tok[len - 1] == ':') 
        {
            tok[len - 1] = '\0';

            const char *key = tok;

            if (strcmp(key, "width") == 0) 
            {
                tok = strtok(NULL, " \t\r\n");

                if (tok) obj->cam_width = atof(tok);

            } 
            
            else if (strcmp(key, "height") == 0) 
            {
                tok = strtok(NULL, " \t\r\n");

                if (tok) obj->cam_height = atof(tok);

            } 
            
            else if (strcmp(key, "radius") == 0) 
            {
                tok = strtok(NULL, " \t\r\n");

                if (tok) obj->radius = atof(tok);

            } 
            
            else if (strcmp(key, "c_diff") == 0) 
            {
                
                tok = strtok(NULL, " \t\r\n");

                if (tok) obj->c_diff.x = atof(tok);

                tok = strtok(NULL, " \t\r\n");

                if (tok) obj->c_diff.y = atof(tok);

                tok = strtok(NULL, " \t\r\n");

                if (tok) obj->c_diff.z = atof(tok);

            } 
            
            else if (strcmp(key, "position") == 0) 
            {
                tok = strtok(NULL, " \t\r\n");

                if (tok) obj->position.x = atof(tok);

                tok = strtok(NULL, " \t\r\n");

                if (tok) obj->position.y = atof(tok);

                tok = strtok(NULL, " \t\r\n");

                if (tok) obj->position.z = atof(tok);

            } 
            
            else if (strcmp(key, "normal") == 0) 
            {
                tok = strtok(NULL, " \t\r\n");

                if (tok) obj->normal.x = atof(tok);

                tok = strtok(NULL, " \t\r\n");

                if (tok) obj->normal.y = atof(tok);

                tok = strtok(NULL, " \t\r\n");

                if (tok) obj->normal.z = atof(tok);
            }
        }

        tok = strtok(NULL, " \t\r\n");
    }
}

int load_scene(const char *filename, Object *objects, int *count,
                      Object **camera) 
{
    FILE *fp = fopen(filename, "r");

    if (!fp) 
    {
        fprintf(stderr, "Error: cannot open scene file '%s'\n", filename);
        
        return 0;
    }

    char line[1024];
    *count  = 0;
    *camera = NULL;

    
    if (!fgets(line, sizeof(line), fp)) 
    {
        fclose(fp);

        return 0;
    }
    
    {
        int i = (int)strlen(line) - 1;

        while (i >= 0 && (line[i] == '\n' || line[i] == '\r' || line[i] == ' '))
            line[i--] = '\0';
    }

    if (strcmp(line, "img410scene") != 0) 
    {
        fprintf(stderr, "Error: missing 'img410scene' header\n");

        fclose(fp);

        return 0;
    }

    
    while (fgets(line, sizeof(line), fp)) 
    {
        {
            int i = (int)strlen(line) - 1;

            while (i >= 0 && (line[i] == '\n' || line[i] == '\r'))
                line[i--] = '\0';
        }

        if (strlen(line) == 0) continue;

        if (strncmp(line, "end", 3) == 0) break;
        
        char linecopy[1024];

        strncpy(linecopy, line, sizeof(linecopy) - 1);

        linecopy[sizeof(linecopy) - 1] = '\0';

        char *type_tok = strtok(linecopy, " \t\r\n");

        if (!type_tok) continue;

        if (*count >= MAX_OBJECTS) 
        {
            fprintf(stderr, "Warning: object limit reached, skipping line\n");

            continue;
        }

        Object *obj = &objects[*count];

        memset(obj, 0, sizeof(Object));   

        if (strcmp(type_tok, "camera") == 0) 
        {
            obj->type = OBJ_CAMERA;
        } 
        
        else if (strcmp(type_tok, "sphere") == 0) 
        {
            obj->type = OBJ_SPHERE;
        } 
        
        else if (strcmp(type_tok, "plane") == 0) 
        {
            obj->type = OBJ_PLANE;
        } 
        
        else 
        {
            continue;   
        }

        
        char rest[1024];
        char *after = line + strlen(type_tok);

        strncpy(rest, after, sizeof(rest) - 1);

        rest[sizeof(rest) - 1] = '\0';

        {
            int i = (int)strlen(rest) - 1;

            while (i >= 0 && (rest[i] == ';'  || rest[i] == ' ' ||
                               rest[i] == '\r' || rest[i] == '\n'))
                rest[i--] = '\0';
        }

        parse_object_line(rest, obj);

        if (obj->type == OBJ_CAMERA) *camera = obj;

        (*count)++;
    }

    fclose(fp);

    return 1;
}

uint8_t float_to_u8(double v) 
{
    int iv = (int)(v * 255.0 + 0.5);

    if (iv < 0) return 0;
    if (iv > 255) return 255;

    return (uint8_t)iv;
}

int main(int argc, char *argv[]) 
{
    if (argc != 5) 
    {
        fprintf(stderr, "Usage: %s width height input.scene output.ppm\n",
                argv[0]);
        
        return 1;
    }

    int img_width = atoi(argv[1]);
    int img_height = atoi(argv[2]);
    const char *scene_file = argv[3];
    const char *output_file = argv[4];

    if (img_width <= 0 || img_height <= 0) 
    {
        fprintf(stderr, "Error: invalid image dimensions %dx%d\n",
                img_width, img_height);

        return 1;
    }

    Object objects[MAX_OBJECTS];
    int obj_count = 0;
    Object *camera = NULL;

    if (!load_scene(scene_file, objects, &obj_count, &camera)) 
    {
        return 1;
    }

    if (!camera) 
    {
        fprintf(stderr, "Error: no camera found in scene\n");

        return 1;
    }

    double view_width  = camera->cam_width;
    double view_height = camera->cam_height;

    Image img;
    img.width  = img_width;
    img.height = img_height;
    img.maxVal = 255;

    img.pixels = (Color *)calloc((size_t)(img_width * img_height), sizeof(Color));

    if (!img.pixels) 
    {
        fprintf(stderr, "Error: out of memory\n");

        return 1;
    }

    Vec3 origin = vec3(0.0, 0.0, 0.0);   

    for (int py = 0; py < img_height; py++) 
    {
        for (int px = 0; px < img_width; px++) 
        {
            double u = view_width * ((px + 0.5) / img_width - 0.5);
            double v = view_height * (0.5 - (py + 0.5) / img_height);
            Vec3 dir = vec3_normalize(vec3(u, v, -1.0));
            double t_min = DBL_MAX;
            Object *hit = NULL;

            for (int i = 0; i < obj_count; i++) 
            {
                Object *obj = &objects[i];
                double t = -1.0;

                if (obj->type == OBJ_SPHERE) 
                {
                    t = intersect_sphere(origin, dir, obj);
                } 
                
                else if (obj->type == OBJ_PLANE) 
                {
                    t = intersect_plane(origin, dir, obj);
                }
                

                if (t > 0.0 && t < t_min) 
                {
                    t_min = t;

                    hit = obj;
                }
            }

            
            if (hit) 
            {
                int idx = py * img_width + px;

                img.pixels[idx].red   = float_to_u8(hit->c_diff.x);
                img.pixels[idx].green = float_to_u8(hit->c_diff.y);
                img.pixels[idx].blue  = float_to_u8(hit->c_diff.z);
            }
        }
    }

    
    
    
    if (write_ppm(output_file, &img) != 0) 
    {
        free_image(&img);

        return 1;
    }

    free_image(&img);

    printf("Rendered %dx%d image to '%s'\n", img_width, img_height, output_file);

    return 0;
}