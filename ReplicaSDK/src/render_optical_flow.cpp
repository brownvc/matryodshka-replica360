
// Copyright (c) Facebook, Inc. and its affiliates. All Rights Reserved
#define cimg_display 0

#include <PTexLib.h>
#include "MirrorRenderer.h"
#include <DepthMeshLib.h>

#include <EGL.h>
#include <pangolin/image/image_convert.h>
#include <Eigen/Geometry>

#include <string>
#include <iostream>
#include <ctime>
#include <chrono>
#include <random>
#include <iterator>
#include <fstream>
#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;
using namespace std::chrono;

void output_pixel_motion_vector(const char *filename, const void *ptr, const int width, const int height)
{
  std::ofstream flo_file(filename, std::ios::binary);
  if (!flo_file)
  {
    return;
  }
  else
  {
    flo_file << "PIEH";
    flo_file.write((char *)&width, sizeof(int));
    flo_file.write((char *)&height, sizeof(int));
    for (int i = 0; i < width * height; i++)
    {
      flo_file.write(static_cast<const char *>(ptr + sizeof(float) * 4 * i), 2 * sizeof(float));
    }
  }
  flo_file.close();
}

// write the depth map to a .dpt file (Sintel format).
void output_depth_map_dpt(const char *filename, const void *ptr, const int width, const int height)
{
  if (filename == nullptr)
  {
    std::cout << "Error in " << __FUNCTION__ << ": empty filename.";
    return;
  }

  const char *dot = strrchr(filename, '.');
  if (dot == nullptr)
  {
    std::cout << "Error in " << __FUNCTION__ << ": extension required in filename " << filename << ".";
    return;
  }

  if (strcmp(dot, ".dpt") != 0)
  {
    std::cout << "Error in " << __FUNCTION__ << ": filename " << filename << " should have extension '.dpt'.";
    return;
  }

  FILE *stream = fopen(filename, "wb");
  if (stream == 0)
  {
    std::cout << "Error in " << __FUNCTION__ << ": could not open " << filename;
    return;
  }

  // write the header
  fprintf(stream, "PIEH");
  if ((int)fwrite(&width, sizeof(int), 1, stream) != 1 || (int)fwrite(&height, sizeof(int), 1, stream) != 1)
  {
    std::cout << "Error in writeSintelDptFile(" << filename << "): problem writing header.";
    return;
  }

  // write the depth data
  fwrite(ptr, 1, width * height * sizeof(float), stream);

  fclose(stream);
}

void load_mv(const std::string &navPositions,
             std::vector<std::vector<float>> &cameraPos,
             std::vector<pangolin::OpenGlMatrix> &cameraMV)
{
  std::fstream in(navPositions);
  std::string line;
  int i = 0;
  while (std::getline(in, line))
  {
    float value;
    std::stringstream ss(line);
    cameraPos.push_back(std::vector<float>());

    while (ss >> value)
    {
      cameraPos[i].push_back(value);
    }

    // transform the camera position and rotation parameter to MV matrix
    Eigen::Matrix3f model_mat;
    model_mat = Eigen::AngleAxisf(cameraPos[i][6] / 180.0 * M_PI, Eigen::Vector3f::UnitZ()) * 
                Eigen::AngleAxisf(cameraPos[i][5] / 180.0 * M_PI, Eigen::Vector3f::UnitY()) * 
                Eigen::AngleAxisf(cameraPos[i][4] / 180.0 * M_PI, Eigen::Vector3f::UnitX());

    Eigen::Vector3f eye_point(cameraPos[i][1], cameraPos[i][2], cameraPos[i][3]);
    Eigen::Vector3f target_point = eye_point + model_mat * Eigen::Vector3f(1, 0, 0);
    Eigen::Vector3f up = Eigen::Vector3f::UnitZ();

    // +x right, -y up, +z forward
    pangolin::OpenGlMatrix mv = pangolin::ModelViewLookAtRDF(
        eye_point[0], eye_point[1], eye_point[2],
        target_point[0], target_point[1], target_point[2],
        up[0], up[1], up[2]);

    cameraMV.push_back(mv);

    ++i;
  }

  std::stringstream ss;
  ss << "there are :" << i << " cameras pose in the file :" << navPositions << std::endl;
  std::cout << ss.str() << std::endl;
}

