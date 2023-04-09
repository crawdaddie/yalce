#!/bin/zsh

function kitty_window_exists() {
  title=$1
  exists=$(kitty @ ls | jq -e '.[].tabs[].windows[] | select(.title == "SIMPLE_AUDIO_POST_WINDOW")' || false)
  if [ "$exists" = false ]; then
    return 1 
  else
    return 0 
  fi
}

if kitty_window_exists SIMPLE_AUDIO_POST_WINDOW; then
  echo "post window already exists"
else
  kitty @ launch --cwd $(pwd) --keep-focus --title=SIMPLE_AUDIO_POST_WINDOW ./scripts/follow_logs.sh
  exit 0
fi
