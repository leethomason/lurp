REM writes the binaries and script files to a zippable folder
REM Useage, in the root of the project:
REM package lurp_0.5.0

cmake --build build --config Release

mkdir %1

copy /Y *.md %1
copy /Y LICENSE %1

copy /Y build\Release\lurp.exe %1
mkdir %1\script
copy /Y script %1\script

mkdir %1\game
xcopy /E /Y game\*.* %1\game

cd %1
lurp.exe --noScan
if %errorlevel% neq 0 exit /b %errorlevel%
cd ..

git status
echo %1 is ready for zipping if git status is clean
