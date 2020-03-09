import argparse
import cv2
import numpy as np
from os import listdir
from os.path import isfile, join

parser = argparse.ArgumentParser()
parser.add_argument('--width', type=int, help='Output width', default=640)
parser.add_argument('--height', type=int, help='Output height', default=320)
parser.add_argument('--image_dir', type=str, help='Image file', default='out')
args = parser.parse_args()

files = [join(args.image_dir, f) for f in listdir(args.image_dir) if isfile(join(args.image_dir, f))]

for i, f in enumerate(files):
    print(f)
    frame = cv2.imread(f)

    # Resize images
    sigma = float(frame.shape[0]) / (2 * args.height)
    kwidth = int(sigma * 3)

    if kwidth % 2 == 0:
        kwidth = kwidth + 1

    # Blur
    frame = cv2.GaussianBlur(frame, (kwidth, kwidth), sigma)

    # Write images
    cv2.imwrite(f, frame)
