# Stereomag360 Add-ons
The original ReplicaRenderer and ReplicaViewer remain the same. See below for usage.

## ODS-ODS-ERP Dataset generation
Usage:
```
./build/ReplicaSDK/ReplicaRendererDataset dataset/scene_name/mesh.ply 
dataset/scene_name/textures dataset/scene_name/glass.sur camera_parameters[file.txt / n] 
spherical[y/n] output/dir/ output_width output_height
```

For example, to render an example set of [left_ods, right_ods, equirect] images of room_0 (with a hardcoded position):
```
./build/ReplicaSDK/ReplicaRendererDataset dataset/room_0/mesh.ply 
dataset/room_0/textures dataset/room_0/glass.sur n y output/dir/ width height
```

To render on room_0 with txt files:
```
./build/ReplicaSDK/ReplicaRendererDataset dataset/room_0/mesh.ply 
dataset/room_0/textures dataset/room_0/glass.sur glob/room_0.txt y output/dir/ width height
```

Format of one line in the input text file (camera_parameters.txt) should be:
```
camera_position_x camera_position_y camera_position_z ods_baseline 
target1_offset_x target1_offset_y target1_offset_z 
target2_offset_x target2_offset_y target2_offset_z 
target3_offset_x target3_offset_y target3_offset_z
```
The existing text files contain navigable positions within each scene, sampled with Habitat-SIM. Find all the existing text files in glob/ and test-glob/.


## Video rendering
To render on one scene:
```
./build/ReplicaSDK/ReplicaVideoRenderer path/to/scene/mesh.ply 
path/to/scene/textures path/to/scene/glass.sur camera_parameters.txt 
spherical[y/n] output/dir/ width height
```
The difference is the format of one line in input text file (camera_parameters.txt), which for video should be:
```
camera_position_x camera_position_y camera_position_z 
lookat_x lookat_y lookat_z ods_baseline 
rotation_x rotation_y rotation_z target_x target_y target_z
```

Can use glob/gen_video_path.py to generate a candidate path text file. See glob/example_script for an example of straight path.

## Depth-based Mesh rendering
Example Usage:
```
./build/ReplicaSDK/DepthMeshRendererBatch TEST_FILES CAMERA_POSES 
OUT_DIR SPHERICAL<y|n> y y OUTPUT_WIDTH OUTPUT_HEIGHT

```

TEST_FILES should contain lines of format:
```
<ods_left_color>.png <ods_left_depth>.png
```
CAMERA_POSES should contain the desired camera offsets for rendering as below for each line:
```
<translate_x> <translate_y> <translate_z>
```

See more instructions in assets/EVALUATION_HOWTO.
