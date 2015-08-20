@echo off

set debug=1
set fast=0
set release=0

set export=-EXPORT:GameUpdateAndRender

set commonCompilerFlags= -nologo -Gm- -GR- -EHa- -fp:fast -Oi -WX -W4 -DWIN32=1
set commonLinkerFlags= -incremental:no -opt:ref
set win32Libraries= user32.lib gdi32.lib winmm.lib

set ignoredWarnings= -wd4505 -wd4201 -wd4100 -wd4189
set debugCompilerFlags= %commonCompilerFlags% -MTd -Od %ignoredWarnings% -DINTERNAL=1 -DSLOW=1 -FC -Z7
set fastCompilerFlags= %commonCompilerFlags% -O2 %ignoredWarnings% -DINTERNAL=1 -DSLOW=0 -FC -Z7
set releaseCompilerFlags= %commonCompilerFlags% -O2 -DINTERNAL=0 -DSLOW=0

IF %debug%==1 (set compilerFlags= %debugCompilerFlags%)
IF %fast%==1 (set compilerFlags= %fastCompilerFlags%)
IF %release%==1 (set compilerFlags= %releaseCompilerFlags%)

IF NOT EXIST bin mkdir bin
pushd bin

REM 32bit
REM cl %commonCompilerFlags% ../src/win32/win32.cpp /link -subsystem:windows,5.1 %commonLinkerFlags%

REM 64bit
del *.pdb > NUL 2> NUL
echo WAITING FOR PDB > lock.tmp
cl %compilerFlags% ..\src\game.cpp -LD /link %commonLinkerFlags% -PDB:handmade_%random%.pdb %export%
del lock.tmp

cl %compilerFlags% /I w:\handmade\src ..\src\win32\win32.cpp /link %commonLinkerFlags% %win32Libraries%

popd
