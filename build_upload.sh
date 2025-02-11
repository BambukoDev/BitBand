#!/bin/env bash

cmake build
cmake --build build
picotool load ./build/HandWatch.uf2 -fx
