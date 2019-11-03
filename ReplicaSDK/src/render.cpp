// Copyright (c) Facebook, Inc. and its affiliates. All Rights Reserved
#include <EGL.h>
#include <PTexLib.h>
#include <string>
#include <pangolin/image/image_convert.h>
#include <Eigen/Geometry>
#include "MirrorRenderer.h"
#include <chrono>
#include <random>
#include <iterator>
#include <iostream>
#include <fstream>
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

  int width = 1920;
  int height = 1080;
  if(spherical){
    width = 4096;
    height = 2048;
  }

  bool renderDepth = true;
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
  // int cx = 1;
  // int cy = 1;

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
      pangolin::ModelViewLookAtRDF(initCam[0],initCam[1],initCam[2], cx, cy, initCam[2], 0, 0, 1));

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
      // eye:              0   1    0     1      0      1     0
      // baseline radius:  bl  bl inter inter extra1 extra2  0

      for(int k =0; k<7;k++){

        int eye = 2；
        float basel = cameraPos[j][3];
        Eigen::Matrix4d T_translate = Eigen::Matrix4d::Identity();
        if(k==0){
          // src/ref baseline
          T_translate.topRightCorner(3, 1) = Eigen::Vector3d(basel, 0, 0);
          T_camera_world = T_translate.inverse() * spot_cam_to_world ;
          s_cam.GetModelViewMatrix() = T_camera_world;
        }
        else if(k==1){
          // src/ref baseline
          T_translate.topRightCorner(3, 1) = Eigen::Vector3d(-basel, 0, 0);
          T_camera_world = T_translate.inverse() * spot_cam_to_world ;
          s_cam.GetModelViewMatrix() = T_camera_world;
        }
        else if(k==2){
          // interpolate frame to the right
          basel = cameraPos[j][4];
          T_translate.topRightCorner(3, 1) = Eigen::Vector3d(basel, 0, 0);
          T_camera_world = T_translate.inverse() * spot_cam_to_world ;
          s_cam.GetModelViewMatrix() = T_camera_world;
        }
        else if(k==3){
          // interpolate frame to the left
          basel = cameraPos[j][4];
          T_translate.topRightCorner(3, 1) = Eigen::Vector3d(-basel, 0, 0);
          T_camera_world = T_translate.inverse() * spot_cam_to_world ;
          s_cam.GetModelViewMatrix() = T_camera_world;
        }
        else if(k==4){
          // extrapolate frame to the right
          basel = cameraPos[j][5];
          T_translate.topRightCorner(3, 1) = Eigen::Vector3d(basel, 0, 0);
          T_camera_world = T_translate.inverse() * spot_cam_to_world ;
          s_cam.GetModelViewMatrix() = T_camera_world;
        }
        else if(k==5){
          // extrapolate frame to the left
          basel = cameraPos[j][6];
          T_translate.topRightCorner(3, 1) = Eigen::Vector3d(-basel, 0, 0);
          T_camera_world = T_translate.inverse() * spot_cam_to_world ;
          s_cam.GetModelViewMatrix() = T_camera_world;
        }
        else if(k==6){
          s_cam.GetModelViewMatrix() = spot_cam_to_world;
        }

        auto frame_start = high_resolution_clock::now();

        std::cout << "\rRendering position " << k + 1 << "/" << 8 << "... with baseline" << basel << "and with eye " << eye << std::endl;

        // Render
        frameBuffer.Bind();
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

        glPushAttrib(GL_VIEWPORT_BIT);
        glViewport(0, 0, width, height);
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
        glEnable(GL_CULL_FACE);

        ptexMesh.SetExposure(0.01);
        ptexMesh.SetBaseline(basel);
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
          snprintf(equirectFilename, 1000, "/home/selenaling/Desktop/Replica-Dataset/build/ReplicaSDK/equirectData/train-data-360-tgt-depth/%s_%04zu_pos%01zu.jpeg",navPositions.substr(0,navPositions.length()-4).c_str(),j,k);

          pangolin::SaveImage(
              image.UnsafeReinterpret<uint8_t>(),
              pangolin::PixelFormatFromString("RGB24"),
              std::string(equirectFilename), 100.0);

        }
        else{
          char cmapFilename[1000];
          snprintf(cmapFilename, 1000, "/home/selenaling/Desktop/Replica-Dataset/build/ReplicaSDK/cubemapData/%s_%04zu_pos%01zu.png",navPositions.substr(0,navPositions.length()-4).c_str(),j,k);

          pangolin::SaveImage(
              image.UnsafeReinterpret<uint8_t>(),
              pangolin::PixelFormatFromString("RGB24"),
              std::string(cmapFilename));
        }

        auto frame_stop = high_resolution_clock::now();
        auto frame_duration = duration_cast<microseconds>(frame_stop - frame_start);

        std::cout << "Time taken rendering the image: " << frame_duration.count() << " microseconds" << std::endl;
        if(renderDepth && k==6){

            depthFrameBuffer.Bind();
            glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

            glPushAttrib(GL_VIEWPORT_BIT);
            glViewport(0, 0, width, height);
            glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

            glEnable(GL_CULL_FACE);

            ptexMesh.RenderDepth(s_cam,1.f/0.5f,Eigen::Vector4f(0.0f, 0.0f, 0.0f, 0.0f),eye);

            glDisable(GL_CULL_FACE);

            glPopAttrib(); //GL_VIEWPORT_BIT
            depthFrameBuffer.Unbind();

            depthTexture.Download(depthImage.ptr, GL_RGB, GL_UNSIGNED_BYTE);

            char filename[1000];
            snprintf(filename, 1000, "/home/selenaling/Desktop/Replica-Dataset/build/ReplicaSDK/equirectData/test-data-tgt-depth/%s_%04zu_pos%01zu.jpeg",navPositions.substr(0,navPositions.length()-4).c_str(),j,k+1);
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


  std::cout << "\rRendering spot " << numSpots << "/" << numSpots << "... done" << std::endl;
  auto model_stop = high_resolution_clock::now();
  auto model_duration = duration_cast<microseconds>(model_stop - model_start);

  std::cout << "Time taken rendering the model: " << model_duration.count() << " microseconds" << std::endl;

  return 0;
}
