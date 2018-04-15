for %%f in (*.png) do ..\tools\image_to_yt\target\release\image_to_yt %%f 

for %%f in (*.yt) do move %%f ..\build
