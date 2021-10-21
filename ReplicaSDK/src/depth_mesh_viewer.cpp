// Copyright (c) Facebook, Inc. and its affiliates. All Rights Reserved
#include <DepthMeshLib.h>
#include <ctime>
#include <pangolin/display/display.h>
#include <pangolin/display/widgets/widgets.h>

int main(int argc, char* argv[]) {
  // Command line args
  ASSERT(argc==7, "Usage: ./ReplicaViewer COLOR DEPTH BASELINE SPHERICAL ODS JUMP");

  const std::string colorFile = std::string(argv[1]);
  const std::string depthFile = std::string(argv[2]);
  bool spherical = std::string(argv[4]).compare(std::string("y")) == 0;
  bool ods = std::string(argv[5]).compare(std::string("y")) == 0;
  bool jump = std::string(argv[6]).compare(std::string("y")) == 0;
  float baseline = std::stof(std::string(argv[3]));

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
  const GLenum frontFace = GL_CW;
  glFrontFace(frontFace);
  glLineWidth(1.0f);

  // Tell the base view to arrange its children equally
  if (uiWidth != 0) {
    pangolin::CreatePanel("ui").SetBounds(0, 1.0f, 0, pangolin::Attach::Pix(uiWidth));
  }

  pangolin::View& container =
      pangolin::CreateDisplay().SetBounds(0, 1.0f, pangolin::Attach::Pix(uiWidth), 1.0f);

  Eigen::Matrix4d start_mat = pangolin::ModelViewLookAtRDF(0, 0, 0, 0, 0, 1, 0, 1, 0);

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
      start_mat);

  pangolin::Handler3D s_handler(s_cam);

  pangolin::View& meshView = pangolin::Display("MeshView")
                                 .SetBounds(0, 1.0f, 0, 1.0f, (double)width / (double)height)
                                 .SetHandler(&s_handler);

  container.AddDisplay(meshView);
  const std::string shadir = STR(SHADER_DIR);
  std::shared_ptr<Shape> quad = DepthMesh::GenerateMeshData(width, height, spherical);

  DepthMesh depthMesh(
      quad,
      colorFile, depthFile, "", false, spherical, ods, true, jump);

  depthMesh.SetBaseline(baseline);

  while (!pangolin::ShouldQuit()) {
    if (meshView.IsShown()) {
      meshView.Activate(s_cam);

      // Render
      glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
      glEnable(GL_CULL_FACE);

      depthMesh.Render(s_cam);

      glDisable(GL_CULL_FACE);
    }

    pangolin::FinishFrame();
  }

  return 0;
}
