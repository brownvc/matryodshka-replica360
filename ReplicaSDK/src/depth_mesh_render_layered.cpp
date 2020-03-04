// Copyright (c) Facebook, Inc. and its affiliates. All Rights Reserved
#include <EGL.h>
#include <DepthMeshLib.h>
#include <ctime>
#include <pangolin/display/display.h>
#include <pangolin/display/widgets/widgets.h>


// TODO:
// 1) Higher resolution spheres
// 2) Render all at once
// 3) Writing, evaluation, check-in with Selena

int main(int argc, char* argv[]) {
  // Command line args
  ASSERT(argc==4, "Usage: ./ReplicaViewer PREFIX OUT_FILE SPHERICAL");

  const std::string prefix(argv[1]);
  const std::string out_file(argv[2]);
  const std::string colorFile = prefix + ".png";
  const std::string depthFile = prefix + "D.png";
  const std::string alphaFile = prefix + "A.png";
  const std::string bgColorFile = prefix + "_BG.png";
  const std::string bgDepthFile = prefix + "_BGD.png";
  const std::string bgAlphaFile = prefix + "_BGA.png";
  const std::string inpColorFile = prefix + "_BG_inp.png";
  const std::string inpDepthFile = prefix + "_BGD_inp.png";
  const std::string sphericalArg = std::string(argv[3]);
  bool spherical = sphericalArg.compare(std::string("y")) == 0;

  // Setup OpenGL Display (based on GLUT)
  int width = 640;
  int height = 320;

  // Setup EGL
  EGLCtx egl;
  egl.PrintInformation();

  // Setup default OpenGL parameters
  glEnable(GL_DEPTH_TEST);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  const GLenum frontFace = GL_CW;
  glFrontFace(frontFace);
  glLineWidth(1.0f);

  pangolin::OpenGlRenderState s_cam(
      pangolin::ProjectionMatrixRDF_TopLeft(
          width,
          height,
          width / 2.0f,
          width / 2.0f,
          (width - 1.0f) / 2.0f,
          (height - 1.0f) / 2.0f,
          0.1f,
          100.0f),
      pangolin::ModelViewLookAtRDF(0, 0, 0.3, 0, 0, 1, 0, 1, 0));

  const std::string shadir = STR(SHADER_DIR);

  pangolin::GlTexture color_buffer3(width, height);
  pangolin::GlRenderBuffer depth_buffer3(width, height);
  pangolin::GlFramebuffer fbo3(color_buffer3, depth_buffer3);
  std::shared_ptr<Shape> quad = DepthMesh::GenerateMeshData(width, height, spherical);

  // FBOs
  pangolin::GlTexture color_buffer1(width, height);
  pangolin::GlRenderBuffer depth_buffer1(width, height);
  pangolin::GlFramebuffer fbo1(color_buffer1, depth_buffer1);

  pangolin::GlTexture color_buffer2(width, height);
  pangolin::GlRenderBuffer depth_buffer2(width, height);
  pangolin::GlFramebuffer fbo2(color_buffer2, depth_buffer2);

  // Rendering passes
  DepthMesh depthMeshInp(
      quad,
      inpColorFile, inpDepthFile, "", true, spherical, true);
  DepthMesh depthMeshBg(
      quad,
      bgColorFile, bgDepthFile, bgAlphaFile, true, spherical, false);
  DepthMesh depthMeshFg(
      quad,
      colorFile, depthFile, alphaFile, true, spherical, false);

  depthMeshInp.SetExposure(1.f);
  depthMeshBg.SetExposure(1.f);
  depthMeshFg.SetExposure(1.f);

  // First pass
  fbo1.Bind();

  glPushAttrib(GL_VIEWPORT_BIT);
  glViewport(0, 0, width, height);
  glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
  glEnable(GL_CULL_FACE);

  depthMeshInp.Render(s_cam);

  glDisable(GL_CULL_FACE);

  fbo1.Unbind();

  // Second pass
  fbo2.Bind();

  glPushAttrib(GL_VIEWPORT_BIT);
  glViewport(0, 0, width, height);
  glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
  glEnable(GL_CULL_FACE);

  glActiveTexture(GL_TEXTURE3);
  color_buffer1.Bind();
  depthMeshBg.Render(s_cam);

  glDisable(GL_CULL_FACE);

  fbo2.Unbind();

  // Third pass
  fbo3.Bind();

  glPushAttrib(GL_VIEWPORT_BIT);
  glViewport(0, 0, width, height);
  glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
  glEnable(GL_CULL_FACE);

  glActiveTexture(GL_TEXTURE3);
  color_buffer2.Bind();
  depthMeshFg.Render(s_cam);

  glDisable(GL_CULL_FACE);

  fbo3.Unbind();

  // Write image
  pangolin::ManagedImage<Eigen::Matrix<uint8_t, 3, 1>> image(width, height);
  pangolin::ManagedImage<Eigen::Matrix<uint8_t, 3, 1>> out_image(width, height);
  color_buffer3.Download(image.ptr, GL_RGB, GL_UNSIGNED_BYTE);

  if(spherical) {
    for(int i = 0; i < height; i++) {
      for(int j = 0; j < width; j++) {
        out_image[i * width + j] = image[i * width + (width - j - 1)];
      }
    }
  }
  else {
    for(int i = 0; i < height; i++) {
      for(int j = 0; j < width; j++) {
        out_image[i * width + j] = image[(height - i - 1) * width + j];
      }
    }
  }

  pangolin::SaveImage(
      out_image.UnsafeReinterpret<uint8_t>(),
      pangolin::PixelFormatFromString("RGB24"),
      std::string(out_file));

  return 0;
}

