#!/bin/bash

getargbool 1 rd.brltty && getargbool 1 rd.brltty.sound && getargbool 1 rd.brltty.pulse && {
   chmod a+w /tmp && pulseaudio --daemonize=yes --system --disallow-exit --disallow-module-loading --disable-shm
}
