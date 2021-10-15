import argparse
import sys
import os

#2, 6
#5, 7
parser = argparse.ArgumentParser()
parser.add_argument('--scene_name', type=str, help='targe_scene_dir', 
        required=True)
parser.add_argument('--cam_param', type=str, help='camera parameter raw')
args = parser.parse_args()

with open(args.cam_param, 'r') as f:
    cameraPos = f.readlines()
    cameraPos = [Pos.split(' ')[:3] + [str(float(Pos.split(' ')[i]) - float(Pos.split(' ')[i+10])) for i in range(3)]  for Pos in cameraPos]

print(len(cameraPos))
print(cameraPos[0])
print(len(cameraPos[0]))
assert False
template = "%s/output_color/frame_%04d_pos0.png %s/output_depth/frame_%04d_pos1.png\n"
with open(args.output_file, 'w') as f:
    for i in range(args.num_files):
        f.write(template % (args.input_dir, i, args.input_dir, i))

