// Copyright (c) Facebook, Inc. and its affiliates. All Rights Reserved
#pragma once
#include <pangolin/display/opengl_render_state.h>
#include <pangolin/gl/gl.h>
#include <pangolin/gl/glsl.h>
#include <memory>
#include <string>

#include "Assert.h"
#include "MeshData.h"
#include "Shape.h"

#define XSTR(x) #x
#define STR(x) XSTR(x)

class DepthMesh {
 public:
  DepthMesh(const std::shared_ptr<Shape> &mesh,
      const std::string& meshColor, const std::string& meshDepth,
      const std::string& meshAlpha,
      bool renderLayered, bool renderSpherical, bool firstPass=true);

  DepthMesh(const std::shared_ptr<Shape> &mesh,
      pangolin::GlTexture &meshColorTex,
      pangolin::GlTexture &meshDepthTex,
      bool renderSpherical);

  virtual ~DepthMesh();

  void Render(
      const pangolin::OpenGlRenderState& cam,
      const Eigen::Vector4f& clipPlane = Eigen::Vector4f(0.0f, 0.0f, 0.0f, 0.0f));

  float Exposure() const;
  void SetExposure(const float& val);

  float Gamma() const;
  void SetGamma(const float& val);

  float Saturation() const;
  void SetSaturation(const float& val);

  void SetBaseline(const float& val);
  static std::shared_ptr<Shape> GenerateMeshData(int width, int height, bool renderSpherical);

 private:
  void LoadMeshColor(const std::string& meshColor);
  void LoadMeshDepth(const std::string& meshDepth);
  void LoadMeshAlpha(const std::string& meshAlpha);

  bool renderLayered = false;
  bool renderSpherical = false;
  bool firstPass = false;

  pangolin::GlSlProgram shader;

  float exposure = 1.0f;
  float gamma = 1.0f;
  float saturation = 1.0f;
  float baseline = 0.064f;
  bool isHdr = false;

 public:
  std::shared_ptr<Shape> mesh;
  pangolin::GlTexture meshColorTex, meshDepthTex, meshAlphaTex;
};
