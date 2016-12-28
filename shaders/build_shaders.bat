for %%f in (*.frag) do glslangvalidator -V %%f -o ../build/%%f.spv
for %%f in (*.vert) do glslangvalidator -V %%f -o ../build/%%f.spv
for %%f in (*.geom) do glslangvalidator -V %%f -o ../build/%%f.spv
for %%f in (*.comp) do glslangvalidator -V %%f -o ../build/%%f.spv