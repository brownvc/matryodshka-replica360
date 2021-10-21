import argparse
import sys

template = "%s/output_color/frame_%04d.png %s/im_BG.png %s/im_BG_inp.png %s/output_depth/frame_%04d.png %s/im_BGD.png %s/im_BGD_inp.png %s/output_alpha/frame_%04d.png %s/im_BGA.png\n"

parser = argparse.ArgumentParser()
parser.add_argument('--input_dir', type=str, help='Input dir', default='./assets/shore')
parser.add_argument('--output_file', type=str, help='Output file', default='test_data_layered')
parser.add_argument('--num_files', type=int, help='Num files', default=929)
args = parser.parse_args()

with open(args.output_file, 'w') as f:
    for i in range(args.num_files):
        f.write(template % (args.input_dir, i, args.input_dir, args.input_dir, args.input_dir, i, args.input_dir, args.input_dir, args.input_dir, i, args.input_dir))
