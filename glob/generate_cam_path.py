import math
import csv
import sys

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
    print(len(time_series))
    return time_series

def gen_straight_path(scene_name, slope, moveDirection, noise, moveFactor):

    file_to_save = scene_name + "x"+str(moveFactor)+"ipd_video.txt"

    scenePos = open(scene_name+".txt", "r")
    data = [[float(i) for i in line.split()] for line in scenePos]

    start_idx = 0
    cx = data[start_idx][0];
    cy = data[start_idx][1];
    cz = data[start_idx][2] - 1.5;

    baseline = data[0][3];
    offset_x = data[0][4];
    offset_y = data[0][5];
    offset_z = data[0][6];

    offset_x = 0.064 * moveFactor
    offset_y = 0.064 * moveFactor
    offset_z = 0.064 * moveFactor

    print("Sampled position:", cx, cy, cz, "with baseline of ",baseline, "and offset by ",offset_x, offset_y, offset_z)
    if(noise):
        time_series = gen_time_series('data.csv')
        time_series_xradian = gen_time_series('data-rotx.csv')
        time_series_yradian = gen_time_series('data-roty.csv')
        time_series_zradian = gen_time_series('data.csv')

        ztime_series = [(x - time_series[0])/10 for x in time_series]
        ytime_series = [(x - time_series[0])/25 for x in time_series]

        radx = [((x - time_series_xradian[0])/7) for x in time_series_xradian]
        rady = [((x - time_series_yradian[0])/7) for x in time_series_yradian]
        radz = [((x - time_series_zradian[0])/7) for x in time_series_zradian]
    navigable_positions = []
    for i in range(450):
        spot = []
        if(moveDirection==0):
            cx += 0.005
            # camera world position
            if(noise):
                spot.append(cx)
                spot.append(cy+ytime_series[i]-i*slope/450.0)
                spot.append(cz+ztime_series[i]+1.0)
            else:
                spot.append(cx)
                spot.append(cy)
                spot.append(cz+1.0)
        else:
            cy -= 0.005
            # spot.append(i)
            # camera world position
            if(noise):
                spot.append(cx+ytime_series[i]-i*slope/450.0)
                spot.append(cy)
                spot.append(cz+ztime_series[i]+1.0)
            else:
                spot.append(cx)
                spot.append(cy)
                spot.append(cz+1.0)

        # camera look-at position
        if(noise):
            spot.append(1)
            spot.append(1)
            spot.append(cz+ztime_series[i]+1.0)
            spot.append(radx[i])
            spot.append(rady[i])
            spot.append(radz[i])
        else:
            spot.append(cx)
            spot.append(cy+1)
            spot.append(cz+1.0)
            spot.append(baseline)
            spot.append(offset_x)
            spot.append(offset_y)
            spot.append(offset_z)
        navigable_positions.append(spot)

    with open(file_to_save,'w') as f:
        f.writelines(' '.join(str(j) for j in i) +'\n' for i in navigable_positions)

