from tqdm import tqdm
import numpy as np
import cv2
import sys, os
import argparse

def str2bool(v):
    if isinstance(v, bool):
        return v
        if v.lower() in ('yes', 'true', 't', 'y', '1'):
            return True
        elif v.lower() in ('no', 'false', 'f', 'n', '0'):
            return False
        else:
            raise argparse.ArgumentTypeError('Boolean value expected.')

# Arguments
parser = argparse.ArgumentParser()
parser.add_argument('--sample_rate', type=int, help='Sample rate for video', default=1)
parser.add_argument('--width', type=int, help='Video width', default=640)
parser.add_argument('--height', type=int, help='Video width', default=320)
parser.add_argument('--output_dir', type=str, help='Output directory', default='output')
parser.add_argument('--output_prefix', type=str, help='Output prefix', default='frame')
parser.add_argument('--video', type=str, help='Video file', default='input.mp4')
parser.add_argument('--flip_lr', type=str2bool, help='Flip left and right images', default=False)
args = parser.parse_args()

# Create output directory
if not os.path.exists(args.output_dir):
    os.makedirs(args.output_dir)


# Capture video
cap = cv2.VideoCapture(args.video)
length = int(cap.get(cv2.CAP_PROP_FRAME_COUNT))
frame_num = 0

for iter_num in tqdm(range(min(length, 1000))):
    if iter_num % args.sample_rate == 0:
        ret, frame = cap.read()

        # Write out frame
        left = frame[frame.shape[0]//2:, :, :]
        right = frame[:frame.shape[0]//2, :, :]

        if args.flip_lr:
            temp = left
            left = right
            right = temp

        # Resize images
        sigma = float(frame.shape[0]) / (2 * args.height)
        kwidth = int(sigma * 3)

        if kwidth % 2 == 0:
            kwidth = kwidth + 1

        ''' Resize with cuda later '''
        left = cv2.GaussianBlur(left, (kwidth, kwidth), sigma)
        right = cv2.GaussianBlur(right, (kwidth, kwidth), sigma)
        left = cv2.resize(left, (args.width, args.height))
        right = cv2.resize(right, (args.width, args.height))

        # Write images
        cv2.imwrite(args.output_dir + '/' + args.output_prefix + ('_%04d' % frame_num) + '_pos0.png', left)
        cv2.imwrite(args.output_dir + '/' + args.output_prefix + ('_%04d' % frame_num) + '_pos1.png', right)

        frame_num += 1

with open(args.output_dir+"/framenumber.txt","w") as file:
    file.write("%d" % frame_num)

cap.release()
cv2.destroyAllWindows()
