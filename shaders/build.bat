@echo off

for %%f in (*.frag) do start /b glslc %%f -o ../build/%%f.spv
for %%f in (*.vert) do start /b glslc %%f -o ../build/%%f.spv
for %%f in (*.geom) do start /b glslc %%f -o ../build/%%f.spv
for %%f in (*.comp) do start /b glslc %%f -o ../build/%%f.spv