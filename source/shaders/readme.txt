This is a very bad attempt at ghetto rigging together the shader compile pipeline, which is already pretty jank in the first place.

You can only build on windows. I don't know that Valve even makes a version for nix.

1. Install http://strawberryperl.com/
	- You may need to install some additional modules using the commands below...
		> cpan String::CRC32
2. Check if "fxc" is a valid command on your VS command prompt. If not, install the DirectX SDK. The version shouldn't matter, just get one that works with your VS.
3. Run "buildvoxelshaders.bat" from your VS command prompt.