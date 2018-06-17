#!/bin/bash

hciconfig hci0 up
systemctl --no-block start bluetooth
