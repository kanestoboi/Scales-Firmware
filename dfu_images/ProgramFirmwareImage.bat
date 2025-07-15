@Echo Off

echo Programming Firmware Image to Device
nrfjprog --recover
nrfjprog -f nrf52 --program bl_sd_settings_app.hex --verify
nrfjprog --reset
