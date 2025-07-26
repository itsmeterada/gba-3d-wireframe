# GBA 3D Wireframe Demo

This is a simple 3D graphics demo for the Nintendo Game Boy Advance (GBA). It renders a rotating and scaling wireframe model (either a cube or a torus) using a software-based 3D pipeline.

## Controls

- **A Button:** Switch between the cube and torus models.
- **B Button:** Toggle between Perspective and Orthographic cameras.

## Features

- **Switchable Models:** Toggle between a static cube and a procedurally generated torus.
- **Aspect Ratio Correction:** Renders models with a 3:2 aspect ratio, matching the GBA's screen to prevent distortion.
- **Procedural Model Generation:** The torus mesh (vertices and edges) is generated at runtime using parametric equations.
- **3D Rendering:** A basic 3D pipeline running on the GBA CPU.
- **Switchable Cameras:** Toggle between a perspective and orthographic camera.
- **Rotation and Scaling:** The models animate with continuous rotation on two axes and a pulsating scaling effect.
- **Line Clipping:** Implements the Liang-Barsky algorithm to correctly clip the model's edges against the screen boundaries.
- **Double Buffering:** Uses GBA's Mode 4 with page flipping for smooth, flicker-free animation.
- **Fixed-Point Math:** All calculations use fixed-point arithmetic for performance.
- **High-Resolution Sine Table:** A 12-bit (4096-entry) sine lookup table is used for smooth rotations.

## Building from Source

To build the project, you need to have the **devkitARM** toolchain from devkitPro installed and configured.

Once the toolchain is set up, you can build the ROM by running:

```bash
make
```

This will create the GBA ROM file at `bin/my_game.gba`.

To clean the build artifacts, run:

```bash
make clean
```

## Technical Details

- **Display Mode:** GBA Mode 4 (240x160, 8-bit paletted color)
- **Aspect Ratio:** 3:2
- **Clipping Algorithm:** Liang-Barsky
- **Line Drawing:** Bresenham's line algorithm
