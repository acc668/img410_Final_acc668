# Final Project: Animation

## Author(s)
- Name: Alexandra Curry
- Email: acc668@nau.edu

## Overview

This project extends the existing ray caster with a **keyframe animation system**.
A new `.anim` scene format lets you specify multiple keyframes per object, and
the renderer interpolates position, color, and radius between them using a
**Catmull-Rom spline**. The result is a sequence of PPM frames that can be
assembled into an animated GIF or MP4.

---

## Building

```
make
```

This produces two binaries:

| Binary    | Purpose                                    |
|-----------|--------------------------------------------|
| `raycast` | Original single-frame renderer             |
| `animate` | New multi-frame animation renderer         |

---

# Usage

### Single frame
```
./raycast width height input.scene output.ppm
```

### Animation
```
./animate input.anim output_dir [frame_prefix]
```

- `input.anim`    – animation scene file (see format below)
- `output_dir`    – directory that will receive the PPM frames
- `frame_prefix`  – optional prefix (default: `frame`)

Frame files are written as `output_dir/<prefix>_0000.ppm`, `_0001.ppm`, etc.

**Example:**
```
./animate orbit.anim frames frame
```

### Assembling frames into a GIF or MP4
```
chmod +x render_anim.sh
./render_anim.sh frames frame 24 gif   # requires ffmpeg or ImageMagick
./render_anim.sh frames frame 24 mp4   # requires ffmpeg
```

---

## Files

- `animate.hpp`          Animation, AnimObject, Keyframe structs; public API
- `animate.cpp`          Parser, Catmull-Rom interpolation, per-frame renderer
- `animateMain.cpp`      main() entry point for the animate binary
- `orbit.anim`           Example animation scene (two orbiting spheres)
- `renderAnimation.sh`   Shell script to convert PPM frames to GIF/MP4

---

## Research

The core algorithm for this project is **Catmull-Rom spline interpolation**, used to smoothly animate object positions, colors, and sizes between keyframes.

### Why not linear interpolation?

Linear interpolation (lerp) between keyframes produces robotic, unnatural motion — objects snap to constant velocity and jerk at every keyframe. A spline through the keyframes looks far more natural because it eases in and out of each position.

### Catmull-Rom splines

A Catmull-Rom spline is a cubic Hermite spline where tangents are estimated automatically from neighboring control points. This means the animator only provides keyframe positions — no extra tangent handles needed. Crucially, the curve passes exactly through every keyframe (it is *interpolating*, not approximating), so objects land precisely on every keyframed value.

Given four control points p0, p1, p2, p3, the curve between p1 and p2 at parameter u in [0,1] is:

    q(u) = 0.5 * (
        (-u^3 + 2u^2 - u      ) * p0
      + ( 3u^3 - 5u^2 + 2    ) * p1
      + (-3u^3 + 4u^2 + u    ) * p2
      + ( u^3 - u^2          ) * p3
    )

This formula is applied independently to each component of position, color, and radius.

### Color clamping

The spline can slightly overshoot its control points. For position this creates nice arcing paths, but for color it would produce invalid RGB values. The implementation clamps each color channel to [0,1] after interpolation to prevent this.

### References

- Catmull, E. & Rom, R. (1974). "A class of local interpolating splines."
  Computer Aided Geometric Design, pp. 317-326.
- Lasseter, J. (1987). "Principles of traditional animation applied to 3D
  computer animation." SIGGRAPH '87 Proceedings.
- Wikipedia: Centripetal Catmull-Rom spline
  https://en.wikipedia.org/wiki/Centripetal_Catmull%E2%80%93Rom_spline

---

# Known Issues

None.