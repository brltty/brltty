#!/bin/bash

getargbool 1 rd.brltty && getargbool 1 rd.brltty.sound && getargbool 1 rd.brltty.speechd && {
   systemctl -q is-active speech-dispatcherd || {
      systemctl --no-block start speech-dispatcherd
   }
}
