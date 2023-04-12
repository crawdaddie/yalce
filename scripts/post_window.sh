#!/bin/zsh

title=SIMPLE_AUDIO_POST_WINDOW
if [[ $(kitty @ ls | jq -e '.[].tabs[].windows[] | select(.title == "'"$title"'")') ]]; then
    echo "post window exists"
    exit 0
else
    kitty @ launch --cwd $(pwd) --keep-focus --title=SIMPLE_AUDIO_POST_WINDOW ./scripts/follow_logs.sh

    exit 0
fi
