@echo off

rem goto release

IF NOT EXIST bin\ (mkdir bin)
pushd bin

cl ..\win32.c /Zi /Fe:Emu /nologo /W3 /WX /DBx64 /Od /GS- /link /fixed /incremental:no /opt:icf /opt:ref /subsystem:windows  shell32.lib kernel32.lib user32.lib gdi32.lib Winmm.lib d3d11.lib dxgi.lib d3dcompiler.lib dxguid.lib Ole32.lib 


popd
goto end

:release

rem release
IF NOT EXIST rbin\ (mkdir rbin)
pushd rbin 

cl ..\win32.c /Fe:8080 /nologo /W3 /WX /DBx64 /O2 /GS- /link /fixed /incremental:no /opt:icf /opt:ref /subsystem:windows  shell32.lib kernel32.lib user32.lib gdi32.lib Winmm.lib d3d11.lib dxgi.lib d3dcompiler.lib dxguid.lib Ole32.lib

popd

:end
