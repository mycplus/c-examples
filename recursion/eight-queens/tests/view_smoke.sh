#!/bin/sh
# Opens queens_view in a virtual X display, presses keys and checks the
# window title follows them, then quits with Esc. Linux only; needs
# xvfb-run and xdotool.
set -eu
view=$1
xvfb-run -a sh -c '
  "$0" 8 &
  pid=$!
  title() { xdotool search --sync --name "8 queens" getwindowname | head -n 1; }
  t1=$(title)
  xdotool search --sync --name "8 queens" windowfocus --sync key --window %1 Right Right
  sleep 1
  t2=$(title)
  xdotool search --sync --name "8 queens" windowfocus --sync key --window %1 Left
  sleep 1
  t3=$(title)
  xdotool search --sync --name "8 queens" windowfocus --sync key --window %1 Escape
  wait $pid
  status=$?
  echo "$t1 / $t2 / $t3 / exit $status"
  [ "$t1" = "8 queens: solution 1 of 92" ]
  [ "$t2" = "8 queens: solution 3 of 92" ]
  [ "$t3" = "8 queens: solution 2 of 92" ]
  [ "$status" -eq 0 ]
' "$view"
