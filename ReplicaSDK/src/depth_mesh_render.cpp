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
  ASSERT(argc==7, "Usage: ./ReplicaViewer COLOR DEPTH OUT_FILE SPHERICAL ODS JUMP");

  const std::string colorFile = std::string(argv[1]);
  const std::string depthFile = std::string(argv[2]);
  const std::string out_file(argv[3]);

  bool spherical = std::string(argv[4]).compare(std::string("y")) == 0;
  bool ods = std::string(argv[5]).compare(std::string("y")) == 0;
  bool jump = std::string(argv[6]).compare(std::string("y")) == 0;

  // Setup OpenGL Display (based on GLUT)
  int width = 4096;
  int height = 2048;

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
      pangolin::ModelViewLookAtRDF(0, 0, 0, 0, 0, 1, 0, 1, 0));

  const std::string shadir = STR(SHADER_DIR);

  // Render
  pangolin::GlTexture color_buffer(width, height);
  pangolin::GlRenderBuffer depth_buffer(width, height);
  pangolin::GlFramebuffer fbo(color_buffer, depth_buffer);

  std::shared_ptr<Shape> quad = DepthMesh::GenerateMeshData(width, height, spherical);
  DepthMesh depthMesh(
      quad,
      colorFile, depthFile, "", false, spherical, ods, true, jump);

  // Render
  fbo.Bind();

  glPushAttrib(GL_VIEWPORT_BIT);
  glViewport(0, 0, width, height);
  glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
  glEnable(GL_CULL_FACE);

  depthMesh.Render(s_cam);

  glDisable(GL_CULL_FACE);

  fbo.Unbind();

  // Write image
  pangolin::ManagedImage<Eigen::Matrix<uint8_t, 3, 1>> image(width, height);
  pangolin::ManagedImage<Eigen::Matrix<uint8_t, 3, 1>> out_image(width, height);
  color_buffer.Download(image.ptr, GL_RGB, GL_UNSIGNED_BYTE);

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

