@Echo Off
echo Creating Scales Leveler Firmware Image and Programming

call GenerateDFUPackage.bat

call ProgramFirmwareImage.bat