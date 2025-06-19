# shaderun
A tool to interact with `glsl` shaders aimed to streamline the process of
creating and running them. Somewhat compatible with work done using [Shadertoy](https://www.shadertoy.com/).

## dependencies
This project requires [GLEW](https://glew.sourceforge.net/) and [GLFW](https://www.glfw.org/).

## compile
`make`

## running
`Usage: shaderun <fragment_path> [window_width] [window_height] [-o <output_file>]`

## to do/work in progress
- Not sure if the window width and height work yet, would need to mess with my window manager
- Would like to get exporting through ffmpeg working
- - brian (well know graphics man) told me to just sequentially render to texture each frame for
    ffmpeg [here](https://www.opengl-tutorial.org/intermediate-tutorials/tutorial-14-render-to-texture/)


### credits
Credit to `evilryu` for the `test.glsl` file
