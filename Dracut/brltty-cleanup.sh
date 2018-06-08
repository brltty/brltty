#!/bin/bash

read pid</var/run/brltty.pid
kill -0 $pid && kill -9 $pid
