#!/bin/bash
outdir=$1
width=$2
height=$3
find * -type d -maxdepth 0 | xargs -P 2 -I {} -n 1 ../build/ReplicaSDK/ReplicaRenderer {}/mesh.ply {}/textures {}/glass.sur ../glob/{}.txt y $outdir $width $height

