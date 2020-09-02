// Copyright (c) Facebook, Inc. and its affiliates. All Rights Reserved
#pragma once
#include <pangolin/display/opengl_render_state.h>
#include <pangolin/gl/gl.h>
#include <pangolin/gl/glsl.h>
#include <memory>
#include <string>

#include "Assert.h"
#include "MeshData.h"

#define XSTR(x) #x
#define STR(x) XSTR(x)

class PTexMesh {
 public:
  PTexMesh(const std::string& meshFile, const std::string& atlasFolder, bool renderSpherical, bool renderOpticalFlow = false);

  virtual ~PTexMesh();

  void RenderSubMesh(
      size_t subMesh,
      const pangolin::OpenGlRenderState& cam,
      const Eigen::Vector4f& clipPlane,
      int lrC = 0);

  void RenderSubMeshDepth(
      size_t subMesh,
      const pangolin::OpenGlRenderState& cam,
      const float depthScale,
      const Eigen::Vector4f& clipPlane,
      int lrC = 0);

  void RenderSubMeshOpticalFlow(
      size_t subMesh,
      const pangolin::OpenGlRenderState& cam,
      const pangolin::OpenGlRenderState& cam_pre,
      const int image_width,
      const int image_height);

  void Render(
      const pangolin::OpenGlRenderState& cam,
      const Eigen::Vector4f& clipPlane = Eigen::Vector4f(0.0f, 0.0f, 0.0f, 0.0f),
      int lrC = 0);

  void RenderWireframe(
      const pangolin::OpenGlRenderState& cam,
      const Eigen::Vector4f& clipPlane = Eigen::Vector4f(0.0f, 0.0f, 0.0f, 0.0f));

  void RenderDepth(
    const pangolin::OpenGlRenderState& cam,
    const float depthScale=1.0f,
    const Eigen::Vector4f& clipPlane = Eigen::Vector4f(0.0f, 0.0f, 0.0f, 0.0f),
    int lrC = 0);

  void RenderOpticalFlow(
    const pangolin::OpenGlRenderState& cam,
    const pangolin::OpenGlRenderState& cam_pre,
    const int image_width,
    const int image_height);

  float Exposure() const;
  void SetExposure(const float& val);

  float Gamma() const;
  void SetGamma(const float& val);

  float Saturation() const;
  void SetSaturation(const float& val);

  void SetBaseline(const float& val);


  size_t GetNumSubMeshes() {
    return meshes.size();
  }

 private:
  struct Mesh {
    pangolin::GlTexture atlas;
    pangolin::GlBuffer vbo;
    pangolin::GlBuffer ibo;
    pangolin::GlBuffer abo;
  };

  static std::vector<MeshData> SplitMesh(const MeshData& mesh, const float splitSize);
  static void CalculateAdjacency(const MeshData& mesh, std::vector<uint32_t>& adjFaces);

  void LoadMeshData(const std::string& meshFile);
  void LoadAtlasData(const std::string& atlasFolder);

  float splitSize = 0.0f;
  uint32_t tileSize = 0;

  bool renderSpherical=false;
  bool renderOpticalFlow=false;

  pangolin::GlSlProgram shader;
  pangolin::GlSlProgram depthShader;
  pangolin::GlSlProgram opticalflowShader;

  float exposure = 1.0f;
  float gamma = 1.0f;
  float saturation = 1.0f;
  float baseline = 0.064f;
  bool isHdr = false;


  static constexpr int ROTATION_SHIFT = 30;
  static constexpr int FACE_MASK = 0x3FFFFFFF;

  std::vector<std::unique_ptr<Mesh>> meshes;
};
