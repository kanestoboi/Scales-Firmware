
@Echo off
echo Compiling Release
rm -r ../output
rm -r ../../Scales-Bootloader/output

emBuild -config "Release" -echo ../Scales_pca10056_s140.emProject
emBuild -config "Release" -echo ../../Scales-Bootloader/ScalesBootloader_s140_pca10056.emProject


set build-configuration=Release

set majorFirmwareVersion=0
set minorFirmwareVersion=2
set patchFirmwareVersion=0

@REM set the firmware version as a uint32 where each bytes represents the major, minor and patch versionsS
set /a firmwareVersion=(majorFirmwareVersion * 65536) + (minorFirmwareVersion * 256) + patchFirmwareVersion

echo Firmware version: 0x%firmwareVersion%

set /A bootloaderVersion=1
@REM The bl settings version depends on the SDK version. >= 15.3.0 is bl settings version 2
set /A blSettingsVersion=2  

set application-hex=../Output/%build-configuration%/Exe/Scales.hex
set soft-device-hex=../../nRF5_SDK_Current/components/softdevice/s140/hex/s140_nrf52_7.2.0_softdevice.hex
set bootloader-hex=../../Scales-Bootloader/Output/Release/Exe/ScalesSecureBootloader_s140_pca10056.hex

echo Generating DFU Settings Page
nrfutil.exe settings generate --family NRF52840 --application %application-hex% --application-version %firmwareVersion% --bootloader-version %bootloaderVersion% --bl-settings-version %blSettingsVersion% bl_settings.hex

echo Merging Bootloader, Soft Device, Settings, and Application
mergehex --merge bl_settings.hex %bootloader-hex% %soft-device-hex% %application-hex% --output bl_sd_settings_app.hex

nrfutil.exe pkg generate --hw-version 52 --application-version %firmwareVersion% --application %application-hex% --sd-req 0x0100 --sd-id 0x0100 --key-file keys/private.key scales_application_v%majorFirmwareVersion%.%minorFirmwareVersion%.%patchFirmwareVersion%_s140.zip

