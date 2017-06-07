gm_voxelate
====================
[![forthebadge](http://forthebadge.com/images/badges/compatibility-club-penguin.svg)](http://forthebadge.com)

Welcome to the home of gm_voxelate, the configurable voxel module for Garrysmod.

## Links
**NO EXTERNAL LINKS RIGHT NOW. A NEW FACEPUNCH THREAD WILL BE CREATED FOR VERSION 0.3.0.**

## Documentation
See the wiki in the sidebar.

**DOCS ARE CURRENTLY OUTDATED. API SUBJECT TO CHANGE AND WILL BE STABILIZED IN VERSION 0.4.0.**

## Support

- **Windows**: :heavy_check_mark: Works.
- **Linux**: :question: Shaders not tested.
- **Mac**: :question: Shaders not tested.

## Bugs
Please report bugs on this repository's issues page!

## Building
This is designed to be built with [MetaMan](https://github.com/danielga)'s [Garry's Mod](https://github.com/danielga/garrysmod_common) and [Source SDK](https://github.com/danielga/sourcesdk-minimal) headers.

It requires a compiler/runtime that supports C++11, which can cause problems on older linux systems.

**YOU SHOULD PROBABLY JUST DEBUG ON RELEASE BUILDS BECAUSE THE DEBUG SETTINGS ARE A BIT AGGRESSIVE AND MAKE THIS RUN SLOW AS BALLS AND I HAVN'T BOTHERED TO FIX IT.**

### Quick Start
**THIS GUIDE IS WINDOWS-ONLY.**
**SETTINGS IN THIS GUIDE ARE NOT IDEAL FOR RELEASES.**
1. Clone this, garrysmod_common, and sourcesdk-minimal into the same directory.
2. Get a **recent build** of premake5 and put it in your path. Official builds may not be new enough. Contact us if you have issues with this...
3. Run "gm_voxelate/projects/setup.bat". If you want to use something other than VS2017, just change that part of the command.
4. Run the shader install script in the repo's root directory.

At this point you should have working VS projects to build the modules with. Upon build, modules will automatically be copied to your garrysmod directory, assuming it is in the default place on the C drive.

If you want lua hotloading, make a `.vox_hotloading` file as described in that section.

### Auto Install
The `--autoinstall` flag can be passed to premake to automatically copy binaries to the garrysmod directory on each build. Only supported on windows. Assumes the gmod directory is "C:\Program Files (x86)\Steam\steamapps\common\GarrysMod\garrysmod\".

### Shaders
This module needs to load custom shaders. Fortunately, compiled shaders are shipped with the module. Unfortunately, they still need to be copied to the garrysmod directory. You can either run "install_shaders.bat", which assumes that the gmod directory is the same as above, or manually copy the shaders from "source/shaders/shaders" to "Garrysmod/garrysmod/shaders".

If you actually want to build shaders, follow the instructions in "source/shaders/readme.txt".

### Lua Hotloading

Right now, the Lua portion of gm\_voxelate is compiled into the DLL, and is unmodifiable at runtime. This can get quite annoying during extended sessions of modifying the Lua portion of gm\_voxelate exclusively. As such, you may enable lua hotloading to ease the burden of development and debugging, which directly reads files from the OS filesystem and loads them into Lua.

Hotloading can be enabled by creating a file in GarrysMod/garrysmod/data with the name `.vox_hotloading`, whose contents is the path to the `lua` folder in this repo. **NO NEWLINES, NO TRAILING SPACES!**

Secondly, you will either:
1. install [gm_luaiox](https://cdn.discordapp.com/attachments/152162730244177920/311479180141527041/gmsv_luaiox_win32.dll) (kindly provided by MetaMan)
2. compile gm_voxelate with the premake trigger `--luahotloading`, which will add a dangerous function that can read any file without restrictions

Enjoy!

P.S. fair word of warning:
![warning](https://cdn.discordapp.com/attachments/152162730244177920/311486688025247744/unknown.png)

## Technical details for using this in a gamemode

**IGNORE THIS.**

### Required libraries/dependencies

We generally don't depend on garryFuncs:tm:
You should be good to completely override `includes/init.lua`, as long as the stock luajit 5.1 funcs aren't modified, and a version of the `hook` and `timer` libraries is available for use.

### Sub entities

#### Voxelate engine based

use `gm_voxelate:RegisterSubEntity(className,classObj)` and it will internally extend the VoxelEntity class

#### Source engine based

extend `voxel_entity`

note: both the voxelate engine and source engine entities are mutually exclusive: you may only use one type at a time.

## Used In
[gm_f1atgrass](https://github.com/glua/gm_f1atgrass) - Testbed for the module
