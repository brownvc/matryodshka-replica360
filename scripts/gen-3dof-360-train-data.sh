#!/bin/bash
outdir=$1
width=$2
height=$3
find ./dataset/train/* -type d -maxdepth 0 | xargs -I {} basename {} | xargs -P 3 -I {} -n 1 ./build/ReplicaSDK/ReplicaRendererDataset ./dataset/train/{}/mesh.ply ./dataset/train/{}/textures ./dataset/train/{}/glass.sur ./glob/train/{}_6dof.txt y $outdir $width $height