int main(int argc, char *argv[])
{
  // render enable
  bool rgb_image_enable = true;
  bool depth_map_enable = true;
  bool optical_flow_enable = true;
  auto model_start = high_resolution_clock::now();

  ASSERT(argc == 9, "Usage: ./Path/to/ReplicaViewer mesh.ply textures glass.sur[glass.sur/n] cameraPositions.txt[file.txt/n] spherical[y/n] outputDir width height");

  const std::string meshFile(argv[1]);
  ASSERT(pangolin::FileExists(meshFile));

  const std::string atlasFolder(argv[2]);
  ASSERT(pangolin::FileExists(atlasFolder));

  bool noSurfaceFile = std::string(argv[3]).compare(std::string("n")) == 0 || !pangolin::FileExists(std::string(argv[3]));

  // the camera position input file
  bool noTxtFile = std::string(argv[4]).compare(std::string("n")) == 0 || !pangolin::FileExists(std::string(argv[4]));
  ASSERT(!noTxtFile, "This code only render with camera position txt file.");

  bool spherical = std::string(argv[5]).compare(std::string("y")) == 0;
  ASSERT(spherical, "This code only render spherical images.");

  std::string outputDir(argv[6]);
  std::cout << "output director:" << outputDir << std::endl;
  ASSERT(pangolin::FileExists(outputDir));

  int width = std::stoi(std::string(argv[7]));
  int height = std::stoi(std::string(argv[8]));
  ASSERT(width == 2 * height, "the width should be the 2 times of height");

  // get scene name
  std::string scene;
  const size_t last_slash_idx = meshFile.rfind("/");
  const size_t second2last_slash_idx = meshFile.substr(0, last_slash_idx).rfind("/");
  if (std::string::npos != last_slash_idx)
  {
    scene = meshFile.substr(second2last_slash_idx + 1, last_slash_idx - second2last_slash_idx - 1);
    std::cout << "Generating from scene " << scene << std::endl;
  }

  // create output dir
  ASSERT(pangolin::FileExists(outputDir));

  // load surface file
  std::string surfaceFile;
  if (!noSurfaceFile)
  {
    surfaceFile = std::string(argv[3]);
    ASSERT(pangolin::FileExists(surfaceFile));
  }

  // camera trajectory
  std::string navPositions;
  bool navCam = !noTxtFile;
  if (!noTxtFile)
  {
    navPositions = std::string(argv[4]);
    ASSERT(pangolin::FileExists(navPositions));
  }

  // load camera trajectory from txt file for data generation
  std::vector<std::vector<float>> cameraPos;
  std::vector<pangolin::OpenGlMatrix> cameraMV;
  if (navCam)
  {
    load_mv(navPositions, cameraPos, cameraMV);
  }

  // Setup OpenGl context
  // Setup EGL, render with out window
  EGLCtx egl;
  egl.PrintInformation();

  if (glewInit() != GLEW_OK)
  {
    pango_print_error("Unable to initialize GLEW.");
  }

  // Setup default OpenGL parameters
  glEnable(GL_DEPTH_TEST);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  const GLenum frontFace = GL_CW;
  glFrontFace(frontFace);
  glLineWidth(1.0f);

  // Setup a framebuffer
  pangolin::GlRenderBuffer renderBuffer(width, height);

  pangolin::GlTexture rgbTexture(width, height);
  pangolin::GlFramebuffer frameBuffer(rgbTexture, renderBuffer); // to render rgb image

  pangolin::GlTexture depthTexture(width, height, GL_R32F, true, 0, GL_RED, GL_FLOAT);
  pangolin::GlFramebuffer depthFrameBuffer(depthTexture, renderBuffer); // to render depth image

  pangolin::GlTexture opticalflowTexture(width, height, GL_RGBA32F);
  pangolin::GlFramebuffer opticalflowFrameBuffer(opticalflowTexture, renderBuffer); // to render motion flow image

  // load mirrors
  std::vector<MirrorSurface> mirrors;
  if (!noSurfaceFile && surfaceFile.length() > 1)
  {
    std::ifstream file(surfaceFile);
    picojson::value json;
    picojson::parse(json, file);

    for (size_t i = 0; i < json.size(); i++)
    {
      mirrors.emplace_back(json[i]);
    }
    std::cout << "Loaded " << mirrors.size() << " mirrors" << std::endl;
  }

  const std::string shadir = STR(SHADER_DIR);
  MirrorRenderer mirrorRenderer(mirrors, width, height, shadir);

  // load mesh and textures
  bool opticalFlow = true;
  PTexMesh ptexMesh(meshFile, atlasFolder, spherical, opticalFlow);
  pangolin::ManagedImage<Eigen::Matrix<uint8_t, 3, 1>> image(width, height);
  pangolin::ManagedImage<Eigen::Matrix<float, 1, 1>> depthImage(width, height);
  pangolin::ManagedImage<Eigen::Matrix<float, 4, 1>> opticalFlow_forward(width, height);
  pangolin::ManagedImage<Eigen::Matrix<float, 4, 1>> opticalFlow_backward(width, height);

  // Setup a camera
  pangolin::OpenGlRenderState s_cam_current;
  pangolin::OpenGlRenderState s_cam_next;

  // video frame rendering scheme: rgb, depth, optical flow
  size_t numFrames = cameraPos.size();
  for (size_t j = 0; j < numFrames; j++)
  {
    std::cout << "\r Spot " << j + 1 << "/" << numFrames << std::endl;

    // load camera MV matrix
    s_cam_current.SetModelViewMatrix(cameraMV[j]);
    s_cam_next.SetModelViewMatrix(cameraMV[(j + 1) % numFrames]);

    // render rgb images
    if (rgb_image_enable)
    {
      frameBuffer.Bind();
      glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
      glPushAttrib(GL_VIEWPORT_BIT);
      glViewport(0, 0, width, height);
      glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
      glDisable(GL_CULL_FACE);
      ptexMesh.SetExposure(0.01);
      ptexMesh.SetBaseline(0);
      ptexMesh.Render(s_cam_current, Eigen::Vector4f(0.0f, 0.0f, 0.0f, 0.0f), 2);
      glEnable(GL_CULL_FACE);
      glPopAttrib(); //GL_VIEWPORT_BIT
      frameBuffer.Unbind();

      // Download and save
      rgbTexture.Download(image.ptr, GL_RGB, GL_UNSIGNED_BYTE);

      char equirectFilename[1000];
      snprintf(equirectFilename, 1000, "%s/%04zu_rgb.jpg", outputDir.c_str(), j);
      pangolin::SaveImage(image.UnsafeReinterpret<uint8_t>(),
                          pangolin::PixelFormatFromString("RGB24"),
                          std::string(equirectFilename));
    }

    // render depth image and save to *.bin file
    if (depth_map_enable)
    {
      depthFrameBuffer.Bind();
      glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
      glPushAttrib(GL_VIEWPORT_BIT);
      glViewport(0, 0, width, height);
      glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
      glDisable(GL_CULL_FACE);
      ptexMesh.RenderDepth(s_cam_current, 1.f / 16.f, Eigen::Vector4f(0.0f, 0.0f, 0.0f, 0.0f), 2);
      glEnable(GL_CULL_FACE);
      glPopAttrib(); //GL_VIEWPORT_BIT
      depthFrameBuffer.Unbind();

      depthTexture.Download(depthImage.ptr, GL_RED, GL_FLOAT);

      char filename[1000];
      snprintf(filename, 1000, "%s/%04zu_depth.dpt", outputDir.c_str(), j);
      output_depth_map_dpt(filename, depthImage.ptr, width, height);
    }

    // render optical flow and save to *.flo file
    if (optical_flow_enable)
    {
      // 0) render optical flow (current frame to next frame)
      opticalflowFrameBuffer.Bind();
      glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
      glPushAttrib(GL_VIEWPORT_BIT);
      glViewport(0, 0, width, height);
      glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
      // glFrontFace(GL_CCW);      //Don't draw backfaces
      // glEnable(GL_CULL_FACE);
      glDisable(GL_CULL_FACE);
      glDisable(GL_LINE_SMOOTH);
      glDisable(GL_POLYGON_SMOOTH);
      glDisable(GL_MULTISAMPLE);
      ptexMesh.RenderOpticalFlow(s_cam_current, s_cam_next, width, height);
      glDisable(GL_CULL_FACE);
      glEnable(GL_MULTISAMPLE);
      glPopAttrib(); //GL_VIEWPORT_BIT
      opticalflowFrameBuffer.Unbind();

      opticalflowTexture.Download(opticalFlow_forward.ptr, GL_RGBA, GL_FLOAT);
      char filename[1024];
      snprintf(filename, 1024, "%s/%04zu_opticalflow_forward.flo", outputDir.c_str(), j);
      output_pixel_motion_vector(filename, opticalFlow_forward.ptr, width, height); // output optical flow to file

      // 1) render optical flow (next frame to current frame)
      opticalflowFrameBuffer.Bind();
      glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
      glPushAttrib(GL_VIEWPORT_BIT);
      glViewport(0, 0, width, height);
      glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
      // glFrontFace(GL_CCW);      //Don't draw backfaces
      // glEnable(GL_CULL_FACE);
      glDisable(GL_CULL_FACE);
      glDisable(GL_LINE_SMOOTH);
      glDisable(GL_POLYGON_SMOOTH);
      glDisable(GL_MULTISAMPLE);
      ptexMesh.RenderOpticalFlow(s_cam_next, s_cam_current, width, height);
      glDisable(GL_CULL_FACE);
      glEnable(GL_MULTISAMPLE);
      glPopAttrib(); //GL_VIEWPORT_BIT
      opticalflowFrameBuffer.Unbind();

      opticalflowTexture.Download(opticalFlow_backward.ptr, GL_RGBA, GL_FLOAT);
      snprintf(filename, 1024, "%s/%04zu_opticalflow_backward.flo", outputDir.c_str(), (j + 1) % numFrames );
      output_pixel_motion_vector(filename, opticalFlow_backward.ptr, width, height); // output optical flow to file
    }

  }

  auto model_stop = high_resolution_clock::now();
  auto model_duration = duration_cast<microseconds>(model_stop - model_start);
  std::cout << "Time taken rendering the video " << navPositions.substr(0, navPositions.length() - 9).c_str() << ": " << model_duration.count() << " microseconds" << std::endl;

  return 0;
}
