// Copyright (c) Facebook, Inc. and its affiliates. All Rights Reserved
#include "DepthMeshLib.h"
#include "PLYParser.h"
#include "SphereData.h"
#include "lodepng.h"

#include <pangolin/utils/file_utils.h>
#include <pangolin/utils/picojson.h>
#include <Eigen/Geometry>

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <experimental/filesystem>
#include <fstream>
#include <unordered_map>
#include <iostream>

DepthMesh::DepthMesh(const std::string& meshColor, const std::string& meshDepth,
      const std::string& meshAlpha,
      bool rL, bool rS, bool fp) {
  std::cout << meshColor << std::endl;
  std::cout << meshDepth << std::endl;

  // Check everything exists
  ASSERT(pangolin::FileExists(meshColor));
  ASSERT(pangolin::FileExists(meshDepth));

  if(rL && !fp) {
    ASSERT(pangolin::FileExists(meshAlpha));
  }

  // Set rendering mdoe
  renderLayered = rL;
  renderSpherical = rS;
  firstPass = fp;

  // Load mesh data
  LoadMeshData();
  LoadMeshColor(meshColor);
  LoadMeshDepth(meshDepth);

  if(renderLayered && !firstPass) {
    LoadMeshAlpha(meshAlpha);
  }

  if (isHdr) {
    // set defaults for HDR scene
    exposure = 0.025f;
    gamma = 1.6969f;
    saturation = 1.5f;
  }

  // Load shader
  const std::string shadir = STR(SHADER_DIR);
  ASSERT(pangolin::FileExists(shadir), "Shader directory not found!");

  if(renderSpherical){
    shader.AddShaderFromFile(pangolin::GlSlVertexShader, shadir + "/depth-mesh-spherical.vert", {}, {shadir});
    shader.AddShaderFromFile(pangolin::GlSlGeometryShader, shadir + "/depth-mesh-spherical.geom", {}, {shadir});
    shader.AddShaderFromFile(pangolin::GlSlFragmentShader, shadir + "/depth-mesh.frag", {}, {shadir});
    shader.Link();

  }
  else{
    shader.AddShaderFromFile(pangolin::GlSlVertexShader, shadir + "/depth-mesh.vert", {}, {shadir});
    shader.AddShaderFromFile(pangolin::GlSlFragmentShader, shadir + "/depth-mesh.frag", {}, {shadir});
    shader.Link();

  }
}

DepthMesh::~DepthMesh() {}

float DepthMesh::Exposure() const {
  return exposure;
}

void DepthMesh::SetExposure(const float& val) {
  exposure = val;
}

float DepthMesh::Gamma() const {
  return gamma;
}

void DepthMesh::SetGamma(const float& val) {
  gamma = val;
}

float DepthMesh::Saturation() const {
  return saturation;
}

void DepthMesh::SetSaturation(const float& val) {
  saturation = val;
}

void DepthMesh::SetBaseline(const float& val){

  baseline = val;

}

void DepthMesh::Render(
    const pangolin::OpenGlRenderState& cam,
    const Eigen::Vector4f& clipPlane
  ) {
  shader.Bind();
  shader.SetUniform("MVP", cam.GetProjectionModelViewMatrix());
  shader.SetUniform("exposure", exposure);
  shader.SetUniform("gamma", 1.0f / gamma);
  shader.SetUniform("saturation", saturation);
  shader.SetUniform("clipPlane", clipPlane(0), clipPlane(1), clipPlane(2), clipPlane(3));

  if(renderSpherical) {
    shader.SetUniform("MV", cam.GetModelViewMatrix());
    shader.SetUniform("baseline", baseline);
  }

  if(renderLayered) {
    shader.SetUniform("render_layered", 1);

    if(firstPass) {
      shader.SetUniform("first_pass", 1);
    }
    else {
      shader.SetUniform("first_pass", 0);
    }
  }
  else {
    shader.SetUniform("render_layered", 0);
  }

  glActiveTexture(GL_TEXTURE0);
  meshColorTex.Bind();

  glActiveTexture(GL_TEXTURE1);
  meshDepthTex.Bind();

  if(renderLayered && !firstPass) {
    glActiveTexture(GL_TEXTURE2);
    meshAlphaTex.Bind();
  }

  mesh->draw();

  shader.Unbind();
}

