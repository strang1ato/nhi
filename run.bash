#!/bin/bash

export LD_PRELOAD="$(pwd)/lib/nhi.so" && \
bash
