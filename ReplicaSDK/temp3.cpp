// Copyright (c) Facebook, Inc. and its affiliates. All Rights Reserved
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
  ASSERT(argc==4, "Usage: ./ReplicaViewer PREFIX LAYERED SPHERICAL");

  const std::string prefix(argv[1]);

  const std::string colorFile = prefix + ".png";
  const std::string depthFile = prefix + "D.png";
  const std::string alphaFile = prefix + "A.png";
  const std::string bgColorFile = prefix + "_BG.png";
  const std::string bgDepthFile = prefix + "_BGD.png";
  const std::string bgAlphaFile = prefix + "_BGA.png";
  const std::string inpColorFile = prefix + "_BG_inp.png";
  const std::string inpDepthFile = prefix + "_BGD_inp.png";

  const std::string layeredArg = std::string(argv[2]);
  const std::string sphericalArg = std::string(argv[3]);

  bool layered = layeredArg.compare(std::string("y")) == 0;
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

  // Inpainted layer
  pangolin::GlTexture color_buffer1(width, height);
  pangolin::GlRenderBuffer depth_buffer1(width, height);
  pangolin::GlFramebuffer fbo_buffer1(color_buffer1, depth_buffer1);

  DepthMesh depthMesh1(inpColorFile, inpDepthFile, layered, spherical);
  depthMesh1.SetExposure(1.f);

  // Background layer
  pangolin::GlTexture color_buffer2(width, height);
  pangolin::GlRenderBuffer depth_buffer2(width, height);
  pangolin::GlFramebuffer fbo_buffer2(color_buffer2, depth_buffer2);

  DepthMesh depthMesh2(bgColorFile, bgDepthFile, layered, spherical);
  depthMesh2.SetExposure(1.f);

  // Background alpha
  pangolin::GlTexture bgAlphaTex;
  std::vector<unsigned char> image2;
  unsigned w, h;
  unsigned error = lodepng::decode(image2, w, h, bgAlphaFile);
  bgAlphaTex.Reinitialise(width, height, GL_RGBA8, true, 0, GL_RGBA, GL_UNSIGNED_BYTE);
  bgAlphaTex.Upload(&image2[0], GL_RGBA, GL_UNSIGNED_BYTE);

  // Inpainted layer
  pangolin::GlTexture color_buffer3(width, height);
  pangolin::GlRenderBuffer depth_buffer3(width, height);
  pangolin::GlFramebuffer fbo_buffer3(color_buffer3, depth_buffer3);

  //DepthMesh depthMesh3(colorFile, depthFile, layered, spherical);
  //depthMesh3.SetExposure(1.f);

  // Create fullscreen quad
  std::shared_ptr<Shape> quad = std::make_shared<Shape>();

  quad->addPosition(-1, -1, 0);
  quad->addPosition(1, -1, 0);
  quad->addPosition(1, 1, 0);
  quad->addPosition(-1, 1, 0);

  quad->addNormal(0, 0, 1);
  quad->addNormal(0, 0, 1);
  quad->addNormal(0, 0, 1);
  quad->addNormal(0, 0, 1);

  quad->addTextureCoordinate(0, 0);
  quad->addTextureCoordinate(1, 0);
  quad->addTextureCoordinate(1, 1);
  quad->addTextureCoordinate(0, 1);

  quad->addFace(0, 1, 2);
  quad->addFace(2, 3, 0);

  // Passthrough shader
  pangolin::GlSlProgram finalPassShader;
  finalPassShader.AddShaderFromFile(pangolin::GlSlVertexShader, shadir + "/final-pass.vert", {}, {shadir});
  finalPassShader.AddShaderFromFile(pangolin::GlSlFragmentShader, shadir + "/final-pass.frag", {}, {shadir});
  finalPassShader.Link();

  while (!pangolin::ShouldQuit()) {
    if (meshView.IsShown()) {
      meshView.Activate(s_cam);

      // First pass
      fbo_buffer1.Bind();

      glEnable(GL_CULL_FACE);
      glEnable(GL_DEPTH_TEST);
      glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
      depthMesh1.Render(s_cam);
      fbo_buffer1.Unbind();

      // Second pass
      fbo_buffer2.Bind();

      glEnable(GL_CULL_FACE);
      glEnable(GL_DEPTH_TEST);
      glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
      depthMesh2.Render(s_cam);
      fbo_buffer2.Unbind();

      // Final pass
      glDisable(GL_CULL_FACE);
      glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

      finalPassShader.Bind();
      glActiveTexture(GL_TEXTURE0);
      color_buffer2.Bind();
      quad->draw();
      finalPassShader.Unbind();

      glDisable(GL_CULL_FACE);
    }

    pangolin::FinishFrame();
  }

  return 0;
}

