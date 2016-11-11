;glslangvalidator -V basic.vert
;glslangvalidator -V basic.frag
;glslangvalidator -V wireframe.geom

;xcopy /Y .\*.spv ..\build

for %%f in (*.frag) do glslangvalidator -V %%f -o ../build/%%f.spv
for %%f in (*.vert) do glslangvalidator -V %%f -o ../build/%%f.spv
for %%f in (*.geom) do glslangvalidator -V %%f -o ../build/%%f.spv
del *.spv