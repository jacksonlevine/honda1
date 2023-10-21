astyle.exe src/*.cpp src/*.hpp

@echo off
for %%f in (src\*.orig) do (
    del "%%f"
)

pause