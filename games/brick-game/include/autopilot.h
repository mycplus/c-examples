/* autopilot.h - a simple computer player, used for the demo and the tests. */
#ifndef AUTOPILOT_H
#define AUTOPILOT_H

#include "breakout.h"

/* Moves the paddle toward where a falling ball will reach the paddle row,
 * ignoring bricks, so that the ball lands on the paddle's centre. */
Input autopilot_input(const Game *g);

#endif
