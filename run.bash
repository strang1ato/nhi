#!/bin/bash

export LD_PRELOAD="$(pwd)/lib/bash/nhi.so" && \
bash
