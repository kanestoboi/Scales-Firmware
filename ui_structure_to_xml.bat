@echo off
setlocal enabledelayedexpansion

:: Set the root folder to process (relative to the current directory)
set "root_dir=./Components/LCD/ui"
set "output_file=directory_structure.xml"

:: Navigate to the root directory
pushd "%root_dir%" || (
    echo Error: Could not navigate to the root directory "%root_dir%".
    exit /b 1
)

:: Start the XML file
(
    echo ^<?xml version="1.0" encoding="UTF-8"?^>
    echo ^<root^>
) > "%output_file%"

:: Function to process a folder
:process_folder
set "folder_path=%~1"
set "folder_name=%~nx1"

:: Handle the root folder case
if "%folder_name%"=="" set "folder_name=%folder_path%"

:: Write folder tag
(
    echo     ^<folder Name="%folder_name%"^>
) >> "%output_file%"

:: Process files in this folder
for %%f in ("%folder_path%\*.*") do (
    if not "%%~xf"=="" (
        echo         ^<file file_name="%%~f" / ^> >> "%output_file%"
    )
)

:: Process subfolders
for /d %%d in ("%folder_path%\*") do (
    call :process_folder "%%d"
)

:: Close folder tag
(
    echo     ^</folder^>
) >> "%output_file%"

exit /b

:: Start processing from root
call :process_folder "."

:: Close the XML root tag
(
    echo ^</root^>
) >> "%output_file%"

:: Return to the original directory
popd

echo XML
