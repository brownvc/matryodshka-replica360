// Copyright (c) Facebook, Inc. and its affiliates. All Rights Reserved
#include <EGL.h>
#include <DepthMeshLib.h>
#include <ctime>
#include <pangolin/display/display.h>
#include <pangolin/display/widgets/widgets.h>
#include <bits/stdc++.h>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>

std::vector<std::string> split(std::string str, char delimiter) {
  std::vector<std::string> internal;
  std::stringstream ss(str); // Turn the string into a stream.
  std::string tok;

  while(std::getline(ss, tok, delimiter)) {
    internal.push_back(tok);
  }

  return internal;
}

int main(int argc, char* argv[]) {
  // Command line args
  ASSERT(argc==10, "Usage: ./ReplicaViewer TEST_FILES CAMERA_POSES BASELINES OUT_DIR SPHERICAL ODS JUMP WIDTH HEIGHT");

  const std::string data_file = std::string(argv[1]);
  const std::string camera_poses_file(argv[2]);
  const std::string baselines_file(argv[3]);
  const std::string out_dir = std::string(argv[4]);

  bool spherical = std::string(argv[5]).compare(std::string("y")) == 0;
  bool ods = std::string(argv[6]).compare(std::string("y")) == 0;
  bool jump = std::string(argv[7]).compare(std::string("y")) == 0;
  bool fixed_bl = std::string(argv[3]).compare(std::string("n")) == 0;

  // Setup OpenGL Display (based on GLUT)
  int width = std::stoi(std::string(argv[8]));
  int height = std::stoi(std::string(argv[9]));
  std::cout << width << std::endl;
  std::cout << height << std::endl;

  // Get camera poses
  std::fstream in_cam(camera_poses_file);
  std::string line;
  std::vector<std::vector<float>> camera_poses;

  int c = 0;

  while(std::getline(in_cam,line)){
    float value;
    std::stringstream ss(line);
    camera_poses.push_back(std::vector<float>());

    while(ss >> value){
      camera_poses[c].push_back(value);
    }

    ++c;
  }

  // Get baselines
  std::fstream in_bas(baselines_file);
  std::vector<std::vector<float>> baselines;

  if(!fixed_bl){
    c = 0;

    while(std::getline(in_bas,line)){
      float value;
      std::stringstream ss(line);
      baselines.push_back(std::vector<float>());

      while(ss >> value){
        baselines[c].push_back(value);
      }

      ++c;
    }

  }

  // Get test files
  std::fstream in_data(data_file);
  std::vector<std::vector<std::string>> test_files;

  while(std::getline(in_data, line)){
    std::vector<std::string> files = split(line, ' ');
    test_files.push_back(files);
  }

  ASSERT(test_files.size() == camera_poses.size(), "Number of provided examples and camera poses must be equal");

  // Setup EGL
  EGLCtx egl;
  egl.PrintInformation();

  // Setup default OpenGL parameters
  glEnable(GL_DEPTH_TEST);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  const GLenum frontFace = GL_CCW;
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

  // Save default model view matrix
  Eigen::Matrix4d spot_cam_to_world = s_cam.GetModelViewMatrix();

  for(int i = 0; i < test_files.size(); i++) {
    // Transform
    Eigen::Matrix4d T_translate = Eigen::Matrix4d::Identity();
    // T_translate.topRightCorner(3, 1) = Eigen::Vector3d(camera_poses[i][0], camera_poses[i][1], camera_poses[i][2]);
    T_translate.topRightCorner(3, 1) = Eigen::Vector3d(camera_poses[i][0], camera_poses[i][1], -camera_poses[i][2]);
    // T_translate.topRightCorner(3, 1) = Eigen::Vector3d(camera_poses[i][2], camera_poses[i][2], camera_poses[i][0]);

    Eigen::Matrix4d T_camera_world = T_translate.inverse() * spot_cam_to_world;
    s_cam.GetModelViewMatrix() = T_camera_world;

    // Generate mesh
    DepthMesh depthMesh(
        quad,
        test_files[i][0], test_files[i][1], "", false, spherical, ods, true, jump);

    if(!fixed_bl){
      depthMesh.SetBaseline(baselines[i][0]);
    }

    // Render
    fbo.Bind();

    glPushAttrib(GL_VIEWPORT_BIT);
    glViewport(0, 0, width, height);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    glEnable(GL_CULL_FACE);

    // depthMesh.SetBaseline(0.0789946717611322);
    depthMesh.Render(s_cam);

    glDisable(GL_CULL_FACE);

    fbo.Unbind();

    // Write image
    pangolin::ManagedImage<Eigen::Matrix<uint8_t, 3, 1>> image(width, height);
    color_buffer.Download(image.ptr, GL_RGB, GL_UNSIGNED_BYTE);

    char dir[1000];
    snprintf(dir, 1000, "mkdir %s%04d", out_dir.c_str(), i);
    std::cout<<"Create new directory "<<dir<<std::endl;
    int status = system(dir);

    char filename[1000];
    snprintf(filename, 1000, "%s/%04d/output_tgt_%04d.png", out_dir.c_str(), i, i);

    pangolin::SaveImage(
        image.UnsafeReinterpret<uint8_t>(),
        pangolin::PixelFormatFromString("RGB24"),
        filename);
  }

  return 0;
}
