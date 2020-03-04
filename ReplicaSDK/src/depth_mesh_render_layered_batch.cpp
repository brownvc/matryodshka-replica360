// Copyright (c) Facebook, Inc. and its affiliates. All Rights Reserved
#include <EGL.h>
#include <DepthMeshLib.h>
#include <ctime>
#include <pangolin/display/display.h>
#include <pangolin/display/widgets/widgets.h>

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
  ASSERT(argc==7, "Usage: ./ReplicaViewer TEST_FILES CAMERA_POSES OUT_DIR SPHERICAL WIDTH HEIGHT");

  const std::string data_file = std::string(argv[1]);
  const std::string camera_poses_file(argv[2]);
  const std::string out_dir = std::string(argv[3]);
  bool spherical = std::string(argv[4]).compare(std::string("y")) == 0;

  // Setup OpenGL Display (based on GLUT)
  int width = std::stoi(std::string(argv[5]));
  int height = std::stoi(std::string(argv[6]));
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

  // FBOs
  pangolin::GlTexture color_buffer1(width, height);
  pangolin::GlRenderBuffer depth_buffer1(width, height);
  pangolin::GlFramebuffer fbo1(color_buffer1, depth_buffer1);

  pangolin::GlTexture color_buffer2(width, height);
  pangolin::GlRenderBuffer depth_buffer2(width, height);
  pangolin::GlFramebuffer fbo2(color_buffer2, depth_buffer2);

  pangolin::GlTexture color_buffer3(width, height);
  pangolin::GlRenderBuffer depth_buffer3(width, height);
  pangolin::GlFramebuffer fbo3(color_buffer3, depth_buffer3);
  std::shared_ptr<Shape> quad = DepthMesh::GenerateMeshData(width, height, spherical);

  // Save default model view matrix
  Eigen::Matrix4d spot_cam_to_world = s_cam.GetModelViewMatrix();

  for(int i = 0; i < test_files.size(); i++) {
    // Transform
    Eigen::Matrix4d T_translate = Eigen::Matrix4d::Identity();
    T_translate.topRightCorner(3, 1) = Eigen::Vector3d(camera_poses[i][0], camera_poses[i][1], camera_poses[i][2]);
    Eigen::Matrix4d T_camera_world = T_translate.inverse() * spot_cam_to_world;
    s_cam.GetModelViewMatrix() = T_camera_world;

    // Generate meshes
    ASSERT(pangolin::FileExists(test_files[i][7]));

    DepthMesh depthMeshInp(
        quad,
        test_files[i][2], test_files[i][5], "", true, spherical, false, true, false);
    DepthMesh depthMeshBg(
        quad,
        test_files[i][1], test_files[i][4], test_files[i][7], true, spherical, false, false, false);
    DepthMesh depthMeshFg(
        quad,
        test_files[i][0], test_files[i][3], test_files[i][6], true, spherical, false, false, false);

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
    color_buffer3.Download(image.ptr, GL_RGB, GL_UNSIGNED_BYTE);

    pangolin::SaveImage(
        image.UnsafeReinterpret<uint8_t>(),
        pangolin::PixelFormatFromString("RGB24"),
        out_dir + "/im" + std::to_string(i) + ".png");


    /*
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
        out_dir + "/im" + std::to_string(i) + ".png");
    */
  }

  return 0;
}


