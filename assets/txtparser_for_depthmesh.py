import sys
import numpy as np

def convert2paths(images):


    #dir = '/home/selenaling/Desktop/20191113_6DoF_TestSet/pngcuda_640x320/'
    dir = '/home/selenaling/Desktop/20191113_6DoF_TestSet/test_videowd_640x320/'

    filepaths = []
    for i in images:
        scene = i[0]
        path = [dir + scene + '_pos2.png', dir + scene + '_pos6.png'] #colorfile and depthfile
        filepaths.append(path)
    return filepaths

def convert2odsnetpaths(images):

    dir = '/home/selenaling/Desktop/20191113_6DoF_TestSet/pngcuda_640x320/'
    depth_dir = '/home/selenaling/Desktop/ods-net/results/depth/'

    filepaths = []
    count = 0
    for i in images:
        scene = i[0]
        path = [dir + scene + '_pos00.png', depth_dir + 'im_%04d.png' % count ] #colorfile and depthfile
        filepaths.append(path)
        count += 1

    return filepaths

def convert2paths_odspair(images):

    dir = '/home/selenaling/Desktop/20191113_6DoF_TestSet/pngcuda_640x320/'

    filepaths = []
    for i in images:
        scene = i[0]
        path = [dir + scene + '_pos00.png', dir + scene + '_pos01.png'] #colorfile and depthfile
        filepaths.append(path)
    return filepaths

def main():
    file = sys.argv[1]
    output = sys.argv[2]
    poses = np.loadtxt(file, usecols=[5,6,7])
    images = np.loadtxt(file, usecols=[0,3], dtype=np.str)
    baselines = np.loadtxt(file, usecols=[4])

    img_paths = convert2paths(images)
    print(img_paths)
    # odspair_paths = convert2paths_odspair(images)
    #
    # odspair4mesh_paths = convert2odsnetpaths(images)
    #
    # print(img_paths)
    # print(poses)
    # print(odspair_paths)
    # print(odspair4mesh_paths)
    #
    # baseline_file = file[:-4] + '_baselines.txt'
    # with open(baseline_file, 'w'):
    #     np.savetxt(baseline_file, baselines)

    posefile = output + '_poses.txt'
    with open(posefile, 'w'):
        np.savetxt(posefile, poses)

    imgfile = output + '_dmeshinput.txt'
    with open(imgfile, 'w'):
        np.savetxt(imgfile, img_paths, fmt="%s")

    # odspairfile = file[:-4] + '_odspair.txt'
    # with open(odspairfile, 'w'):
    #     np.savetxt(odspairfile, odspair_paths, fmt="%s")

    # odsnetinput = file[:-4] + '_odsnet.txt'
    # with open(odsnetinput, 'w'):
    #     np.savetxt(odsnetinput, odspair4mesh_paths, fmt="%s")

if __name__ == '__main__':
    main()
