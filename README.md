[![Linux Build](https://github.com/voldien/opengl-samples/actions/workflows/linux-build.yml/badge.svg)](https://github.com/voldien/opengl-samples/actions/workflows/linux-build.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

# OpenGL Sample Repository - Work In Progress

A collection of OpenGL Samples, work in progress

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
  -D, --display arg         Display (default: -1)
  -m, --multi-sample arg    Set MSAA (default: 0)
```

## Samples

- [**Triangle**](Samples/Triangle)
![Triangle](https://github.com/voldien/OpenGL-Samples/assets/9608088/d85a24d2-97fb-4faa-a7ef-e71bdd979444)

- [**Texture**](Samples/Texture)
![Texture](https://github.com/voldien/OpenGL-Samples/assets/9608088/a87834cc-452e-41e0-8ea2-3de881780823)
![Texture](https://github.com/voldien/OpenGL-Samples/assets/9608088/7bdcaa6f-c0de-4030-b344-d6f180dc6e7b)


- [**Point Lights**](Samples/PointLight)
- ![PointLights](https://github.com/voldien/OpenGL-Samples/assets/9608088/dcd67197-4dc9-4333-ae6f-9fff9f7eb317)

- **Area Lights**

- [**Normal Mapping**](Samples/NormalMap)

- [**SkyBox Cubemap**](Samples/Skybox)

- [**SkyBox Panoramic**](Samples/Skybox)
![SkyboxPanoramic](https://github.com/voldien/OpenGL-Samples/assets/9608088/41cacd12-5782-46bd-b06b-dc3725f21b3d)

- **Refrection**

- **Simple Reflection**

- [**Instance**](Samples/Instance)
![Instance](https://github.com/voldien/OpenGL-Samples/assets/9608088/8fb4adca-cd99-45c9-9811-38e727b6685b)

- [**SubSurface Scattering**](Samples/SubSurfaceScattering)
![Subsurface Scattering](https://github.com/voldien/OpenGL-Samples/assets/9608088/062f5af0-898e-4c1b-91ab-38975b898e0c)
![Subsurface Scattering](https://github.com/voldien/OpenGL-Samples/assets/9608088/ac752bfc-cc72-49ae-a9d6-2c729cd07c2d)
 
- [**Normal**] (Samples/Normal)
![Normal](https://github.com/user-attachments/assets/d06d37ca-71d3-4346-91ef-266bc043a4f9)


- **Blinn - Phong**
![PhongBlinn](https://github.com/voldien/OpenGL-Samples/assets/9608088/e14e4035-1639-4532-a316-65c8c879abf9)
![PhongBlinn](https://github.com/voldien/OpenGL-Samples/assets/9608088/02ae2bac-974e-4270-b80f-a59dba260160)

- **Fog**

- **Terrain**

- **Shadow Map**
![BasicShadowMapp](https://github.com/user-attachments/assets/f4596f70-1f89-4595-912c-dd3848c0788c)


- **Shadow Map PCF**
![BasicShadowMap PCF](https://github.com/user-attachments/assets/1097b804-c8d5-46df-b032-b4e29df7ae58)
![BasicShadowMap PCF](https://github.com/user-attachments/assets/cae348e3-9c2a-4443-8119-02f018fc0109)


- **Point Shadow Light**
![PointLightShadow](https://github.com/user-attachments/assets/daaf91b3-b0ee-4b55-a182-ad326b0e2634)

- [**Shadow Projection**](Samples/ProjectedShadow/)

![ProjectedShadow](https://github.com/user-attachments/assets/d1a17f8e-21c5-42e3-8e4a-094bbe3d8a0b)

- **Shadow Map Variance**

- **Shadow Volume**

- **Cascading Shadow**

- **Contact Shadow**

- [**Tessellation Basic**](Samples/Tessellation/)

- **Grass**

- **MipMap Visual**
![MipMapVisual](https://github.com/voldien/OpenGL-Samples/assets/9608088/ea0714f4-4971-42ad-80db-4d567dc29b03)

- **Multipass**
![AmbientOcclusion](https://github.com/voldien/OpenGL-Samples/assets/9608088/6c156319-7617-43aa-af08-93d709bc07e9)

- [**Deferred Rendering**](Samples/Deferred/)

- [**Ambient Occlusion**](Samples/AmbientOcclusion/)
![AmbientOcclusion](https://github.com/voldien/OpenGL-Samples/assets/9608088/91691104-4c70-4d25-91c0-17cb4bcf38af)
![AmbientOcclusion](https://github.com/voldien/OpenGL-Samples/assets/9608088/4e8f9d85-b48d-4fc4-816d-632a9bbfd37f)
![AmbientOcclusion](https://github.com/voldien/OpenGL-Samples/assets/9608088/4df33593-64f3-4a03-87b5-5e9d205642c5)
![AmbientOcclusion](https://github.com/voldien/OpenGL-Samples/assets/9608088/8ed5a638-2432-4800-8ead-2c5f4881fffc)

- **Physical Based Rendering**

- **Panoramic**
![Panoramic](https://github.com/user-attachments/assets/3431b44d-0932-49cb-a96e-7cc6b4bbd8f2)
![Panoramic](https://github.com/user-attachments/assets/dd618200-ea77-4844-a56f-7b863878864f)


- [**Skinned Mesh**](Samples/SkinnedMesh/)

- [**Simple Ocean**](Samples/SimpleOcean/)

- **Ocean**

- [**Video Playback**](Samples/VideoPlayback/)

- **Ray Tracing**

- [**Game Of Life**](Samples/GameOfLife)
![GameOfLife](https://github.com/voldien/OpenGL-Samples/assets/9608088/5617752d-cd6d-4ceb-9918-f5015565489f)


- [**Mandelbrot**](Samples/Mandelbrot/)
![MandelBrot](https://github.com/voldien/OpenGL-Samples/assets/9608088/cae86eb2-b5f5-4c20-ad13-aed2c154d5d1)

- [**ReactionDiffusion**](Samples/ReactionDiffusion)

- [**Vector Field 2D**](Samples/VectorField2D/)
![VectorField2D](https://github.com/user-attachments/assets/43b3da51-83d6-4b51-ae06-792b01493c2a)

- **Support Vector Machine**

- **Compute Group Visual**
![ComputeGroup](https://github.com/user-attachments/assets/a06d9256-05fa-4e16-9621-e2f7b473c9c5)


- **Rigidbody**
![RigidBody](https://github.com/user-attachments/assets/74e55eea-8f19-4a80-82b1-e88952334db0)


## Build Instruction

```bash
git submodule update --init --recursive
mkdir build && cd build
cmake ..
make
make DownloadAsset
```

## License

This project is licensed under the GPL+3 License - see the [LICENSE](LICENSE) file for details
