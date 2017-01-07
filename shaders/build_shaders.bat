for %%f in (*.frag) do glslc %%f -o ../build/%%f.spv
for %%f in (*.vert) do glslc %%f -o ../build/%%f.spv
for %%f in (*.geom) do glslc %%f -o ../build/%%f.spv
for %%f in (*.comp) do glslc %%f -o ../build/%%f.spv