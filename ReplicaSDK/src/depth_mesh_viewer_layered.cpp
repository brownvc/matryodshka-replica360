// Copyright (c) Facebook, Inc. and its affiliates. All Rights Reserved
#include <DepthMeshLib.h>
#include <ctime>
#include <pangolin/display/display.h>
#include <pangolin/display/widgets/widgets.h>

int main(int argc, char* argv[]) {
  // Command line args
  ASSERT(argc==3, "Usage: ./ReplicaViewer PREFIX SPHERICAL");

  const std::string prefix(argv[1]);

  const std::string colorFile = prefix + ".png";
  const std::string depthFile = prefix + "D.png";
  const std::string alphaFile = prefix + "A.png";
  const std::string bgColorFile = prefix + "_BG.png";
  const std::string bgDepthFile = prefix + "_BGD.png";
  const std::string bgAlphaFile = prefix + "_BGA.png";
  const std::string inpColorFile = prefix + "_BG_inp.png";
  const std::string inpDepthFile = prefix + "_BGD_inp.png";
  const std::string sphericalArg = std::string(argv[2]);
  bool spherical = sphericalArg.compare(std::string("y")) == 0;

  // Setup OpenGL Display (based on GLUT)
  const int uiWidth = 0;
  const int width = 1280;
  const int height = 960;

  pangolin::CreateWindowAndBind("ReplicaViewer", uiWidth + width, height);

  if (glewInit() != GLEW_OK) {
    pango_print_error("Unable to initialize GLEW.");
  }

  // Setup default OpenGL parameters
  glEnable(GL_DEPTH_TEST);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  const GLenum frontFace = GL_CCW;
  glFrontFace(frontFace);
  glLineWidth(1.0f);

  // Tell the base view to arrange its children equally
  if (uiWidth != 0) {
    pangolin::CreatePanel("ui").SetBounds(0, 1.0f, 0, pangolin::Attach::Pix(uiWidth));
  }

  pangolin::View& container =
      pangolin::CreateDisplay().SetBounds(0, 1.0f, pangolin::Attach::Pix(uiWidth), 1.0f);

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

  pangolin::Handler3D s_handler(s_cam);

  pangolin::View& meshView = pangolin::Display("MeshView")
                                 .SetBounds(0, 1.0f, 0, 1.0f, (double)width / (double)height)
                                 .SetHandler(&s_handler);

  container.AddDisplay(meshView);
  const std::string shadir = STR(SHADER_DIR);
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
      inpColorFile, inpDepthFile, "", true, spherical, false, true);
  DepthMesh depthMeshBg(
      quad,
      bgColorFile, bgDepthFile, bgAlphaFile, true, spherical, false, false);
  DepthMesh depthMeshFg(
      quad,
      colorFile, depthFile, alphaFile, true, spherical, false, false);

  depthMeshInp.SetExposure(1.f);
  depthMeshBg.SetExposure(1.f);
  depthMeshFg.SetExposure(1.f);

  while (!pangolin::ShouldQuit()) {
      if (meshView.IsShown()) {
      meshView.Activate(s_cam);

      // First pass
      fbo1.Bind();

      glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
      glEnable(GL_CULL_FACE);

      depthMeshInp.Render(s_cam);

      glDisable(GL_CULL_FACE);

      fbo1.Unbind();

      // Second pass
      fbo2.Bind();

      glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
      glEnable(GL_CULL_FACE);

      glActiveTexture(GL_TEXTURE3);
      color_buffer1.Bind();
      depthMeshBg.Render(s_cam);

      glDisable(GL_CULL_FACE);

      fbo2.Unbind();

      // Third pass
      glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
      glEnable(GL_CULL_FACE);

      glActiveTexture(GL_TEXTURE3);
      color_buffer2.Bind();
      depthMeshFg.Render(s_cam);

      glDisable(GL_CULL_FACE);
      }

      pangolin::FinishFrame();
  }

  return 0;
}
