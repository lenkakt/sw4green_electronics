cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(greenhouse)

target_sources(app PRIVATE
	src/main.c
	src/app_device.c
	src/app_led.c
	src/app_watchdog.c
	src/app_sensor.c
	src/app_control.c
	src/control_logic.c
	src/app_actuator.c
	src/app_backend.c
)

if(CONFIG_APP_SENSOR_SIMULATE)
	target_sources(app PRIVATE src/app_sensor_sim.c)
else()
	target_sources(app PRIVATE src/app_sensor_hw.c)
endif()

target_sources_ifdef(CONFIG_SHELL app PRIVATE src/app_shell.c)
target_sources_ifdef(CONFIG_WIFI app PRIVATE src/app_wifi.c)
if(NOT CONFIG_WIFI)
	target_sources(app PRIVATE src/app_eth.c)
endif()

target_sources_ifdef(CONFIG_BOOTLOADER_MCUBOOT app PRIVATE src/app_update.c)
