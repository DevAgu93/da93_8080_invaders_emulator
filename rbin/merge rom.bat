@echo off
set fname=

IF NOT EXIST invaders.h (set fname=invaders.h
 goto error)
IF NOT EXIST invaders.g (set fname=invaders.g
 goto error)
IF NOT EXIST invaders.f (set fname=invaders.f
 goto error)
IF NOT EXIST invaders.e (set fname=invaders.e
 goto error)
 
type invaders.h invaders.g invaders.f invaders.e > invaders.rom

@echo Created file invaders.rom
goto end

:error

@echo "%fname% not found" 
pause

:end