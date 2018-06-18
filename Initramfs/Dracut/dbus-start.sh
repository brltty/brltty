#!/bin/bash

getargbool 1 rd.brltty && getargbool 1 rd.brltty.bluetooth && {
   systemctl -q is-active dbus || {
      systemctl --no-block start dbus
   }
}
