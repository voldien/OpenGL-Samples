[![Linux Build](https://github.com/voldien/opengl-samples/actions/workflows/linux-build.yml/badge.svg)](https://github.com/voldien/opengl-samples/actions/workflows/linux-build.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

# OpenGL Sample Repository

A collection of OpenGL Samples.

## CLI - Command Line Options

```bash
Usage:
  OpenGL Sample: GLSample [OPTION...]

 GLSample options:
  -h, --help                helper information.
  -d, --debug               Enable Debug View. (default: true)
  -t, --time arg            How long to run sample (default: 0)
  -f, --fullscreen          Run in FullScreen Mode
  -v, --vsync               Vertical Blank Sync
  -g, --opengl-version arg  OpenGL Version (default: -1)
  -F, --filesystem arg      FileSystem (default: .)
  -r, --renderdoc           Enable RenderDoc
  -G, --gamma-correction    Enable Gamma Correction
  -W, --width arg           Set Window Width (default: -1)
  -H, --height arg          Set Window Height (default: -1)
```

## Samples

- **Triangle**
![Triangle](https://github.com/voldien/OpenGL-Samples/assets/9608088/d85a24d2-97fb-4faa-a7ef-e71bdd979444)

- **Texture**
![Texture](https://github.com/voldien/OpenGL-Samples/assets/9608088/a87834cc-452e-41e0-8ea2-3de881780823)
![Texture](https://github.com/voldien/OpenGL-Samples/assets/9608088/7bdcaa6f-c0de-4030-b344-d6f180dc6e7b)


- **Point Lights**
- ![PointLights](https://github.com/voldien/OpenGL-Samples/assets/9608088/dcd67197-4dc9-4333-ae6f-9fff9f7eb317)

- **Spot Lights**

- **Area Lights**

- **Normal Mapping**

- **SkyBox Cubemap**

- **SkyBox Panoramic**
![SkyboxPanoramic](https://github.com/voldien/OpenGL-Samples/assets/9608088/41cacd12-5782-46bd-b06b-dc3725f21b3d)

- **Refrection**

- **Simple Reflection**

- **Instance**
![Instance](https://github.com/voldien/OpenGL-Samples/assets/9608088/8fb4adca-cd99-45c9-9811-38e727b6685b)

-  **SubSurface Scattering**
![Subsurface Scattering](https://github.com/voldien/OpenGL-Samples/assets/9608088/062f5af0-898e-4c1b-91ab-38975b898e0c)
![Subsurface Scattering](https://github.com/voldien/OpenGL-Samples/assets/9608088/ac752bfc-cc72-49ae-a9d6-2c729cd07c2d)
 
- **Normal**

- **Blinn - Phong**
![PhongBlinn](https://github.com/voldien/OpenGL-Samples/assets/9608088/e14e4035-1639-4532-a316-65c8c879abf9)
![PhongBlinn](https://github.com/voldien/OpenGL-Samples/assets/9608088/02ae2bac-974e-4270-b80f-a59dba260160)
- **Fog**

- **Terrain**

- **Font**

- **Signed Distance Field Font**

- **Shadow Map**
![BasicShadowMap](https://github.com/voldien/OpenGL-Samples/assets/9608088/bd5f11f4-4f3c-4038-a998-9a50fa7eb743)

- **Shadow Map PCF**

- **Point Shadow Light**
![PointLightShadow](https://github.com/voldien/OpenGL-Samples/assets/9608088/8e4e4374-71a3-45ad-9f93-87e8528cbfeb)

- **Shadow Map Variance**

- **Shadow Volume**


- **Cascading Shadow**

- **Contact Shadow**

- **Tessellation Basic**

- **PN Tessellation**

- **Grass**

- **Hair**

- **MipMap Visual**
![MipMapVisual](https://github.com/voldien/OpenGL-Samples/assets/9608088/ea0714f4-4971-42ad-80db-4d567dc29b03)


- **Simple Particle System**

- **Particle System**

- **Multipass**
![AmbientOcclusion](https://github.com/voldien/OpenGL-Samples/assets/9608088/6c156319-7617-43aa-af08-93d709bc07e9)


- **Deferred Rendering**

- **Ambient Occlusion**
![AmbientOcclusion](https://github.com/voldien/OpenGL-Samples/assets/9608088/91691104-4c70-4d25-91c0-17cb4bcf38af)
![AmbientOcclusion](https://github.com/voldien/OpenGL-Samples/assets/9608088/4e8f9d85-b48d-4fc4-816d-632a9bbfd37f)
![AmbientOcclusion](https://github.com/voldien/OpenGL-Samples/assets/9608088/4df33593-64f3-4a03-87b5-5e9d205642c5)
![AmbientOcclusion](https://github.com/voldien/OpenGL-Samples/assets/9608088/8ed5a638-2432-4800-8ead-2c5f4881fffc)

- **Physical Based Rendering**

- **Morph (Blend Shape)**

- **Skinned Mesh**

- **Simple Ocean**

- **Ocean**

- **Video Playback**

- **Ray Tracing**

- **Game Of Life**
![GameOfLife](https://github.com/voldien/OpenGL-Samples/assets/9608088/5617752d-cd6d-4ceb-9918-f5015565489f)


- **Mandelbrot**
![MandelBrot](https://github.com/voldien/OpenGL-Samples/assets/9608088/cae86eb2-b5f5-4c20-ad13-aed2c154d5d1)

- **ReactionDiffusion**

- **Bloom**

- **Vector Field 2D**

## Build Instruction

```bash
git submodule update --init --recursive
mkdir build && cd build
cmake ..
make
```

## License

This project is licensed under the GPL+3 License - see the [LICENSE](LICENSE) file for details
