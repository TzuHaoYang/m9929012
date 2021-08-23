#!/bin/bash

patch_dir=bsp_patches
patch_list_file=patch_list.txt

if ! [ -f ${patch_dir}/${patch_list_file} ]; then
	echo "Error: ${patch_dir}/${patch_list_file} file does not exist !"
	exit 1;
fi

changelists=`cat ${patch_dir}/${patch_list_file}`
for ch in $changelists
do
	echo "applying patch file...${ch}"
	patch -f -p6 < ${patch_dir}/${ch}.patch >& patch${ch}.out
	cat patch${ch}.out >> patch.out
	rm -f patch${ch}.out
done

echo "completed!"
