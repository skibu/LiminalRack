#!/bin/bash
# Sets up configuration of raspberry pi for Liminal Rack.
# To run this script from a terminal on laptop use:
#   ssh liminal@raspberrypi.local ‘bash -s’ < liminal_rpi5_config.sh unique_name_param

# Enable gen3 speed for NVMe SSD drive
printf "\n# Enable gen3 speed for NVMe SSD (when available) \ndtparam=pciex1 \n#dtparam=pciex1_gen=3\n" | sudo tee -a /boot/firmware/config.txt

# Enable Waveshare display 8-DSI-TOUCH-A on port DSI1
printf "\n# Enable Waveshare display 8-DSI-TOUCH-A on port DSI1 \ndtoverlay=vc4-kms-dsi-waveshare-panel-v2,8_0_inch_a\n" | sudo tee -a /boot/firmware/config.txt

# Overclock CPU from default of 2.4GHz to 2.8Ghz 
printf "\n# Overclock CPU from default of 2.4GHz to 2.8Ghz \narm_freq=2800\n" | sudo tee -a /boot/firmware/config.txt 

# Set fan to only come on when temperature is greater than 60 degrees
printf "\n# Set fan to only come on when temperature is greater than 60 degrees\n\
dtparam=fan_temp0=60000\n\
dtparam=fan_temp0_hyst=5000\n\
dtparam=fan_temp0_speed=75\n\
\n\
dtparam=fan_temp1=70000\n\
dtparam=fan_temp1_hyst=5000\n\
dtparam=fan_temp1_speed=128\n\
\n\
dtparam=fan_temp2=75000\n\
dtparam=fan_temp2_hyst=5000\n\
dtparam=fan_temp2_speed=192\n\
\n\
dtparam=fan_temp3=80000\n\
dtparam=fan_temp3_hyst=5000\n\
dtparam=fan_temp3_speed=255\n" | sudo tee -a /boot/firmware/config.txt

# Autohide taskbar (once really ned task bar anymore)
printf "\n# Autohide taskbar \nautohide=false\nautohide_duration=500\n" >> ~/.config/wf-panel-pi/wf-panel-pi.ini

# Change HDMI-1 desktop to show moon
printf "\n# Change desktop \nshow_trash=0\ndesktop_bg=#000000\nwallpaper=/usr/share/rpd-wallpaper/moon.jpg\n" >> ~/.config/pcmanfm/default/desktop-items-HDMI-A-1.conf

# Change Waveshare DSI display to show moon
printf "\n# Change desktop \nshow_trash=0\ndesktop_bg=#000000\nwallpaper=/usr/share/rpd-wallpaper/moon.jpg\n" >> ~/.config/pcmanfm/default/desktop-items-DSI-2.conf

# Or change desktop to just show black
#printf "\n# Change desktop \nshow_trash=0\ndesktop_bg=#000000\nwallpaper=\n" >> ~/.config/pcmanfm/default/desktop-items-HDMI-A-1.conf

# Enable ssh access
sudo raspi-config nonint do_ssh 0

# Enable wireless connect so can access via web browser.
# See https://www.raspberrypi.com/documentation/services/connect.html
sudo raspi-config nonint do_rpi_connect 0

# Set hostname, with script param $1 appended to end to make unique
sudo raspi-config nonint do_hostname liminal$1

# WiFi
sudo raspi-config nonint do_wifi_ssid_passphrase Funtara 536536536

# Try booting from NVMe SSD card first
sudo raspi-config nonint do_boot_order B2

# Boot without waiting for network (to speed up boot)
sudo raspi-config nonint do_boot_wait 0

# Display splash screen instead of ugly boot text
sudo raspi-config nonint do_boot_splash 0

# Disable screen blanking after being idle
sudo raspi-config nonint do_blanking 1

# Enable SPI
sudo raspi-config nonint do_spi 0

# Enable I2C
sudo raspi-config nonint do_i2c 0

# Boot to desktop, no login required
sudo raspi-config nonint do_boot_behaviour B4

# Set to California time zone
sudo raspi-config nonint do_change_timezone America/Los_Angeles

# Disable screenreader called Orca
sudo rm /etc/xdg/autostart/orca-autostart.desktop

# Load in LiminalRack
cd
mkdir vscode-projects
cd vscode-projects
git checkout https://github.com/skibu/LiminalRack.git

# Load in virtual keyboard
sudo cp ~/vscode-projects/LiminalRack/res/us_wide.yaml /usr/share/misc/squeekboard/keyboards/

# Need to reboot to have everything take effect
sudo reboot
