"""
Python file to generate txt file specifying video path for ReplicaVideoRenderer,
Format:
camera_postion_x camera_postion_y camera_postion_z lookat_x lookat_y lookat_z baseline rotation_x rotation_y rotation_z target_translation_x target_translaton_y target_translaton_z

Note:
- rotation(x,y,z) is only nonzero if it's a path with noise
- target_translation(x,y,z) is for test set evaluation
- Limitation:
    This program samples a valid starting position, which does not guarantee the entire generated path would be valid positions.
"""

import math
import csv
import sys
import argparse
from random import randint

def gen_time_series(filename):
    time_series = []
    with open(filename) as csv_file:
        csv_reader = csv.reader(csv_file,delimiter=',')
        line_count = 0
        for row in csv_reader:
            if line_count==0:
                line_count+=1
            else:
                # print(row[0],row[1])
                time_series.append(float(row[1]))
                line_count+=1
    return time_series

def gen_straight_path(scene_name, steps, baseline, slope, moveDirection, noise, moveFactor, file2save):
    '''
    @Args:
    -scene_name
    -baseline
    -slope: forgot the motivation for this, probably for more natural noisy movement
    -moveDirection: x or y
    -noise: True or false
    -moveFactor
    '''

    scenePos = open(scene_name+".txt", "r")
    data = [[float(i) for i in line.split()] for line in scenePos]

    #samples a random valid starting position
    idx = randint(0, len(data))
    cx = data[idx][0];
    cy = data[idx][1];
    cz = data[idx][2];

    #default target offset times specified moveFactor
    offset_x = 0.064 * moveFactor
    offset_y = 0.064 * moveFactor
    offset_z = 0.064 * moveFactor

    print("Sampled starting position:", cx, cy, cz, "with baseline of ",baseline, "and offset by ", offset_x, offset_y, offset_z)

    if(noise):
        time_series = gen_time_series('noise/data.csv')
        time_series_xradian = gen_time_series('noise/data-rotx.csv')
        time_series_yradian = gen_time_series('noise/data-roty.csv')
        time_series_zradian = gen_time_series('noise/data.csv')

        #normalize
        ztime_series = [(x - time_series[0])/10 for x in time_series]
        ytime_series = [(x - time_series[0])/25 for x in time_series]
        radx = [((x - time_series_xradian[0])/7) for x in time_series_xradian]
        rady = [((x - time_series_yradian[0])/7) for x in time_series_yradian]
        radz = [((x - time_series_zradian[0])/7) for x in time_series_zradian]


    navigable_positions = []
    for i in range(steps):
        spot = []
        if(moveDirection=='x'):
            #hardcoded increment
            cx += 0.005

            #Append camera world position
            if(noise):
                spot += [cx, cy+ytime_series[i]-i*slope/steps, cz+ztime_series[i]+1.0]
            else:
                spot += [cx, cy, cz+1.0]
        else:
            #hardcoded
            cy -= 0.005

            #Append camera world position
            if(noise):
                spot += [cx+ytime_series[i]-i*slope/steps, cy, cz+ztime_series[i]+1.0]
            else:
                spot += [cx, cy, cz+1.0]

        # Append camera look-at position, baseline, rotation, and target offsets
        if(noise):
            spot += [1, 1, cz+ztime_series[i]+1.0]
            spot += [baseline]
            spot += [radx[i], rady[i], radz[i]]
            spot += [offset_x, offset_y, offset_z]
        else:
            spot += [cx, cy+1, cz+1]
            spot += [baseline]
            spot += [0, 0, 0]
            spot += [offset_x, offset_y, offset_z]

        navigable_positions.append(spot)

    with open(file2save,'w') as f:
        f.writelines(' '.join(str(j) for j in i) +'\n' for i in navigable_positions)

def gen_square_path(scene_name, steps, baseline, noise, moveFactor, file2save):

    scenePos = open(scene_name+".txt", "r")
    data = [[float(i) for i in line.split()] for line in scenePos]

    #samples a random valid starting position
    idx = randint(0, len(data))
    cx = data[idx][0];
    cy = data[idx][1];
    cz = data[idx][2] - 1.5;

    #default target offset times specified moveFactor
    offset_x = 0.064 * moveFactor
    offset_y = 0.064 * moveFactor
    offset_z = 0.064 * moveFactor

    print("Sampled starting position:", cx, cy, cz, "with baseline of ",baseline, "and offset by ", offset_x, offset_y, offset_z)
    if(noise):
        #Noise not implemented for square path..
        time_series = gen_time_series('noise/data.csv')
        time_series_xradian = gen_time_series('noise/data-rotx.csv')
        time_series_yradian = gen_time_series('noise/data-roty.csv')
        time_series_zradian = gen_time_series('noise/data.csv')

        #normalize
        ztime_series = [(x - time_series[0])/10 for x in time_series]
        ytime_series = [(x - time_series[0])/25 for x in time_series]
        radx = [((x - time_series_xradian[0])/7) for x in time_series_xradian]
        rady = [((x - time_series_yradian[0])/7) for x in time_series_yradian]
        radz = [((x - time_series_zradian[0])/7) for x in time_series_zradian]

    navigable_positions = []
    side_steps = steps//4
    corner_steps = 60 #hardcoded
    for j in range(side_steps):
        spot = []
        cx += 0.01
        spot += [cx, cy, cz, cx+1, cy, cz, baseline, 0, 0, 0, offset_x, offset_y, offset_z]
        navigable_positions.append(spot)

    for k in range(corner_steps):
        spot=[]
        spot += [cx, cy, cz, cx+math.cos((float(k)/60)*(math.pi/2)), cy+math.sin((float(k)/60)*(math.pi/2)), cz,
                baseline, 0, 0, 0, offset_x, offset_y, offset_z]
        navigable_positions.append(spot)

    for j in range(side_steps):
        spot = []
        cy += 0.01
        spot += [cx, cy, cz, cx, cy+1, cz, baseline, 0, 0, 0, offset_x, offset_y, offset_z]
        navigable_positions.append(spot)

    for k in range(corner_steps):
        spot=[]
        spot += [cx, cy, cz, cx-math.sin((float(k)/60)*(math.pi/2)), cy+math.cos((float(k)/60)*(math.pi/2)), cz,
                 baseline, 0, 0, 0, offset_x, offset_y, offset_z]
        navigable_positions.append(spot)

    for j in range(side_steps):
        spot = []
        cx -= 0.01
        spot += [cx, cy, cz, cx-1, cy, cz, baseline, 0, 0, 0, offset_x, offset_y, offset_z]
        navigable_positions.append(spot)

    for k in range(corner_steps):
        spot=[]
        spot += [cx, cy, cz, cx-math.cos((float(k)/60)*(math.pi/2)), cy-math.sin((float(k)/60)*(math.pi/2)), cz,
                 baseline, 0, 0, 0, offset_x, offset_y, offset_z]
        navigable_positions.append(spot)

    for j in range(side_steps):
        spot = []
        cy -= 0.01
        spot += [cx, cy, cz, cx, cy-1, cz, baseline, 0, 0, 0, offset_x, offset_y, offset_z]
        navigable_positions.append(spot)

    with open(file2save,'w') as f:
        f.writelines(' '.join(str(j) for j in i) +'\n' for i in navigable_positions)

