[![Linux Build](https://github.com/voldien/opengl-samples/actions/workflows/linux-build.yml/badge.svg)](https://github.com/voldien/opengl-samples/actions/workflows/linux-build.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

# OpenGL Sample Repository

A collection of OpenGL Samples.

## Command line Options

```bash
 OpenGL-Samples options:
  -h, --help                helper information.
  -d, --debug               Enable Debug View. (default: true)
  -t, --time arg            How long to run sample (default: 0)
  -f, --fullscreen          Run in FullScreen Mode
  -v, --vsync               Vertical Blank Sync
  -g, --opengl-version arg  OpenGL Version (default: -1)
  -F, --filesystem arg      FileSystem (default: .)
```

## Samples

- **Starup Window**

- **Triangle**

- **Texture**

- **Point Lights**

- **Spot Lights**

- **Area Lights**

- **Normal Mapping**

- **SkyBox Cubemap**

- **SkyBox Panoramic**

- **Refrection**

- **Simple Reflection**

- **Fog**

- **Terrain**

- **Font**

- **Signed Distance Field Font**

- **Shadow Map**

- **Shadow Map PCF**

- **Shadow Map Variance**

- **Shadow Volume**

- **Cascading Shadow**

- **Contact Shadow**

- **Tessellation Basic**

- **PN Tessellation**

- **Grass**

- **Hair**

- **Simple Particle System**

- **Particle System**

- **NBody Simulation**

- **Multipass**

- **Deferred Rendering**

- **Ambient Occlusion**

- **Physical Based Rendering**

- **Morph (Blend Shape)**

- **Skinned Mesh**

- **Simple Ocean**

- **Ocean**

- **Video Playback**

- **Ray Tracing**

- **Game Of Life**

- **Mandelbrot**

- **ReactionDiffusion**

- **Bloom**

## Build Instruction

```bash
git submodule update --init --recursive
mkdir build && cd build
cmake ..
make
```

## License

This project is licensed under the GPL+3 License - see the [LICENSE](LICENSE) file for details
