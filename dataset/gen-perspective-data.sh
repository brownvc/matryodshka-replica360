#!/bin/bash

# for f in */;
# do
#   fb=${f%/}
#   ../build/ReplicaSDK/ReplicaRenderer ./${f}mesh.ply ./${f}textures ./${f}glass.sur $fb.txt y
# done

# for i in {1..3}
# do
#   ../build/ReplicaSDK/ReplicaRenderer apartment_1/mesh.ply apartment_1/textures apartment_1/glass.sur short_apartment_1.txt y
# done

#find * -type d -maxdepth 0 | tail -n $(($(ls *txt | wc -l)-3)) | xargs -P 2 -I {} -n 1 ../build/ReplicaSDK/ReplicaRenderer {}/mesh.ply {}/textures {}/glass.sur {}.txt y

find * -type d -maxdepth 0 | xargs -P 2 -I {} -n 1 ../build/ReplicaSDK/ReplicaRenderer {}/mesh.ply {}/textures {}/glass.sur {}.txt 

