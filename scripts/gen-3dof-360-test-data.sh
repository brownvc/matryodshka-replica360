#!/bin/bash
outdir=$1
width=$2
height=$3
find ./dataset/test/* -type d -maxdepth 0 | xargs -I {} -n 1 basename {} | xargs -P 3 -I {} -n 1 ./build/ReplicaSDK/ReplicaRendererDataset ./dataset/test/{}/mesh.ply ./dataset/test/{}/textures ./dataset/test/{}/glass.sur ./glob/test/{}_6dof.txt n $outdir $width $height

