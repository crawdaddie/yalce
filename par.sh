#!/bin/bash

# Start raylib program in the background
./a.out &
raylib_pid=$!

# Run audio_lang in the foreground
build/audio_lang --gui-pid $raylib_pid examples/player.ylc -i 

# After audio_lang exits, kill the raylib program if it's still running
if kill -0 $raylib_pid 2>/dev/null; then
    echo "Terminating raylib program..."
    kill $raylib_pid
    wait $raylib_pid 2>/dev/null
fi

kill $raylib_pid
echo "Execution completed."
