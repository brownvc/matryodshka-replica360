import os
import cv2


nf = 300

for i in range(nf):

    idx = [5,8,11]

    src = "/home/selenaling/Desktop/20191113_6DoF_TestSet/pngcuda_640x320/apartment_1_%04d_pos%02d.png" % (i//3, idx[i%3])
    dest = "/home/selenaling/Desktop/stereomag360/test/results/mesh-based-rerendering/test/%04d/tgt_image_%04d.png" % (i, i)
    unflip_dest = "/home/selenaling/Desktop/stereomag360/test/results/mesh-based-rerendering/test/%04d/tgt_image_%04d_orig.png" % (i, i)

    odsnet_dest = "/home/selenaling/Desktop/stereomag360/test/results/ods-net/test/%04d/tgt_image_%04d.png" % (i, i)


    src_img = cv2.imread(src)
    flip_src = cv2.flip(src_img, 1)
    cv2.imwrite(odsnet_dest, flip_src)

    # print(src)
    # print(dest)
    # command = 'rm -f %s' %  unflip_dest
    # print(command)
    # os.system(command)
