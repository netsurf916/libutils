#!/bin/bash

find . -not -path '*/\.*' -iname '*.[h|c]*' | xargs wc -l

