@echo off

set CommonCompilerFlags=-MTd -nologo -Gm- -GR- -EHa- -Od -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4456 -DGRAPHICS_INTERNAL=1 -DGRAPHICS_SLOW=1 -DGRAPHICS_WIN32=1 -FC -Z7
set CommonLinkerFlags= -incremental:no -opt:ref user32.lib gdi32.lib winmm.lib

REM TODO: - can we just build both with one exe?

IF NOT EXIST ..\..\build mkdir ..\..\build
pushd ..\..\build
IF EXIST win32_graphics.ilk DEL /F /S /Q /A win32_graphics.ilk
IF EXIST win32_graphics.pdb DEL /F /S /Q /A win32_graphics.pdb

REM 32-bit build
REM cl %CommonCompilerFlags% ..\graphics\code\win32_graphics.cpp /link -subsystem:windows,5.1 %CommonLinkerFlags%

REM 64-bit build
del *.pdb > NUL 2> NUL
REM cl %CommonCompilerFlags% ..\graphics\code\graphics.cpp -Fmgraphics.map -LD /link -incremental:no -opt:ref -PDB:graphics_%random%.pdb -EXPORT:GameGetSoundSamples -EXPORT:GameUpdateAndRender
cl %CommonCompilerFlags% ..\graphics\code\win32_graphics.cpp -Fmwin32_graphics.map /link %CommonLinkerFlags%
popd
