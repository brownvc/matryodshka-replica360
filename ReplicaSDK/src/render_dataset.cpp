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

  ASSERT(argc == 9, "Usage: ./Path/to/ReplicaViewer mesh.ply textures glass.sur[glass.sur/n] cameraPositions.txt[file.txt/n] spherical[y/n] outputDir width height");

  bool noSurfaceFile = std::string(argv[3]).compare(std::string("n")) == 0 || !pangolin::FileExists(std::string(argv[3]));
  bool noTxtFile = std::string(argv[4]).compare(std::string("n")) == 0 || !pangolin::FileExists(std::string(argv[4]));
  bool spherical = std::string(argv[5]).compare(std::string("y")) == 0;
  int width = std::stoi(std::string(argv[7]));
  int height = std::stoi(std::string(argv[8]));

  const std::string meshFile(argv[1]);
  const std::string atlasFolder(argv[2]);
  const std::string outputDir(argv[6]);
  ASSERT(pangolin::FileExists(meshFile));
  ASSERT(pangolin::FileExists(atlasFolder));
  ASSERT(pangolin::FileExists(outputDir));

  //get scene name
  std::string scene;
  const size_t last_slash_idx = meshFile.rfind("/");
  const size_t second2last_slash_idx = meshFile.substr(0, last_slash_idx).rfind("/");
  if (std::string::npos != last_slash_idx)
  {
    scene = meshFile.substr(second2last_slash_idx+1, last_slash_idx - second2last_slash_idx-1);
    std::cout<<"Generating from scene "<<scene<<std::endl;
  }

  std::string surfaceFile;
  if(!noSurfaceFile){
    surfaceFile = std::string(argv[3]);
    ASSERT(pangolin::FileExists(surfaceFile));
  }

  std::string navPositions;
  bool navCam = !noTxtFile;
  if(!noTxtFile){
    navPositions = std::string(argv[4]);
    ASSERT(pangolin::FileExists(navPositions));
  }

  // load txt file for data generation
  // FORMAT:
  //              camera_position_x, camera_position_y, camera_position_z, ods baseline,
  //(interpolate) translate_x,       translate_y,       translate_z,
  //(Rextrapolate)translate_x,       translate_y,       translate_z,
  //(Lextrapolate)translate_x,       translate_y,       translate_z,
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

  bool renderDepth = true;
  float depthScale = 65535.0f * 0.1f;

  // Setup EGL
  EGLCtx egl;
  egl.PrintInformation();

  //Don't draw backfaces
  GLenum frontFace = GL_CCW;
  glFrontFace(frontFace);


  // Setup a framebuffer
  pangolin::GlTexture render(width, height);
  pangolin::GlRenderBuffer renderBuffer(width, height);
  pangolin::GlFramebuffer frameBuffer(render, renderBuffer);

  pangolin::GlTexture depthTexture(width, height);
  pangolin::GlFramebuffer depthFrameBuffer(depthTexture, renderBuffer);

  // Setup a camera
  std::vector<float> initCam = {0,0.5,-0.6230950951576233};//default
  if(navCam){
    initCam = cameraPos[0];
    std::cout<<"First camera position:"<<initCam[0]<<" "<<initCam[1]<<" "<<initCam[2];
  }

  //random look at direction
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
      pangolin::ModelViewLookAtRDF(initCam[0],initCam[1],initCam[2], cx, cy, initCam[2], 0, 0, 1));

  // Start at some origin
  Eigen::Matrix4d T_camera_world = s_cam.GetModelViewMatrix();

  // For cubemap dataset: rotation matrix of 90 degree for each face of the cubemap
  // t -> t -> t -> u -> d
  Eigen::Transform<double,3,Eigen::Affine> t(Eigen::AngleAxis<double>(0.5*M_PI,Eigen::Vector3d::UnitY()));
  Eigen::Transform<double,3,Eigen::Affine> u(Eigen::AngleAxis<double>(0.5*M_PI,Eigen::Vector3d::UnitX()));
  Eigen::Transform<double,3,Eigen::Affine> d(Eigen::AngleAxis<double>(M_PI,Eigen::Vector3d::UnitX()));
  Eigen::Matrix4d R_side=Eigen::Matrix4d::Identity();
  Eigen::Matrix4d R_up=Eigen::Matrix4d::Identity();
  Eigen::Matrix4d R_down=Eigen::Matrix4d::Identity();
  R_side=t.matrix();
  R_up=u.matrix();
  R_down=d.matrix();

  // load mirrors
  std::vector<MirrorSurface> mirrors;
  if (!noSurfaceFile && surfaceFile.length()>1) {
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

  size_t numSpots = 20; //default
  if(navCam){
    numSpots = cameraPos.size();
  }
  srand(2019); //random seed

  // rendering the dataset (double equirect pair + interpolation + extrapolation + forward extrapolation)
  for(size_t j=0; j<numSpots;j++){
      //get the modelview matrix
      Eigen::Matrix4d spot_cam_to_world = s_cam.GetModelViewMatrix();
      if(!navCam){
        //no txt file supplied, render a set of left ods, right ods, equirect
        for(int eye =0; eye<3; ++eye){

          std::string type("lods");
          if(eye==1){
            type = "rods";
          }else if(eye==2){
            type = "eqr";
          }
          //Render
          frameBuffer.Bind();
          glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

          glPushAttrib(GL_VIEWPORT_BIT);
          glViewport(0, 0, width, height);
          glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
          glEnable(GL_CULL_FACE);

          //set parameters
          ptexMesh.SetExposure(0.01);
          if(eye != 2){
            ptexMesh.SetBaseline(0.032);
          }
          if(spherical){
            ptexMesh.Render(s_cam, Eigen::Vector4f(0.0f, 0.0f, 0.0f, 0.0f), eye);
          }else{
            ptexMesh.Render(s_cam, Eigen::Vector4f(0.0f, 0.0f, 0.0f, 0.0f));
          }
          glDisable(GL_CULL_FACE);
          glPopAttrib(); //GL_VIEWPORT_BIT
          frameBuffer.Unbind();

          // Download and save
          render.Download(image.ptr, GL_RGB, GL_UNSIGNED_BYTE);
          char equirectFilename[1000];
          snprintf(equirectFilename, 1000, "%s/%s_%s.jpeg", outputDir.c_str(), scene.c_str(), type.c_str());
          pangolin::SaveImage(
              image.UnsafeReinterpret<uint8_t>(),
              pangolin::PixelFormatFromString("RGB24"),
              std::string(equirectFilename), 100.0);
        }

      }
      else if(spherical){
        // double ods+eqr dataset

        // rendering scheme [left_ods, right_ods, equirect]
        // 0,1,2: input spot
        // 3,4,5: interpolation spot
        // 6,7,8: extrapolation spot
        // 9,10,11: extrapolation spot
        // 12,13,14: gt depth for the erp for three tgt position
        for(int k =0; k<12;k++){
          int which_spot = k / 3;
          int eye= k % 3;
          float basel = cameraPos[j][3];

          //translate to target position
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

          //Render
          frameBuffer.Bind();
          glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

          glPushAttrib(GL_VIEWPORT_BIT);
          glViewport(0, 0, width, height);
          glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
          glEnable(GL_CULL_FACE);

          //set parameters
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
          char equirectFilename[1000];
          snprintf(equirectFilename, 1000, "%s/%s_%04zu_pos%02zu.jpeg", outputDir.c_str(), scene.c_str(), j, k);
          pangolin::SaveImage(
              image.UnsafeReinterpret<uint8_t>(),
              pangolin::PixelFormatFromString("RGB24"),
              std::string(equirectFilename), 100.0);


          if( renderDepth && (k==2 || k==5 || k== 8 || k==11)){
              //render depth image for the equirect image
              depthFrameBuffer.Bind();
              glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

              glPushAttrib(GL_VIEWPORT_BIT);
              glViewport(0, 0, width, height);
              glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
              glEnable(GL_CULL_FACE);
              ptexMesh.RenderDepth(s_cam, 1.f/ 16.f, Eigen::Vector4f(0.0f, 0.0f, 0.0f, 0.0f), eye);
              glDisable(GL_CULL_FACE);
              glPopAttrib(); //GL_VIEWPORT_BIT

              depthFrameBuffer.Unbind();
              depthTexture.Download(depthImage.ptr, GL_RGB, GL_UNSIGNED_BYTE);

              char filename[1000];
              snprintf(filename, 1000, "%s/%s_%04zu_pos%02zu.jpeg", outputDir.c_str(), scene.c_str(), j, 11 + (k+1)/3 ); //11+(k+1)/3 maps 2-12; 5-13; 8-14; 11-15
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

      }else{
        //cubemap dataset

        //rendering scheme
        //for each face of cubemap (i)
        //k = 0,1: stereo input position
        //k = 2: interpolate target position
        //k = 3: extrapolate1 target position
        //k = 4: extrapolate2 target position
        //[if renderDepth = true]
        //k = 5,6: stereo input depth
        //k = 7: interpolate depth
        //k = 8: extrapolate depth
        //k = 9: extrapolate depth

        float basel = cameraPos[j][3];

        for(int i=0; i < 6; ++i){

          Eigen::Matrix4d face_cam_to_world = s_cam.GetModelViewMatrix();
          for(int k=0; k < 5; ++k){

            // Render
            frameBuffer.Bind();

            glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
            glPushAttrib(GL_VIEWPORT_BIT);
            glViewport(0, 0, width, height);
            glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
            glEnable(GL_CULL_FACE);
            ptexMesh.SetExposure(0.01);
            ptexMesh.Render(s_cam,Eigen::Vector4f(0.0f, 0.0f, 0.0f, 0.0f));
            glDisable(GL_CULL_FACE);
            glPopAttrib(); //GL_VIEWPORT_BIT

            frameBuffer.Unbind();

            // Download and save
            render.Download(image.ptr, GL_RGB, GL_UNSIGNED_BYTE);

            char cubemapFilename[1000];
            snprintf(cubemapFilename, 1000, "%s/%s_%04zu_pos%01zu.jpeg", outputDir.c_str(), scene.c_str(), 6*j + i, k);
            pangolin::SaveImage(
                image.UnsafeReinterpret<uint8_t>(),
                pangolin::PixelFormatFromString("RGB24"),
                std::string(cubemapFilename), 100.0);

            if(renderDepth){
              depthFrameBuffer.Bind();
              glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
              glPushAttrib(GL_VIEWPORT_BIT);
              glViewport(0, 0, width, height);
              glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
              glEnable(GL_CULL_FACE);
              ptexMesh.RenderDepth(s_cam, 1.f/16.f, Eigen::Vector4f(0.0f, 0.0f, 0.0f, 0.0f));
              glDisable(GL_CULL_FACE);

              glPopAttrib(); //GL_VIEWPORT_BIT
              depthFrameBuffer.Unbind();
              depthTexture.Download(depthImage.ptr, GL_RGB, GL_UNSIGNED_BYTE);

              char depthfilename[1000];
              snprintf(depthfilename, 1000, "%s/%s_%04zu_pos%01zu.jpeg", outputDir.c_str(), scene.c_str(), 6*j + i, k + 5 );
              pangolin::SaveImage(
                  depthImage.UnsafeReinterpret<uint8_t>(),
                  pangolin::PixelFormatFromString("RGB24"),
                  std::string(depthfilename));

            }

            Eigen::Matrix4d T_translate = Eigen::Matrix4d::Identity();
            if(k==0){
              //translate to stereo right-eye position according to input baseline
              T_translate.topRightCorner(3, 1) = Eigen::Vector3d(basel, 0, 0);
              T_camera_world = T_translate.inverse() * face_cam_to_world ;
              s_cam.GetModelViewMatrix() = T_camera_world;
            }else if(k==1){
              //translate to interpolate
              T_translate.topRightCorner(3, 1) = Eigen::Vector3d(cameraPos[j][4], cameraPos[j][5], cameraPos[j][6]);
              T_camera_world = T_translate.inverse() * face_cam_to_world ;
              s_cam.GetModelViewMatrix() = T_camera_world;
            }else if(k==2){
              //translate to exptrapolate 1
              T_translate.topRightCorner(3, 1) = Eigen::Vector3d(cameraPos[j][7], cameraPos[j][8], cameraPos[j][9]);
              T_camera_world = T_translate.inverse() * face_cam_to_world ;
              s_cam.GetModelViewMatrix() = T_camera_world;
            }else if(k==3){
              //translate to extrapolate 2
              T_translate.topRightCorner(3, 1) = Eigen::Vector3d(cameraPos[j][10], cameraPos[j][11], cameraPos[j][12]);
              T_camera_world = T_translate.inverse() * face_cam_to_world ;
              s_cam.GetModelViewMatrix() = T_camera_world;
            }

          }

          if(i<3){
            //turn to the side
            Eigen::Matrix4d curr_spot_cam_to_world = s_cam.GetModelViewMatrix();
            T_camera_world = R_side.inverse() * curr_spot_cam_to_world ;
            s_cam.GetModelViewMatrix() = T_camera_world;

          }else if(i==3){
            //look upward by 90 degree
            Eigen::Matrix4d curr_spot_cam_to_world = s_cam.GetModelViewMatrix();
            T_camera_world = R_up.inverse() * curr_spot_cam_to_world ;
            s_cam.GetModelViewMatrix() = T_camera_world;

          }else if(i==4){
            //look downward by 180 degree
            Eigen::Matrix4d curr_spot_cam_to_world = s_cam.GetModelViewMatrix();
            T_camera_world = R_down.inverse() * curr_spot_cam_to_world ;
            s_cam.GetModelViewMatrix() = T_camera_world;
          }

        }
      }
  }

  auto model_stop = high_resolution_clock::now();
  auto model_duration = duration_cast<microseconds>(model_stop - model_start);
  std::cout << "Time taken rendering the model "<<navPositions.substr(0,navPositions.length()-9).c_str()<<": "<< model_duration.count() << " microseconds" << std::endl;

  return 0;
}
