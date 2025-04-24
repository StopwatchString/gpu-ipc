# gpu-ipc
A demonstration project for how to use DirectX as a bridge for interprocess sharing of a texture in OpenGL. This method does not require a roundtrip to the CPU to share a texture from one process to another. This is a native feature within DirectX, but is not directly supported in OpenGL. Instead we combine the use of DirectX texture sharing with Nvidia OpenGL extensions that allow for the interoperation of DirectX texture and OpenGL. All drawing in this example occurs in OpenGL. DirectX is only a source for the texture object.

This method of texture sharing drastically lessens load and latency when sharing image information between two processes, as a roundtrip into CPU memory is expensive and has inconsistent timing.

# Building and Running
As this project relies on DirectX for interoperability, the only supported platform is Windows.

1) Clone recursively, so that you have all submodules
2) Run ```cmake --preset="default"``` to generate the solution using build options from the provided 'default' profile preset.
3) Open the solution and change the startup project selection to run both gpu_ipc_Producer and gpu_ipc_Consumer. To do so, right click "Solution 'GPU IPC Project' in the Solution Explorer and select 'Properties'. In the 'Configure Startup Projects' submenu, select 'Multiple startup projects:' then set the action on gpu_ipc_Producer and gpu_ipc_Consumer to 'Start'.
4) Run the solution from Visual Studio

You will see two windows pop up, each their own process started by visual studio. The Producer process generates a DirectX texture and writes to it with OpenGL draw commands. It also publishes a HANDLE to the DirectX Texture and source process information into Shared Memory. The Consumer process reads this information and uses the source process ID to duplicate the DirectX Texture HANDLE for use in its own process. It then opens the DirectX Texture as an OpenGL texture and uses it to texture a rotation quad.

The effect is that you'll see two windows open, one with a static triangle and one with a rotating triangle. But the content within the Consumer process, the one with the rotating triangle, is being sourced entirely from the Producer process without a CPU roundtrip.