void DepthMesh::LoadMeshData() {
  mesh = std::make_shared<Shape>(VBO::GEOMETRY_LAYOUT::LAYOUT_TRIANGLE_STRIP);

  int width = 2048;
  int height = 1024;

  for(int row = 0; row < height; row++) {
    for(int col = 0; col < width; col++) {
      float x = ((float)col) / width;
      float x_next = ((float)col + 1.f) / width;
      float y = ((float)row) / height;
      float y_next = ((float)row + 1.f) / height;

      if(renderSpherical) {
        mesh->addPosition(x, y_next, 0.f);
        mesh->addNormal(0.f, 0.f, 0.f);
        mesh->addTextureCoordinate(x, y_next);

        mesh->addPosition(x, y, 0.f);
        mesh->addNormal(0.f, 0.f, 0.f);
        mesh->addTextureCoordinate(x, y);

        mesh->addPosition(x_next, y_next, 0.f);
        mesh->addNormal(0.f, 0.f, 0.f);
        mesh->addTextureCoordinate(x_next, y_next);

        mesh->addPosition(x_next, y, 0.f);
        mesh->addNormal(0.f, 0.f, 0.f);
        mesh->addTextureCoordinate(x_next, y);
      }
      else {
        mesh->addPosition(x, y, 0.f);
        mesh->addNormal(0.f, 0.f, 0.f);
        mesh->addTextureCoordinate(x, y);

        mesh->addPosition(x, y_next, 0.f);
        mesh->addNormal(0.f, 0.f, 0.f);
        mesh->addTextureCoordinate(x, y_next);

        mesh->addPosition(x_next, y, 0.f);
        mesh->addNormal(0.f, 0.f, 0.f);
        mesh->addTextureCoordinate(x_next, y);

        mesh->addPosition(x_next, y_next, 0.f);
        mesh->addNormal(0.f, 0.f, 0.f);
        mesh->addTextureCoordinate(x_next, y_next);
      }
    }
  }

  //std::vector<float> sphereData;
  //sphereData.resize(sphereVertexCount * 8);
  //std::copy(&sphereVertexBufferData[0], &sphereVertexBufferData[sphereVertexCount * 8 - 1], sphereData.begin());
  //mesh = std::make_shared<Shape>(sphereData, VBO::GEOMETRY_LAYOUT::LAYOUT_TRIANGLES);
}

void DepthMesh::LoadMeshColor(const std::string& meshColor) {
    std::vector<unsigned char> image;
    unsigned width, height;
    unsigned error = lodepng::decode(image, width, height, meshColor);

    meshColorTex.Reinitialise(width, height, GL_RGBA8, true, 0, GL_RGBA, GL_UNSIGNED_BYTE);
    meshColorTex.Upload(&image[0], GL_RGBA, GL_UNSIGNED_BYTE);
}

void DepthMesh::LoadMeshDepth(const std::string& meshDepth) {
    std::vector<unsigned char> image;
    unsigned width, height;
    unsigned error = lodepng::decode(image, width, height, meshDepth);

    meshDepthTex.Reinitialise(width, height, GL_RGBA8, true, 0, GL_RGBA, GL_UNSIGNED_BYTE);
    meshDepthTex.Upload(&image[0], GL_RGBA, GL_UNSIGNED_BYTE);
}

void DepthMesh::LoadMeshAlpha(const std::string& meshAlpha) {
    std::vector<unsigned char> image;
    unsigned width, height;
    unsigned error = lodepng::decode(image, width, height, meshAlpha);

    meshAlphaTex.Reinitialise(width, height, GL_RGBA8, true, 0, GL_RGBA, GL_UNSIGNED_BYTE);
    meshAlphaTex.Upload(&image[0], GL_RGBA, GL_UNSIGNED_BYTE);
}
