// Copyright (c) Facebook, Inc. and its affiliates. All Rights Reserved
#define cimg_display 0

#include <EGL.h>
#include <PTexLib.h>
#include <string>
#include <pangolin/image/image_convert.h>
#include <Eigen/Geometry>
#include "MirrorRenderer.h"
// #include "CImg.h"
#include <chrono>
#include <random>
#include <iterator>
#include <iostream>
#include <fstream>
#include <DepthMeshLib.h>

using namespace std::chrono;

int main(int argc, char* argv[]) {

  auto model_start = high_resolution_clock::now();

  ASSERT(argc == 5 || argc == 6, "Usage: ./ReplicaViewer mesh.ply textures [glass.sur] cameraPositions.txt [y if spherical]");

  const std::string meshFile(argv[1]);
  const std::string atlasFolder(argv[2]);
  ASSERT(pangolin::FileExists(meshFile));
  ASSERT(pangolin::FileExists(atlasFolder));

  bool navCam = true;
  std::string navPositions;
  std::string surfaceFile;

  bool spherical = false;

  if (argc == 5) {
    navPositions = std::string(argv[4]);
    if(navPositions.length()==1){
      spherical = true;
      navPositions = std::string(argv[3]);
      ASSERT(pangolin::FileExists(navPositions));

    }else{

      surfaceFile = std::string(argv[3]);
      ASSERT(pangolin::FileExists(surfaceFile));
      ASSERT(pangolin::FileExists(navPositions));

    }
  }

  if (argc == 6) {
    spherical = true;
    surfaceFile = std::string(argv[3]);
    ASSERT(pangolin::FileExists(surfaceFile));

    navPositions = std::string(argv[4]);
    ASSERT(pangolin::FileExists(navPositions));
  }

  // load dataset generating txt file
  std::vector<std::vector<float>> cameraPos;
  if(navCam){
    std::fstream in(navPositions);
    std::string line;
    int i=0;
    while(std::getline(in,line)){
      float value;
      std::stringstream ss(line);
      cameraPos.push_back(std::vector<float>());

      while(ss>>value){
      cameraPos[i].push_back(value);
      }
      ++i;
    }
  }

  int width = 640;
  int height = 320;

  // Downsample images
  int width_ds = 540;
  int height_ds = 270;
  if(spherical){
    width = 4096;
    height = 2048;

    // width = 640;
    // height = 320;

    width_ds = 640;
    height_ds = 320;
  }

  bool renderDepth = true;
  bool renderDepthOnly = true;
  float depthScale = 65535.0f * 0.1f;

  // Setup EGL
  EGLCtx egl;

  egl.PrintInformation();

  //Don't draw backfaces
  GLenum frontFace = GL_CW;

  if(spherical){
    glFrontFace(frontFace);
  }
  else{
    frontFace = GL_CCW;
    glFrontFace(frontFace);

  }

  // Setup a framebuffer
  pangolin::GlTexture render(width, height);
  pangolin::GlRenderBuffer renderBuffer(width, height);
  pangolin::GlFramebuffer frameBuffer(render, renderBuffer);

  pangolin::GlTexture depthTexture(width, height);
  pangolin::GlFramebuffer depthFrameBuffer(depthTexture, renderBuffer);

  // Setup a camera
  std::vector<float> initCam = {0,0.5,-0.6230950951576233};
  if(navCam){
    initCam = cameraPos[0];
    std::cout<<"Initial camera parameters:"<<initCam[0]<<" "<<initCam[1]<<" "<<initCam[2];
  }
  int cx = rand()%4;
  int cy = rand()%4;

  pangolin::OpenGlRenderState s_cam(
      pangolin::ProjectionMatrixRDF_BottomLeft(
          width,
          height,
          width / 2.0f,
          width / 2.0f,
          (width - 1.0f) / 2.0f,
          (height - 1.0f) / 2.0f,
          0.1f,
          100.0f),
      pangolin::ModelViewLookAtRDF(initCam[0],initCam[1],initCam[2], 0, 0, initCam[2], 0, 0, 1));

  // Start at some origin
  Eigen::Matrix4d T_camera_world = s_cam.GetModelViewMatrix();

  // And rotate by 90 degree for each face of the cubemap
  Eigen::Transform<double,3,Eigen::Affine> t(Eigen::AngleAxis<double>(0.5*M_PI,Eigen::Vector3d::UnitY()));
  Eigen::Transform<double,3,Eigen::Affine> u(Eigen::AngleAxis<double>(0.5*M_PI,Eigen::Vector3d::UnitX()));
  Eigen::Transform<double,3,Eigen::Affine> d(Eigen::AngleAxis<double>(M_PI,Eigen::Vector3d::UnitX()));

  Eigen::Matrix4d R_side=Eigen::Matrix4d::Identity();
  Eigen::Matrix4d R_up=Eigen::Matrix4d::Identity();
  Eigen::Matrix4d R_down=Eigen::Matrix4d::Identity();

  R_side=t.matrix();
  R_up=u.matrix();
  R_down=d.matrix();

  //move to the left for new spot
  Eigen::Matrix4d T_new_old = Eigen::Matrix4d::Identity();
  T_new_old.topRightCorner(3, 1) = Eigen::Vector3d(1, 0, 0);

  Eigen::Matrix4d T_stereo = Eigen::Matrix4d::Identity();
  T_stereo.topRightCorner(3, 1) = Eigen::Vector3d(0.2, 0, 0);

  // load mirrors
  std::vector<MirrorSurface> mirrors;
  if (surfaceFile.length()) {
    std::ifstream file(surfaceFile);
    picojson::value json;
    picojson::parse(json, file);

    for (size_t i = 0; i < json.size(); i++) {
      mirrors.emplace_back(json[i]);
    }
    std::cout << "Loaded " << mirrors.size() << " mirrors" << std::endl;
  }

  const std::string shadir = STR(SHADER_DIR);
  MirrorRenderer mirrorRenderer(mirrors, width, height, shadir);

  // load mesh and textures
  PTexMesh ptexMesh(meshFile, atlasFolder, spherical);

  pangolin::ManagedImage<Eigen::Matrix<uint8_t, 3, 1>> image(width, height);
  pangolin::ManagedImage<Eigen::Matrix<uint8_t, 3, 1>> depthImage(width, height);


  // Depth-based re-rendering
  std::shared_ptr<Shape> quad = DepthMesh::GenerateMeshData(width, height, spherical);

  // pangolin::ManagedImage<float> depthImage(width, height);
  pangolin::ManagedImage<uint16_t> depthImageInt(width, height);

  // Render 6 frames for cubemap for each spot
  size_t numSpots = 20;
  if(navCam){
    numSpots = cameraPos.size();
  }

  srand(time(0));

  // rendering the dataset (double equirect pair + interpolation + extrapolation + forward extrapolation)
  for(size_t j=0; j<numSpots;j++){
      //get the modelview matrix with the navigable camera position specified in the text file
      Eigen::Matrix4d spot_cam_to_world = s_cam.GetModelViewMatrix();

      // rendering scheme
      // 0,1,2: input spot
      // 3,4,5: interpolation spot
      // 6,7,8: extrapolation spot
      // 9,10,11: extrapolation spot
      // 12,13,14: gt depth for the erp in three tgt position
      for(int k =0; k<12;k++){
        int which_spot = k / 3;
        int eye= k % 3;
        float basel = cameraPos[j][3];
        if(which_spot == 1){
          // interpolate frame to the right
          Eigen::Matrix4d T_translate = Eigen::Matrix4d::Identity();
          T_translate.topRightCorner(3, 1) = Eigen::Vector3d(cameraPos[j][4], cameraPos[j][5], cameraPos[j][6]);
          T_camera_world = T_translate.inverse() * spot_cam_to_world ;
          s_cam.GetModelViewMatrix() = T_camera_world;

        }
        else if(which_spot == 2){
          // extrapolate frame to the right (?)
          Eigen::Matrix4d T_translate = Eigen::Matrix4d::Identity();
          T_translate.topRightCorner(3, 1) = Eigen::Vector3d(cameraPos[j][7], cameraPos[j][8], cameraPos[j][9]);
          T_camera_world = T_translate.inverse() * spot_cam_to_world ;
          s_cam.GetModelViewMatrix() = T_camera_world;
        }
        else if(which_spot == 3){
          // extrapolate frame to the left (?)
          Eigen::Matrix4d T_translate = Eigen::Matrix4d::Identity();
          T_translate.topRightCorner(3, 1) = Eigen::Vector3d(cameraPos[j][10], cameraPos[j][11], cameraPos[j][12]);
          T_camera_world = T_translate.inverse() * spot_cam_to_world ;
          s_cam.GetModelViewMatrix() = T_camera_world;
        }

        // Render
        frameBuffer.Bind();
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

        glPushAttrib(GL_VIEWPORT_BIT);
        glViewport(0, 0, width, height);
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
        glEnable(GL_CULL_FACE);

        ptexMesh.SetExposure(0.01);

        if(eye != 2){
          ptexMesh.SetBaseline(basel);
        }
        if(spherical){
          ptexMesh.Render(s_cam,Eigen::Vector4f(0.0f, 0.0f, 0.0f, 0.0f),eye);
        }else{
          ptexMesh.Render(s_cam,Eigen::Vector4f(0.0f, 0.0f, 0.0f, 0.0f));
        }
        glDisable(GL_CULL_FACE);

        glPopAttrib(); //GL_VIEWPORT_BIT
        frameBuffer.Unbind();

        // Download and save
        render.Download(image.ptr, GL_RGB, GL_UNSIGNED_BYTE);
        if(spherical){

          char equirectFilename[1000];
          snprintf(equirectFilename, 1000, "/home/selenaling/Desktop/20191113_6DoF_TestSet/jpeg_%dx%d/%s_%04zu_pos%02zu.jpeg",
          width,height,navPositions.substr(0,navPositions.length()-9).c_str(),j,k);

          pangolin::SaveImage(
              image.UnsafeReinterpret<uint8_t>(),
              pangolin::PixelFormatFromString("RGB24"),
              std::string(equirectFilename), 100.0);

        }
        else{
          char cmapFilename[1000];
          snprintf(cmapFilename, 1000, "/media/battal/celsius-data/Replica-Dataset/cubemapData/%s_%04zu_pos%01zu.jpeg",navPositions.substr(0,navPositions.length()-9).c_str(),j,k);
          pangolin::SaveImage(
              image.UnsafeReinterpret<uint8_t>(),
              pangolin::PixelFormatFromString("RGB24"),
              std::string(cmapFilename));
        }

        if( ( renderDepth || renderDepthOnly ) && k==2){

            depthFrameBuffer.Bind();
            glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

            glPushAttrib(GL_VIEWPORT_BIT);
            glViewport(0, 0, width, height);
            glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
            glEnable(GL_CULL_FACE);

            ptexMesh.RenderDepth(s_cam, 1.f/0.5f, Eigen::Vector4f(0.0f, 0.0f, 0.0f, 0.0f),eye);

            glDisable(GL_CULL_FACE);

            glPopAttrib(); //GL_VIEWPORT_BIT
            depthFrameBuffer.Unbind();

            depthTexture.Download(depthImage.ptr, GL_RGB, GL_UNSIGNED_BYTE);

            char filename[1000];
            snprintf(filename, 1000, "/home/selenaling/Desktop/20191113_6DoF_TestSet/jpeg_%dx%d/%s_%04zu_pos%02zu.jpeg",
            width,height,navPositions.substr(0,navPositions.length()-9).c_str(),j, 12); //11+(k-2)/3 map 5-12; 8-13; 11-14
             pangolin::SaveImage(
                depthImage.UnsafeReinterpret<uint8_t>(),
                pangolin::PixelFormatFromString("RGB24"),
                std::string(filename));

        }
      }


      if(navCam){
        if(j+1<numSpots){
          int cx = rand()%4;
          int cy = rand()%4;
          s_cam.SetModelViewMatrix(pangolin::ModelViewLookAtRDF(cameraPos[j+1][0],cameraPos[j+1][1],cameraPos[j+1][2], cx, cy, cameraPos[j+1][2], 0, 0, 1));
        }
      }else{
        continue;
      }

      std::cout << "\r Spot " << j + 1  << "/" << numSpots << std::endl;
  }
  auto model_stop = high_resolution_clock::now();
  auto model_duration = duration_cast<microseconds>(model_stop - model_start);
  std::cout << "Time taken rendering the model "<<navPositions.substr(0,navPositions.length()-9).c_str()<<": "<< model_duration.count() << " microseconds" << std::endl;
  return 0;
}
