import argparse
import cv2
import numpy as np

parser = argparse.ArgumentParser()
parser.add_argument('--width', type=int, help='Output width', default=640)
parser.add_argument('--height', type=int, help='Output height', default=320)
parser.add_argument('--image', type=str, help='Image file', default='input.png')
parser.add_argument('--output_image', type=str, help='Output image file', default='output.png')
args = parser.parse_args()

frame = cv2.imread(args.image)

# Resize images
sigma = float(frame.shape[0]) / (2 * args.height)
kwidth = int(sigma * 3)

if kwidth % 2 == 0:
    kwidth = kwidth + 1

# Blur
frame = cv2.GaussianBlur(frame, (kwidth, kwidth), sigma)

# Write images
cv2.imwrite(args.output_image, frame)
