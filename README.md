gm_voxelate
====================
[![forthebadge](http://forthebadge.com/images/badges/compatibility-club-penguin.svg)](http://forthebadge.com)

Welcome to the home of gm_voxelate, the configurable voxel module for Garrysmod.

## Links
[More Information (Facepunch Thread)](http://facepunch.com/showthread.php?t=1466364)

[Development Plan (Trello)](https://trello.com/b/zrWAQgqX/gm-voxelate)

## Documentation
See the wiki in the sidebar.

## Bugs
Please report bugs on this repository's issues page!

## Building
This is designed to be built with glua's build-loadout repository.

It requires a compiler/runtime that supports C++11, which can cause problems on older linux systems.

**Windows**: Should just work.

**Linux**: Hah! Have fun. It will compile, but you will be begging for death before you have a working binary. You will need to make a lot of changes to the makefile, and even edit part of the Source Sdk code. I will explain more and try to address this eventually.

**Mac**: Unknown, probably similar to Linux.

The test_install scripts are for quickly copying the binaries into directories on my systems.

## Used In
[gm_f1atgrass](https://github.com/glua/gm_f1atgrass) - Testbed for the module
