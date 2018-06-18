#!/bin/bash

read </run/pulse/pid pid && kill -0 "${pid}" && kill -KILL "${pid}"
