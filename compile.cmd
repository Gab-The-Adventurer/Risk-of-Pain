gcc risk_of_pain.c -o risk_of_pain.exe -O1 -Wall -std=c99 -Wno-missing-braces -I include/ -L lib/ -lraylib -lopengl32 -lgdi32 -lwinmm
echo off
echo [92mDone! Check for compilaton warnings :)[0m
exit