def gen_circle_path(scene_name, steps, baseline, r, noise, moveFactor, file2save):

    scenePos = open("/home/selenaling/Desktop/matryodshka-replica360/glob/train/"+scene_name+"_6dof.txt", "r")
    data = [[float(i) for i in line.split()] for line in scenePos]

    #samples a random valid starting position
    idx = randint(0, len(data))
    cx = data[idx][0];
    cy = data[idx][1];
    cz = data[idx][2] - 1.5;

    #default target offset times specified moveFactor
    offset_x = 0.064 * moveFactor
    offset_y = 0.064 * moveFactor
    offset_z = 0.064 * moveFactor

    print("Sampled starting position:", cx, cy, cz, "with baseline of ", baseline, " and offset by ", offset_x, offset_y, offset_z)
    if(noise):
        time_series = gen_time_series('noise/data.csv')
        time_series_xradian = gen_time_series('noise/data-rotx.csv')
        time_series_yradian = gen_time_series('noise/data-roty.csv')
        time_series_zradian = gen_time_series('noise/data.csv')

        #normalize
        ztime_series = [(x - time_series[0])/10 for x in time_series]
        ytime_series = [(x - time_series[0])/25 for x in time_series]
        radx = [((x - time_series_xradian[0])/7) for x in time_series_xradian]
        rady = [((x - time_series_yradian[0])/7) for x in time_series_yradian]
        radz = [((x - time_series_zradian[0])/7) for x in time_series_zradian]

    navigable_positions = []
    for i in range(steps):
        spot = []

        rad = float(i) / steps * math.pi * 2
        nx = cx - math.cos(rad)*r
        ny = cy - math.sin(rad)*r
        nz = cz

        spot += [nx, ny, nz]
        spot += [nx + (nx-cx), ny + (ny-cy), cz]
        spot += [baseline]
        if(noise):
            spot += [radx[i], rady[i], radz[i]]
        else:
            spot += [0,0,0]
        spot += [offset_x, offset_y, offset_z]

        navigable_positions.append(spot)

    with open(file2save,'w') as f:
        f.writelines(' '.join(str(j) for j in i) +'\n' for i in navigable_positions)

def str2bool(v):
    if isinstance(v, bool):
       return v
    if v.lower() in ('yes', 'true', 't', 'y', '1'):
        return True
    elif v.lower() in ('no', 'false', 'f', 'n', '0'):
        return False
    else:
        raise argparse.ArgumentTypeError('Boolean value expected.')

def main():

    parser = argparse.ArgumentParser()
    parser.add_argument('--scene', type=str, help='Scene to render.', default='room_0')
    parser.add_argument('--pathtype', type=str, help='Type of path [straight, square, circle]', default='straight')
    parser.add_argument('--nsteps', type=int, help='Number of steps in the path.', default=100)
    parser.add_argument('--baseline', type=float, help='Baseline for stereo ods image pair.', default='0.032')
    parser.add_argument('--noise', type=str2bool, help='Whether to add nosie for path generation [y/n].', default=False)
    parser.add_argument('--moveFactor', type=int, help='Move factor for default target position [0.064, 0.064, 0.064]. Application for test set generation files, for which a target image is needed for evaluation.', default=1)
    parser.add_argument('--output', type=str, help='Output file name.', default="path.txt")

    parser.add_argument('--moveDir', type=str, help='Move direction for straight path.', default='x')
    parser.add_argument('--radius', type=float, help="Radius of the circle.", default='2.0')
    args = parser.parse_args()

    if(args.pathtype=='straight'):

        gen_straight_path(args.scene, args.nsteps, args.baseline, 0.5, args.moveDir, args.noise , args.moveFactor, args.output)
    elif(args.pathtype=='circle'):
        gen_circle_path(args.scene, args.nsteps, args.baseline, args.radius, args.noise, args.moveFactor, args.output)
    elif(args.pathtype=='square'):
        gen_square_path(args.scene, args.nsteps, args.baseline, args.noise, args.moveFactor, args.output)
    else:
        print("Path type not recognized. Please specify type as one of (straight, square, circle).")

if __name__ == '__main__':
    main()
