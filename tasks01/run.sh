#!/bin/bash

gcc -std=c89 -Wall -Wextra -pedantic -o list main.c list.c
./list
