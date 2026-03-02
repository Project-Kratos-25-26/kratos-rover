#!/bin/bash
host=$1
port=$2
gst-launch-1.0 tcpclientsrc host=$host port=$port ! matroskademux ! av1parse ! av1dec ! videoconvert ! autovideosink sync=false
