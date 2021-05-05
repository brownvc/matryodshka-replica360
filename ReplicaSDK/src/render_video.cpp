
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
  ASSERT(spherical, "This code only render spherical images.");
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
  // 0,1,2,3,4,5,6,7,8,9,10,11,12
  // camera_position_x, camera_position_y, camera_position_z, lookat_x, lookat_y, lookat_z, ods baseline, rotx, roty, rotz, translate_x, translate_y, translate_z
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

  bool saveParameter = false;
  std::vector<std::vector<float>> camPostoSave;

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
  std::vector<float> initCam = {0,0.5,-0.6230950951576233, 1, 1, -0.6230950951576233};//default
  if(navCam){
    initCam = cameraPos[0];
    std::cout<<"First camera position:"<<initCam[0]<<" "<<initCam[1]<<" "<<initCam[2];
    std::cout<<"First camera lookat:"<<initCam[3]<<" "<<initCam[4]<<" "<<initCam[5];
  }

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
      pangolin::ModelViewLookAtRDF(initCam[0],initCam[1],initCam[2], initCam[3], initCam[4], initCam[5], 0, 0, 1));

  Eigen::Matrix4d T_camera_world = s_cam.GetModelViewMatrix();
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

  size_t numFrames = 20; //default
  if(navCam){
    numFrames = cameraPos.size();
  }
  srand(2019); //random seed

  for(size_t j=0; j<numFrames;j++){
      //get the model view matrix
      Eigen::Matrix4d spot_cam_to_world = s_cam.GetModelViewMatrix();

      //video frame rendering scheme:
      //0,1,2: input spot
      //3,4,5: target spot
      //6: input erp depth if renderdepth
      //7: target erp depth if renderdepth
     for(int k = 0; k < 6; k ++){

       int which_spot = k / 3;
       int eye = k % 3;
       float basel = cameraPos[j][6];

       if(which_spot == 1){
         Eigen::Matrix4d T_translate = Eigen::Matrix4d::Identity();
         T_translate.topRightCorner(3, 1) = Eigen::Vector3d(cameraPos[j][10], cameraPos[j][11], cameraPos[j][12]);
         T_camera_world = T_translate.inverse() * spot_cam_to_world ;
         std::cout<<"target spot:"<<std::endl;
         std::cout<<T_translate<<std::endl;
         std::cout<<spot_cam_to_world<<std::endl;
         std::cout<<T_camera_world<<std::endl;
         s_cam.GetModelViewMatrix() = T_camera_world;
       }

       std::cout << "\rRendering position " << k + 1 << "/" << 6 <<"from spot "<<which_spot<<" with baseline of " << basel << "and eye of "<< eye << std::endl;
       frameBuffer.Bind();

       glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
       glPushAttrib(GL_VIEWPORT_BIT);
       glViewport(0, 0, width, height);
       glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
       glEnable(GL_CULL_FACE);
       ptexMesh.SetExposure(0.01);
       ptexMesh.SetBaseline(basel);
       ptexMesh.Render(s_cam,Eigen::Vector4f(0.0f, 0.0f, 0.0f, 0.0f), eye);
       glDisable(GL_CULL_FACE);
       glPopAttrib(); //GL_VIEWPORT_BIT

       frameBuffer.Unbind();

       // Download and save
       render.Download(image.ptr, GL_RGB, GL_UNSIGNED_BYTE);

       char equirectFilename[1000];
       snprintf(equirectFilename, 1000, "%s/%s_%04zu_pos%01d.jpeg",outputDir.c_str(), scene.c_str(), j, k);
       pangolin::SaveImage(image.UnsafeReinterpret<uint8_t>(),
                           pangolin::PixelFormatFromString("RGB24"),
                           std::string(equirectFilename));

       if (renderDepth && (k==2 || k==5)) {

           depthFrameBuffer.Bind();
           glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
           glPushAttrib(GL_VIEWPORT_BIT);
           glViewport(0, 0, width, height);
           glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
           glEnable(GL_CULL_FACE);
           ptexMesh.RenderDepth(s_cam,1.f/16.f,Eigen::Vector4f(0.0f, 0.0f, 0.0f, 0.0f),eye);
           glDisable(GL_CULL_FACE);
           glPopAttrib(); //GL_VIEWPORT_BIT
           depthFrameBuffer.Unbind();
           depthTexture.Download(depthImage.ptr, GL_RGB, GL_UNSIGNED_BYTE);

           char filename[1000];
           snprintf(filename,1000,"%s/%s_%04zu_pos%01d.jpeg",outputDir.c_str(), scene.c_str(), j, 5+(k+1)/3);
           pangolin::SaveImage(
               depthImage.UnsafeReinterpret<uint8_t>(),
               pangolin::PixelFormatFromString("RGB24"),
               std::string(filename)
             );
           }

       if (saveParameter) {
         //calculate and save the quaternion
         T_camera_world = s_cam.GetModelViewMatrix();
         Eigen::Matrix4d cam_matrix = T_camera_world.inverse();
         Eigen::Vector3d forward_after = cam_matrix.block(0,2,3,1);
         Eigen::Vector3d right_after = cam_matrix.block(0,0,3,1);
         Eigen::Vector3d up_after = cam_matrix.block(0,1,3,1);

         right_after.normalized();
         double theta = forward_after.dot(up_after);
         double angle_rotation = -1.0*acos(theta);

         Eigen::Quaterniond q;
         q = Eigen::AngleAxis<double>(angle_rotation,right_after);

         std::cout<<"quaternion matrix"<<std::endl;
         std::cout<<q.w()<<" "<<q.x()<<" "<<q.y()<<" "<<q.z()<<std::endl;
         for(int r = 0; r<4; r++){
           for(int c = 0; c<4; c++){
             std::cout<<T_camera_world(r,c);
           }
           std::cout<<std::endl;
         }

         camPostoSave.push_back(std::vector<float>());
         camPostoSave[j].push_back(cameraPos[j][0]);
         camPostoSave[j].push_back(cameraPos[j][1]);
         camPostoSave[j].push_back(cameraPos[j][2]);

         //save the quaternion
         // camPostoSave[step].push_back(q.x());
         // camPostoSave[step].push_back(q.y());
         // camPostoSave[step].push_back(q.z());
         // camPostoSave[step].push_back(q.w());

         for(int r = 0; r<4; r++){
           for(int c = 0; c<4; c++){
             camPostoSave[j].push_back(T_camera_world(r,c));
           }
         }
         camPostoSave[j].push_back(width/2);
         camPostoSave[j].push_back(width/2);

         char parameter_filename[1000];
         snprintf(parameter_filename, 1000, "%s/%s_parameters.txt", outputDir.c_str(), scene.c_str());

         std::ofstream myfile(parameter_filename);
         if(myfile.is_open()){
           for(int j=0;j<numFrames;j++){
             std::cout<<"hello";
             myfile<<j<<" ";

             for(int e = 0; e < camPostoSave[j].size(); e++){
               myfile<<camPostoSave[j][e]<<" ";
             }
             myfile<<std::endl;
           }
           myfile.close();
         }
       }
     }

     if(navCam){
       if(j+1<numFrames){
         s_cam.SetModelViewMatrix(pangolin::ModelViewLookAtRDF(cameraPos[j+1][0],cameraPos[j+1][1],cameraPos[j+1][2], cameraPos[j+1][3], cameraPos[j+1][4], cameraPos[j+1][5], 0, 0, 1));

         if(abs(cameraPos[j+1][7] - 0.0) > std::numeric_limits<float>::epsilon()
         && abs(cameraPos[j+1][8] - 0.0) > std::numeric_limits<float>::epsilon()
         && abs(cameraPos[j+1][9] - 0.0) > std::numeric_limits<float>::epsilon()){
           //nonzero rotation -- path generated with noise
           T_camera_world = s_cam.GetModelViewMatrix();
           Eigen::Transform<double,3,Eigen::Affine> rx(Eigen::AngleAxis<double>(cameraPos[j+1][7],Eigen::Vector3d::UnitX()));
           Eigen::Transform<double,3,Eigen::Affine> ry(Eigen::AngleAxis<double>(cameraPos[j+1][8],Eigen::Vector3d::UnitZ()));
           Eigen::Transform<double,3,Eigen::Affine> rz(Eigen::AngleAxis<double>(cameraPos[j+1][9],Eigen::Vector3d::UnitY()));
           Eigen::Matrix4d R_X =Eigen::Matrix4d::Identity();
           Eigen::Matrix4d R_Y =Eigen::Matrix4d::Identity();
           Eigen::Matrix4d R_Z =Eigen::Matrix4d::Identity();
           R_X = rx.matrix();
           R_Y = ry.matrix();
           R_Z = rz.matrix();
           T_camera_world = R_Z * R_Y * R_X * T_camera_world;
           s_cam.GetModelViewMatrix() = T_camera_world;
         }
       }
     }else{
       continue;
     }
     std::cout << "\r Spot " << j + 1  << "/" << numFrames << std::endl;

  }

  auto model_stop = high_resolution_clock::now();
  auto model_duration = duration_cast<microseconds>(model_stop - model_start);
  std::cout << "Time taken rendering the video "<<navPositions.substr(0,navPositions.length()-9).c_str()<<": "<< model_duration.count() << " microseconds" << std::endl;

  return 0;
}