def gen_square_path(ix, iy, iz):
    navigable_positions = []
    fname ="square.txt"

    cx = ix
    cy = iy
    cz = iz

    frame_num = 0
    for j in range(150):
        spot = []
        cx += 0.01
        # spot.append(frame_num)
        frame_num += 1
        spot.append(cx)
        spot.append(cy)
        spot.append(cz)
        spot.append(cx+1)
        spot.append(cy)
        spot.append(cz)
        navigable_positions.append(spot)

    for k in range(60):
        spot=[]
        # spot.append(frame_num)
        frame_num += 1
        spot.append(cx)
        spot.append(cy)
        spot.append(cz)
        spot.append(cx+math.cos((float(k)/60)*(math.pi/2)))
        # print(math.cos((float(k)/60)*(math.pi/2)))
        # print(math.sin((float(k)/60)*(math.pi/2)))
        spot.append(cy+math.sin((float(k)/60)*(math.pi/2)))
        spot.append(cz)
        navigable_positions.append(spot)

    for j in range(150):
        spot = []
        # spot.append(frame_num)
        frame_num += 1
        cy += 0.01
        spot.append(cx)
        spot.append(cy)
        spot.append(cz)
        spot.append(cx)
        spot.append(cy+1)
        spot.append(cz)
        navigable_positions.append(spot)

    for k in range(60):
        spot=[]
        # spot.append(frame_num)
        frame_num += 1
        spot.append(cx)
        spot.append(cy)
        spot.append(cz)
        spot.append(cx-math.sin((float(k)/60)*(math.pi/2)))
        # print(math.cos((float(k)/60)*(math.pi/2)))
        # print(math.sin((float(k)/60)*(math.pi/2)))
        spot.append(cy+math.cos((float(k)/60)*(math.pi/2)))
        spot.append(cz)
        navigable_positions.append(spot)

    for j in range(150):
        spot = []
        cx -= 0.01
        # spot.append(frame_num)
        frame_num += 1
        spot.append(cx)
        spot.append(cy)
        spot.append(cz)
        spot.append(cx-1)
        spot.append(cy)
        spot.append(cz)
        navigable_positions.append(spot)

    for k in range(60):
        spot=[]
        # spot.append(frame_num)
        frame_num += 1
        spot.append(cx)
        spot.append(cy)
        spot.append(cz)
        spot.append(cx-math.cos((float(k)/60)*(math.pi/2)))
        # print(math.cos((float(k)/60)*(math.pi/2)))
        # print(math.sin((float(k)/60)*(math.pi/2)))
        spot.append(cy-math.sin((float(k)/60)*(math.pi/2)))
        spot.append(cz)
        navigable_positions.append(spot)

    for j in range(150):
        spot = []
        cy -= 0.01
        # spot.append(frame_num)
        frame_num += 1
        spot.append(cx)
        spot.append(cy)
        spot.append(cz)
        spot.append(cx)
        spot.append(cy-1)
        spot.append(cz)
        navigable_positions.append(spot)



    with open(fname,'w') as f:
        f.writelines(' '.join(str(j) for j in i) +'\n' for i in navigable_positions)

def gen_circle_path(cx, cy, cz, r):

    time_series = gen_time_series('data.csv')
    time_series_xradian = gen_time_series('data-rotx.csv')
    time_series_yradian = gen_time_series('data-roty.csv')
    time_series_zradian = gen_time_series('data.csv')

    ztime_series = [(x - time_series[0])/10 for x in time_series]
    ytime_series = [(x - time_series[0])/25 for x in time_series]

    radx = [((x - time_series_xradian[0])/7) for x in time_series_xradian]
    rady = [((x - time_series_yradian[0])/7) for x in time_series_yradian]
    radz = [((x - time_series_zradian[0])/7) for x in time_series_zradian]

    navigable_positions = []
    fname ="circle_noise.txt"

    for i in range(720):
        spot = []
        rad = float(i)/720 * math.pi * 2
        nx = cx - math.cos(rad)*r
        ny = cy - math.sin(rad)*r
        nz = cz
        # spot.append(i)
        spot.append(nx)
        spot.append(ny)
        spot.append(nz+ztime_series[i])
        spot.append(nx+(nx-cx))
        spot.append(ny+(ny-cy))
        spot.append(cz+ztime_series[i])
        spot.append(radx[i])
        spot.append(rady[i])
        spot.append(radz[i])
        navigable_positions.append(spot)


    with open(fname,'w') as f:
        f.writelines(' '.join(str(j) for j in i) +'\n' for i in navigable_positions)

def main():
    # gen_square_path(0,0.5,-0.6230950951576233)
    # gen_circle_path(1,1,0.5,0.5)
    # gen_straight_path(1.815335988998413, 1.7848060131073, -1.0410082459449768, 'straight_path_noise1.txt',0, 0, True)
    scene = sys.argv[1]
    for i in range(1,4,1):
        gen_straight_path(scene, 0.5, 1, False, i)

    # gen_straight_path(1.815335988998413, 2.7848060131073, -0.5410082459449768, 'straight_path_noise2.txt',0, 0, True)
    # gen_straight_path(1.815335988998413, 2.7848060131073, -0.8410082459449768, 'straight_path_noise3.txt',2, 0, True)
    # gen_straight_path(2.815335988998413, 1.7848060131073, -0.6510082459449768, 'straight_path_noise4.txt',1, 0, True)
    #
    # gen_straight_path(1.815335988998413, 1.7848060131073, -1.0410082459449768, 'straight_path_noise5.txt',0, 1, True)
    # gen_straight_path(1.815335988998413, 0.7848060131073, -0.5410082459449768, 'straight_path_noise6.txt',0.5, 1, True)
    # gen_straight_path(1.815335988998413, 1.2848060131073, -0.8410082459449768, 'straight_path_noise7.txt',1.5, 1, True)
    # gen_straight_path(2.815335988998413, 1.7848060131073, -0.6510082459449768, 'straight_path_noise8.txt',1, 1, True)


if __name__ == '__main__':
    main()
