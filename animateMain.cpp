#include "animate.hpp"
#include <stdio.h>
#include <stdlib.h>


int main(int argc, char *argv[])
{
    if (argc < 3 || argc > 4)
    {
        fprintf(stderr, "Usage: %s input.anim output_dir [frame_prefix]\n", argv[0]);

        fprintf(stderr, "input.anim : animation scene file\n");

        fprintf(stderr, "output_dir : directory for output PPM frames\n");

        fprintf(stderr, "frame_prefix : prefix for frame files (default: frame)\n");

        return 1;
    }

    const char *anim_file = argv[1];
    const char *out_dir = argv[2];
    const char *prefix = (argc == 4) ? argv[3] : "frame";
    Animation anim;

    printf("Loading animation from '%s'...\n", anim_file);

    if (!load_animation(anim_file, &anim))
    {
        fprintf(stderr, "Error: failed to load animation file\n");

        return 1;
    }

    printf("Animation: %d frames at %.1f fps, %dx%d, %d animated object(s)\n", anim.total_frames, anim.fps, anim.img_width, anim.img_height, anim.obj_count);

    int rendered = render_animation(&anim, out_dir, prefix);

    printf("\nDone. Rendered %d / %d frames to '%s/'\n", rendered, anim.total_frames, out_dir);

    if (rendered < anim.total_frames)
    {
        return 1;
    }

    return 0;
}