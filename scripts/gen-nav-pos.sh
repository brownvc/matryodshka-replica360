#!/bin/bash

for f in */;
do
  python3 /home/selenaling/Desktop/habitat-sim/examples/example.py --scene "/home/selenaling/Desktop/Replica-Dataset/dataset/${f}habitat/mesh_semantic.ply"
done
