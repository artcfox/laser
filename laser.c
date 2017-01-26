/*

  laser.c

  Copyright 2016 Matthew T. Pandina. All rights reserved.

  This file is part of Laser.

  Laser is free software: you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Laser is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Laser.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>
#include <stdlib.h>
#include <string.h>
#include <avr/pgmspace.h>
#include <uzebox.h>

#include "data/tileset.inc"
#include "data/sprites.inc"
#include "data/instructions.inc"
#include "data/patches.inc"
#include "data/midisong.h"

typedef struct {
  uint16_t held;
  uint16_t prev;
  uint16_t pressed;
  uint16_t released;
} __attribute__ ((packed)) BUTTON_INFO;

#define TILE_BACKGROUND  1

// Defines for the pieces. Rotations are treated as different pieces
#define P_BLANK 0
#define P_BLOCKER 1
#define P_TARGET_T 2
#define P_TARGET_R 3
#define P_TARGET_B 4
#define P_TARGET_L 5
#define P_MIRROR_BL 6
#define P_MIRROR_TL 7
#define P_MIRROR_TR 8
#define P_MIRROR_BR 9
#define P_SPLIT_TLBR 10
#define P_SPLIT_TRBL 11

/* Each position on the board may have a laser beam going in and/or out in any direction
      IN   OUT
   0b 0000 0000
       \\\\ \\\\__ top
        \\\\ \\\__ bottom
         \\\\ \\__ left
          \\\\ \__ right
           \\\\
            \\\\__ top
             \\\__ bottom
              \\__ left
               \__ right
*/
#define D_OUT_T 1
#define D_OUT_B 2
#define D_OUT_L 4
#define D_OUT_R 8

#define D_IN_T 16
#define D_IN_B 32
#define D_IN_L 64
#define D_IN_R 128

// The configuration of the playing board (with the laser off)
uint8_t board[5][5] = {
  {  0,  0,  0,  0,  0 },
  {  0,  0,  0,  0,  0 },
  {  0,  0,  0,  0,  0 },
  {  0,  0,  0,  0,  0 },
  {  0,  0,  0,  0,  0 },
};

// The bitmap of where the laser is, and which direction(s) it is travelling
uint8_t laser[5][5] = {
  {  0,  0,  0,  0,  0 },
  {  0,  0,  0,  0,  0 },
  {  0,  0,  0,  0,  0 },
  {  0,  0,  0,  0,  0 },
  {  0,  0,  0,  0,  0 },
};

// The pieces in your "hand" (that need to be placed on the board)
uint8_t hand[5] = { 0, 0, 0, 0, 0 };

const uint8_t levelData[] PROGMEM = {
  // LEVEL 1
  // Puzzle
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, P_TARGET_T, 0, 0,
  // Solution
  0, 0, 0, 0, 0,
  0, 0, P_MIRROR_BL, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, P_TARGET_T, 0, 0,
  // Hand
  P_MIRROR_TL, 0, 0, 0, 0,

  // LEVEL 2
  // Puzzle
  0, 0, 0, 0, 0,
  0, 0, 0, 0, P_TARGET_L,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, P_TARGET_T, 0, 0,
  // Solution
  0, 0, 0, 0, 0,
  0, 0, P_SPLIT_TLBR, 0, P_TARGET_L,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, P_TARGET_T, 0, 0,
  // Hand
  P_SPLIT_TRBL, 0, 0, 0, 0,

  // LEVEL 3
  // Puzzle
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, P_TARGET_R, 0,
  0, 0, 0, 0, 0,
  // Solution
  0, 0, 0, 0, 0,
  0, 0, 0, 0, P_MIRROR_BL,
  0, 0, 0, 0, 0,
  0, 0, 0, P_TARGET_R, P_MIRROR_TL,
  0, 0, 0, 0, 0,
  // Hand
  P_MIRROR_TL, P_MIRROR_TL, 0, 0, 0,

  // LEVEL 4
  // Puzzle
  0, P_TARGET_R, 0, P_BLOCKER, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  // Solution
  0, P_TARGET_R, P_MIRROR_BL, P_BLOCKER, 0,
  0, 0, P_MIRROR_TL, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  // Hand
  P_MIRROR_TL, P_MIRROR_TL, 0, 0, 0,
  
  // LEVEL 5
  // Puzzle
  0, 0, 0, 0, 0,
  0, P_BLOCKER, P_TARGET_B, 0, 0,
  0, 0, 0, P_BLOCKER, 0,
  P_BLOCKER, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  // Solution
  0, 0, 0, 0, 0,
  P_MIRROR_BL, P_BLOCKER, P_TARGET_B, 0, 0,
  P_MIRROR_TR, 0, P_MIRROR_TL, P_BLOCKER, 0,
  P_BLOCKER, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  // Hand
  P_MIRROR_TL, P_MIRROR_TL, P_MIRROR_TL, 0, 0,

  // LEVEL 6
  // Puzzle
  0, 0, 0, 0, 0,
  0, P_BLOCKER, 0, 0, 0,
  0, 0, P_BLOCKER, 0, 0,
  0, 0, 0, 0, P_TARGET_T,
  0, 0, 0, 0, 0,
  // Solution
  P_MIRROR_BR, 0, 0, 0, P_MIRROR_BL,
  P_MIRROR_TL, P_BLOCKER, 0, 0, 0,
  0, 0, P_BLOCKER, 0, 0,
  0, 0, 0, 0, P_TARGET_T,
  0, 0, 0, 0, 0,
  // Hand
  P_MIRROR_TL, P_MIRROR_TL, P_MIRROR_TL, 0, 0,

  // LEVEL 7
  // Puzzle
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, P_SPLIT_TLBR, 0, 0, P_TARGET_L,
  0, 0, 0, 0, 0,
  0, P_TARGET_T, 0, 0, 0,
  // Solution
  0, 0, 0, 0, 0,
  0, P_MIRROR_BL, 0, 0, 0,
  0, P_SPLIT_TLBR, 0, 0, P_TARGET_L,
  0, 0, 0, 0, 0,
  0, P_TARGET_T, 0, 0, 0,
  // Hand
  P_MIRROR_TL, 0, 0, 0, 0,

  // LEVEL 8
  // Puzzle
  0, 0, 0, 0, 0,
  0, P_MIRROR_BL, 0, 0, 0,
  P_BLOCKER, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  P_TARGET_T, 0, 0, 0, 0,
  // Solution
  0, 0, 0, 0, 0,
  0, P_MIRROR_BL, 0, 0, 0,
  P_BLOCKER, 0, 0, 0, 0,
  P_MIRROR_BR, P_MIRROR_TL, 0, 0, 0,
  P_TARGET_T, 0, 0, 0, 0,
  // Hand
  P_MIRROR_TL, P_MIRROR_TL, 0, 0, 0,

  // LEVEL 9
  // Puzzle
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, P_TARGET_L,
  0, 0, 0, P_TARGET_T, 0,
  // Solution
  0, 0, 0, 0, 0,
  0, 0, 0, P_MIRROR_BL, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, P_SPLIT_TLBR, P_TARGET_L,
  0, 0, 0, P_TARGET_T, 0,
  // Hand
  P_SPLIT_TRBL, P_MIRROR_TL, 0, 0, 0,

  // LEVEL 10
  // Puzzle
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, P_TARGET_B, 0,
  0, 0, P_BLOCKER, 0, 0,
  0, 0, 0, P_BLOCKER, 0,
  // Solution
  0, 0, 0, 0, 0,
  0, 0, 0, 0, P_MIRROR_BL,
  0, 0, 0, P_TARGET_B, 0,
  0, 0, P_BLOCKER, P_MIRROR_TR, P_MIRROR_TL,
  0, 0, 0, P_BLOCKER, 0,
  // Hand
  P_MIRROR_TL, P_MIRROR_TL, P_MIRROR_TL, 0, 0,

  // LEVEL 11
  // Puzzle
  0, 0, 0, 0, P_TARGET_B,
  0, 0, 0, P_MIRROR_BL, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, P_BLOCKER,
  0, 0, 0, P_TARGET_T, 0,
  // Solution
  0, 0, 0, 0, P_TARGET_B,
  0, 0, 0, P_MIRROR_BL, 0,
  0, 0, 0, P_SPLIT_TLBR, P_MIRROR_TL,
  0, 0, 0, 0, P_BLOCKER,
  0, 0, 0, P_TARGET_T, 0,
  // Hand
  P_SPLIT_TRBL, P_MIRROR_TL, 0, 0, 0,

  // LEVEL 12
  // Puzzle
  0, 0, P_TARGET_R, 0, 0,
  0, 0, 0, P_MIRROR_BL, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, P_MIRROR_TL,
  0, 0, 0, 0, 0,
  // Solution
  0, 0, P_TARGET_R, 0, P_MIRROR_BL,
  0, 0, 0, P_MIRROR_BL, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, P_MIRROR_TR, P_MIRROR_TL,
  0, 0, 0, 0, 0,
  // Hand
  P_MIRROR_TL, P_MIRROR_TL, 0, 0, 0,

  // LEVEL 13
  // Puzzle
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, P_BLOCKER, 0, 0, 0,
  0, 0, P_TARGET_L, 0, 0,
  P_BLOCKER, 0, P_TARGET_L, 0, 0,
  // Solution
  0, 0, 0, 0, 0,
  P_MIRROR_BL, 0, 0, 0, 0,
  0, P_BLOCKER, 0, 0, 0,
  P_MIRROR_TR, P_SPLIT_TLBR, P_TARGET_L, 0, 0,
  P_BLOCKER, P_MIRROR_TR, P_TARGET_L, 0, 0,
  // Hand
  P_SPLIT_TRBL, P_MIRROR_TL, P_MIRROR_TL, P_MIRROR_TL, 0,

  // LEVEL 14
  // Puzzle
  P_TARGET_R, 0, 0, 0, 0,
  0, 0, 0, 0, P_MIRROR_TL,
  0, 0, P_BLOCKER, 0, P_TARGET_B,
  0, P_BLOCKER, 0, P_BLOCKER, 0,
  0, 0, 0, 0, P_MIRROR_TL,
  // Solution
  P_TARGET_R, 0, 0, 0, P_MIRROR_BL,
  P_SPLIT_TLBR, 0, 0, 0, P_MIRROR_TL,
  0, 0, P_BLOCKER, 0, P_TARGET_B,
  0, P_BLOCKER, 0, P_BLOCKER, 0,
  P_MIRROR_TR, 0, 0, 0, P_MIRROR_TL,
  // Hand
  P_SPLIT_TRBL, P_MIRROR_TL, P_MIRROR_TL, 0, 0,

  // LEVEL 15
  // Puzzle
  0, 0, 0, P_TARGET_B, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, P_TARGET_T, 0, 0, 0,
  // Solution
  0, 0, 0, P_TARGET_B, 0,
  0, P_SPLIT_TLBR, 0, P_MIRROR_TL, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, P_TARGET_T, 0, 0, 0,
  // Hand
  P_SPLIT_TRBL, P_MIRROR_TL, 0, 0, 0,

  // LEVEL 16
  // Puzzle
  0, 0, 0, P_TARGET_R, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, P_TARGET_T, 0, 0, 0,
  // Solution
  0, 0, 0, P_TARGET_R, P_MIRROR_BL,
  0, P_SPLIT_TLBR, 0, 0, P_MIRROR_TL,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, P_TARGET_T, 0, 0, 0,
  // Hand
  P_SPLIT_TRBL, P_MIRROR_TL, P_MIRROR_TL, 0, 0,

  // LEVEL 17
  // Puzzle
  0, 0, 0, 0, 0,
  P_SPLIT_TLBR, P_MIRROR_BL, 0, 0, 0,
  0, 0, P_MIRROR_BL, 0, 0,
  P_BLOCKER, 0, 0, P_TARGET_L, 0,
  0, 0, P_TARGET_T, 0, 0,
  // Solution
  0, 0, 0, 0, 0,
  P_SPLIT_TLBR, P_MIRROR_BL, 0, 0, 0,
  P_MIRROR_TR, 0, P_MIRROR_BL, 0, 0,
  P_BLOCKER, P_MIRROR_TR, 0, P_TARGET_L, 0,
  0, 0, P_TARGET_T, 0, 0,
  // Hand
  P_MIRROR_TL, P_MIRROR_TL, 0, 0, 0,

  // LEVEL 18
  // Puzzle
  P_BLOCKER, 0, 0, 0, P_MIRROR_BL,
  0, P_BLOCKER, P_TARGET_T, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, P_BLOCKER,
  0, 0, 0, 0, 0,
  // Solution
  P_BLOCKER, 0, P_MIRROR_BR, 0, P_MIRROR_BL,
  P_MIRROR_BL, P_BLOCKER, P_TARGET_T, 0, 0,
  P_MIRROR_TR, 0, 0, 0, P_MIRROR_TL,
  0, 0, 0, 0, P_BLOCKER,
  0, 0, 0, 0, 0,
  // Hand
  P_MIRROR_TL, P_MIRROR_TL, P_MIRROR_TL, P_MIRROR_TL, 0,

  // LEVEL 19
  // Puzzle
  0, P_TARGET_B, 0, 0, 0,
  0, 0, 0, 0, P_MIRROR_BL,
  0, 0, 0, 0, 0,
  0, P_BLOCKER, 0, 0, 0,
  0, 0, 0, 0, 0,
  // Solution
  0, P_TARGET_B, 0, 0, 0,
  0, 0, 0, 0, P_MIRROR_BL,
  0, P_MIRROR_TR, 0, 0, P_MIRROR_TL,
  0, P_BLOCKER, 0, 0, 0,
  0, 0, 0, 0, 0,
  // Hand
  P_MIRROR_TL, P_MIRROR_TL, 0, 0, 0,

  // LEVEL 20
  // Puzzle
  0, 0, 0, P_TARGET_B, 0,
  0, 0, 0, 0, 0,
  P_TARGET_R, 0, 0, P_SPLIT_TLBR, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  // Solution
  0, 0, 0, P_TARGET_B, 0,
  0, 0, 0, 0, P_MIRROR_BL,
  P_TARGET_R, 0, 0, P_SPLIT_TLBR, P_MIRROR_TL,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  // Hand
  P_MIRROR_TL, P_MIRROR_TL, 0, 0, 0,

  // LEVEL 21
  // Puzzle
  0, 0, P_MIRROR_BR, 0, 0,
  0, 0, 0, 0, P_MIRROR_TL,
  P_TARGET_B, 0, 0, 0, 0,
  0, P_BLOCKER, 0, 0, P_TARGET_L,
  0, 0, P_MIRROR_TL, 0, 0,
  // Solution
  0, 0, P_MIRROR_BR, 0, P_MIRROR_BL,
  0, 0, 0, 0, P_MIRROR_TL,
  P_TARGET_B, 0, 0, 0, 0,
  0, P_BLOCKER, P_SPLIT_TLBR, 0, P_TARGET_L,
  P_MIRROR_TR, 0, P_MIRROR_TL, 0, 0,
  // Hand
  P_SPLIT_TRBL, P_MIRROR_TL, P_MIRROR_TL, 0, 0,

  // LEVEL 22
  // Puzzle
  0, 0, 0, 0, 0,
  0, 0, 0, 0, P_BLOCKER,
  0, P_MIRROR_TR, 0, 0, 0,
  P_TARGET_R, 0, 0, 0, 0,
  0, 0, 0, 0, P_TARGET_T,
  // Solution
  0, 0, 0, 0, 0,
  0, P_MIRROR_BL, 0, 0, P_BLOCKER,
  0, P_MIRROR_TR, 0, 0, P_MIRROR_BL,
  P_TARGET_R, 0, 0, 0, P_SPLIT_TRBL,
  0, 0, 0, 0, P_TARGET_T,
  // Hand
  P_SPLIT_TRBL, P_MIRROR_TL, P_MIRROR_TL, 0, 0,

  // LEVEL 23
  // Puzzle
  0, P_MIRROR_BL, 0, 0, 0,
  0, 0, 0, 0, P_MIRROR_BL,
  P_TARGET_T, 0, 0, 0, 0,
  0, P_TARGET_R, 0, 0, P_MIRROR_TL,
  0, 0, 0, 0, 0,
  // Solution
  P_MIRROR_BR, P_MIRROR_BL, 0, 0, 0,
  0, P_SPLIT_TRBL, 0, 0, P_MIRROR_BL,
  P_TARGET_T, 0, 0, 0, 0,
  0, P_TARGET_R, 0, 0, P_MIRROR_TL,
  0, 0, 0, 0, 0,
  // Hand
  P_SPLIT_TRBL, P_MIRROR_TL, 0, 0, 0,

  // LEVEL 24
  // Puzzle
  0, 0, 0, P_TARGET_B, 0,
  0, 0, 0, 0, 0,
  0, 0, P_BLOCKER, 0, 0,
  0, 0, 0, P_TARGET_T, 0,
  0, 0, 0, 0, 0,
  // Solution
  0, 0, 0, P_TARGET_B, 0,
  0, 0, 0, P_SPLIT_TRBL, P_MIRROR_BL,
  0, 0, P_BLOCKER, P_MIRROR_BR, P_MIRROR_TL,
  0, 0, 0, P_TARGET_T, 0,
  0, 0, 0, 0, 0,
  // Hand
  P_SPLIT_TRBL, P_MIRROR_TL, P_MIRROR_TL, P_MIRROR_TL, 0,

  // LEVEL 25
  // Puzzle
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, P_BLOCKER, 0, 0,
  P_TARGET_R, 0, 0, 0, 0,
  0, P_BLOCKER, 0, 0, P_TARGET_L,
  // Solution
  0, 0, 0, 0, 0,
  0, 0, 0, P_MIRROR_BL, 0,
  0, 0, P_BLOCKER, 0, 0,
  P_TARGET_R, 0, 0, P_SPLIT_TRBL, 0,
  0, P_BLOCKER, 0, P_MIRROR_TR, P_TARGET_L,
  // Hand
  P_SPLIT_TRBL, P_MIRROR_TL, P_MIRROR_TL, 0, 0,

  // LEVEL 26
  // Puzzle
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  P_BLOCKER, 0, P_SPLIT_TLBR, 0, 0,
  0, 0, 0, P_TARGET_L, P_TARGET_T,
  // Solution
  0, 0, 0, 0, 0,
  0, P_MIRROR_BL, 0, 0, 0,
  0, 0, 0, 0, 0,
  P_BLOCKER, P_MIRROR_TR, P_SPLIT_TLBR, 0, P_MIRROR_BL,
  0, 0, P_MIRROR_TR, P_TARGET_L, P_TARGET_T,
  // Hand
  P_MIRROR_TL, P_MIRROR_TL, P_MIRROR_TL, P_MIRROR_TL, 0,

  // LEVEL 27
  // Puzzle
  P_TARGET_B, 0, P_MIRROR_BR, 0, P_BLOCKER,
  0, 0, 0, 0, 0,
  0, 0, P_BLOCKER, 0, P_TARGET_T,
  P_MIRROR_TR, 0, 0, P_MIRROR_TL, 0,
  0, 0, 0, 0, 0,
  // Solution
  P_TARGET_B, 0, P_MIRROR_BR, P_MIRROR_BL, P_BLOCKER,
  0, 0, P_SPLIT_TRBL, 0, P_MIRROR_BL,
  0, 0, P_BLOCKER, 0, P_TARGET_T,
  P_MIRROR_TR, 0, 0, P_MIRROR_TL, 0,
  0, 0, 0, 0, 0,
  // Hand
  P_SPLIT_TRBL, P_MIRROR_TL, P_MIRROR_TL, 0, 0,

  // LEVEL 28
  // Puzzle
  P_MIRROR_BR, 0, 0, P_BLOCKER, 0,
  0, 0, P_BLOCKER, 0, 0,
  P_BLOCKER, 0, 0, 0, 0,
  0, 0, 0, P_SPLIT_TLBR, 0,
  0, 0, 0, 0, 0,
  // Solution
  P_MIRROR_BR, P_MIRROR_BL, 0, P_BLOCKER, 0,
  P_MIRROR_TL, 0, P_BLOCKER, 0, 0,
  P_BLOCKER, 0, 0, 0, 0,
  0, P_MIRROR_TR, 0, P_SPLIT_TLBR, P_TARGET_L,
  0, 0, 0, P_TARGET_T, 0,
  // Hand
  P_TARGET_L, P_TARGET_L, P_MIRROR_TL, P_MIRROR_TL, P_MIRROR_TL,

  // LEVEL 29
  // Puzzle
  P_TARGET_R, 0, 0, P_MIRROR_BL, 0,
  0, 0, P_TARGET_R, 0, 0,
  0, 0, 0, P_BLOCKER, 0,
  0, P_MIRROR_TR, 0, 0, 0,
  0, 0, 0, 0, 0,
  // Solution
  P_TARGET_R, 0, 0, P_MIRROR_BL, 0,
  0, P_MIRROR_BL, P_TARGET_R, P_SPLIT_TLBR, P_MIRROR_BL,
  0, 0, 0, P_BLOCKER, 0,
  0, P_MIRROR_TR, 0, 0, P_MIRROR_TL,
  0, 0, 0, 0, 0,
  // Hand
  P_SPLIT_TRBL, P_MIRROR_TL, P_MIRROR_TL, P_MIRROR_TL, 0,

  // LEVEL 30
  // Puzzle
  P_BLOCKER, 0, 0, 0, 0,
  0, 0, P_TARGET_R, 0, 0,
  0, P_BLOCKER, 0, 0, 0,
  0, 0, 0, 0, P_BLOCKER,
  0, 0, P_TARGET_R, 0, 0,
  // Solution
  P_BLOCKER, P_MIRROR_BR, 0, P_MIRROR_BL, 0,
  0, P_MIRROR_TL, P_TARGET_R, P_SPLIT_TRBL, 0,
  0, P_BLOCKER, 0, 0, 0,
  0, 0, 0, 0, P_BLOCKER,
  0, 0, P_TARGET_R, P_MIRROR_TL, 0,
  // Hand
  P_SPLIT_TRBL, P_MIRROR_TL, P_MIRROR_TL, P_MIRROR_TL, P_MIRROR_TL,

  // LEVEL 31
  // Puzzle
  P_MIRROR_BR, 0, 0, P_BLOCKER, 0,
  0, 0, P_BLOCKER, 0, 0,
  0, P_BLOCKER, 0, 0, 0,
  0, 0, 0, P_SPLIT_TLBR, 0,
  0, 0, 0, 0, 0,
  // Solution
  P_MIRROR_BR, P_MIRROR_BL, 0, P_BLOCKER, 0,
  0, P_MIRROR_TL, P_BLOCKER, 0, 0,
  0, P_BLOCKER, 0, 0, 0,
  P_MIRROR_TR, 0, 0, P_SPLIT_TLBR, P_TARGET_L,
  0, 0, 0, P_TARGET_T, 0,
  // Hand
  P_TARGET_L, P_TARGET_L, P_MIRROR_TL, P_MIRROR_TL, P_MIRROR_TL,

  // LEVEL 32
  // Puzzle
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  P_TARGET_B, P_TARGET_R, 0, P_BLOCKER, 0,
  0, 0, 0, P_BLOCKER, 0,
  0, P_BLOCKER, 0, 0, P_MIRROR_TL,
  // Solution
  0, 0, 0, 0, 0,
  0, 0, 0, 0, P_MIRROR_BL,
  P_TARGET_B, P_TARGET_R, P_MIRROR_BL, P_BLOCKER, 0,
  P_MIRROR_TR, 0, P_SPLIT_TLBR, P_BLOCKER, 0,
  0, P_BLOCKER, P_MIRROR_TR, 0, P_MIRROR_TL,
  // Hand
  P_SPLIT_TRBL, P_MIRROR_TL, P_MIRROR_TL, P_MIRROR_TL, P_MIRROR_TL,

  // LEVEL 33
  // Puzzle
  0, P_TARGET_B, P_TARGET_B, 0, 0,
  0, 0, 0, 0, P_MIRROR_BL,
  0, 0, 0, 0, 0,
  0, 0, P_SPLIT_TLBR, 0, P_BLOCKER,
  0, 0, 0, 0, 0,
  // Solution
  0, P_TARGET_B, P_TARGET_B, 0, 0,
  0, 0, 0, 0, P_MIRROR_BL,
  0, 0, 0, P_MIRROR_BR, P_MIRROR_TL,
  0, P_MIRROR_TR, P_SPLIT_TLBR, P_MIRROR_TL, P_BLOCKER,
  0, 0, 0, 0, 0,
  // Hand
  P_MIRROR_TL, P_MIRROR_TL, P_MIRROR_TL, P_MIRROR_TL, 0,

  // LEVEL 34
  // Puzzle
  0, 0, P_MIRROR_BL, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, P_TARGET_L,
  P_TARGET_R, 0, P_BLOCKER, 0, 0,
  // Solution
  0, P_MIRROR_BR, P_MIRROR_BL, 0, 0,
  0, 0, P_MIRROR_TL, 0, 0,
  0, 0, 0, 0, 0,
  0, P_SPLIT_TLBR, 0, 0, P_TARGET_L,
  P_TARGET_R, P_MIRROR_TL, P_BLOCKER, 0, 0,
  // Hand
  P_SPLIT_TRBL, P_MIRROR_TL, P_MIRROR_TL, P_MIRROR_TL, 0,

  // LEVEL 35
  // Puzzle
  0, P_TARGET_B, P_TARGET_B, 0, 0,
  0, 0, 0, 0, P_MIRROR_BL,
  0, 0, 0, 0, 0,
  0, 0, P_SPLIT_TRBL, 0, 0,
  0, 0, 0, 0, 0,
  // Solution
  0, P_TARGET_B, P_TARGET_B, 0, 0,
  P_MIRROR_BL, P_MIRROR_TR, 0, 0, P_MIRROR_BL,
  0, 0, 0, 0, 0,
  P_MIRROR_TR, 0, P_SPLIT_TRBL, 0, P_MIRROR_TL,
  0, 0, 0, 0, 0,
  // Hand
  P_MIRROR_TL, P_MIRROR_TL, P_MIRROR_TL, P_MIRROR_TL, 0,

  // LEVEL 36
  // Puzzle
  0, P_TARGET_R, 0, 0, 0,
  0, 0, 0, 0, P_MIRROR_BL,
  0, 0, 0, P_BLOCKER, 0,
  0, P_TARGET_R, 0, 0, P_MIRROR_TL,
  0, 0, P_MIRROR_TR, 0, 0,
  // Solution
  0, P_TARGET_R, P_MIRROR_BL, 0, 0,
  0, 0, 0, 0, P_MIRROR_BL,
  0, 0, 0, P_BLOCKER, 0,
  0, P_TARGET_R, 0, P_SPLIT_TRBL, P_MIRROR_TL,
  0, 0, P_MIRROR_TR, P_MIRROR_TL, 0,
  // Hand
  P_SPLIT_TRBL, P_MIRROR_TL, P_MIRROR_TL, 0, 0,

  // LEVEL 37
  // Puzzle
  0, P_TARGET_B, 0, 0, 0,
  0, 0, 0, 0, 0,
  P_TARGET_R, 0, 0, 0, 0,
  0, 0, 0, P_SPLIT_TLBR, 0,
  0, 0, 0, 0, 0,
  // Solution
  0, P_TARGET_B, 0, 0, 0,
  0, 0, 0, 0, P_MIRROR_BL,
  P_TARGET_R, 0, 0, P_MIRROR_BL, 0,
  0, P_MIRROR_TR, 0, P_SPLIT_TLBR, P_MIRROR_TL,
  0, 0, 0, 0, 0,
  // Hand
  P_MIRROR_TL, P_MIRROR_TL, P_MIRROR_TL, P_MIRROR_TL, 0,

  // LEVEL 38
  // Puzzle
  0, 0, 0, 0, 0,
  0, 0, 0, P_BLOCKER, 0,
  P_TARGET_B, P_TARGET_R, 0, 0, 0,
  0, 0, 0, P_BLOCKER, 0,
  0, P_BLOCKER, 0, 0, P_MIRROR_TL,
  // Solution
  0, 0, 0, 0, 0,
  0, 0, P_MIRROR_BL, P_BLOCKER, 0,
  P_TARGET_B, P_TARGET_R, 0, 0, P_MIRROR_BL,
  P_MIRROR_TR, 0, P_SPLIT_TRBL, P_BLOCKER, 0,
  0, P_BLOCKER, P_MIRROR_TR, 0, P_MIRROR_TL,
  // Hand
  P_SPLIT_TRBL, P_MIRROR_TL, P_MIRROR_TL, P_MIRROR_TL, P_MIRROR_TL,

  // LEVEL 39
  // Puzzle
  P_MIRROR_BR, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, P_TARGET_R, 0,
  0, 0, 0, 0, 0,
  0, 0, P_MIRROR_TL, 0, P_TARGET_T,
  // Solution
  P_MIRROR_BR, 0, 0, 0, P_MIRROR_BL,
  0, 0, P_MIRROR_BL, 0, 0,
  0, 0, 0, P_TARGET_R, P_SPLIT_TRBL,
  0, 0, 0, 0, 0,
  P_MIRROR_TR, 0, P_MIRROR_TL, 0, P_TARGET_T,
  // Hand
  P_SPLIT_TRBL, P_MIRROR_TL, P_MIRROR_TL, P_MIRROR_TL, 0,

  // LEVEL 40
  // Puzzle
  0, P_TARGET_R, 0, 0, 0,
  0, 0, 0, P_BLOCKER, 0,
  0, P_TARGET_R, 0, 0, 0,
  0, 0, P_MIRROR_TR, 0, 0,
  0, 0, 0, 0, 0,
  // Solution
  0, P_TARGET_R, 0, 0, P_MIRROR_BL,
  0, 0, P_MIRROR_BL, P_BLOCKER, 0,
  0, P_TARGET_R, 0, P_MIRROR_BL, 0,
  0, 0, P_MIRROR_TR, P_SPLIT_TRBL, P_MIRROR_TL,
  0, 0, 0, 0, 0,
  // Hand
  P_SPLIT_TRBL, P_MIRROR_TL, P_MIRROR_TL, P_MIRROR_TL, P_MIRROR_TL,
};

#define LEVEL_SIZE 55
#define LEVELS (sizeof(levelData) / LEVEL_SIZE)

const VRAM_PTR_TYPE* MapName(uint8_t piece)
{
  switch (piece) {
  case P_BLANK:
    return map_blank;
  case P_BLOCKER:
    return map_blocker;
  case P_TARGET_T:
    return map_target_t;
  case P_TARGET_R:
    return map_target_r;
  case P_TARGET_B:
    return map_target_b;
  case P_TARGET_L:
    return map_target_l;
  case P_MIRROR_BL:
    return map_mirror_bl;
  case P_MIRROR_TL:
    return map_mirror_tl;
  case P_MIRROR_TR:
    return map_mirror_tr;
  case P_MIRROR_BR:
    return map_mirror_br;
  case P_SPLIT_TLBR:
    return map_split_tlbr;
  case P_SPLIT_TRBL:
    return map_split_trbl;
  }
  return map_blank;
}

/*
 * BCD_addConstant
 *
 * Adds a constant (binary number) to a BCD number
 *
 * num [in, out]
 *   The BCD number
 *
 * digits [in]
 *   The number of digits in the BCD number, num
 *
 * x [in]
 *   The binary value to be added to the BCD number
 *
 *   Note: The largest value that can be safely added to a BCD number
 *         is BCD_ADD_CONSTANT_MAX. If the result would overflow num,
 *         then num will be clamped to its maximum value (all 9's).
 *
 * Returns:
 *   A boolean that is true if num has been clamped to its maximum
 *   value (all 9's), or false otherwise.
 */
#define BCD_ADD_CONSTANT_MAX 244
static bool BCD_addConstant(uint8_t* const num, const uint8_t digits, uint8_t x)
{
  for (uint8_t i = 0; i < digits; ++i) {
    uint8_t val = num[i] + x;
    if (val < 10) { // speed up the common cases
      num[i] = val;
      x = 0;
      break;
    } else if (val < 20) {
      num[i] = val - 10;
      x = 1;
    } else if (val < 30) {
      num[i] = val - 20;
      x = 2;
    } else if (val < 40) {
      num[i] = val - 30;
      x = 3;
    } else { // handle the rest of the cases (up to 255 - 9) with a loop
      for (uint8_t j = 5; j < 26; ++j) {
        if (val < (j * 10)) {
          num[i] = val - ((j - 1) * 10);
          x = (j - 1);
          break;
        }
      }
    }
  }

  if (x > 0) {
    for (uint8_t i = 0; i < digits; ++i)
      num[i] = 9;
    return true;
  }

  return false;
}

#define PREV_NEXT_X 2
#define PREV_NEXT_Y 25
#define FIRST_DIGIT_SPRITE 2
// The highest sprite index is for the "mouse cursor"
// and the 9 highest below that are reserved for drag-and-drop
#define RESERVED_SPRITES 10

static void LoadLevel(const uint8_t level, bool solution)
{
  for (uint8_t i = 0; i < MAX_SPRITES - 1; ++i)
    sprites[i].x = OFF_SCREEN;
  
  for (uint8_t v = 0; v < VRAM_TILES_V; ++v)
    for (uint8_t h = 0; h < VRAM_TILES_H; ++h)
      SetTile(h, v, TILE_BACKGROUND);

  DrawMap(0, 1, map_graphic);
  DrawMap(9, 22, map_move_to_grid);

  DrawMap(PREV_NEXT_X, PREV_NEXT_Y, map_prev_next);
  
  uint8_t levelDisplay[2] = {0};
  BCD_addConstant(levelDisplay, 2, level);
  // Since we ran out of unique background tile indices, we have to resort to using sprites
  sprites[0].tileIndex = levelDisplay[0] + FIRST_DIGIT_SPRITE;
  sprites[1].tileIndex = levelDisplay[1] + FIRST_DIGIT_SPRITE;
  
  sprites[0].y = 23 * TILE_HEIGHT + TILE_HEIGHT / 2; // This uses an extra RAM tile
  sprites[1].y = 23 * TILE_HEIGHT + TILE_HEIGHT / 2; // but makes it look better
  sprites[0].x = (PREV_NEXT_X + 2) * TILE_WIDTH;
  sprites[1].x = (PREV_NEXT_X + 1) * TILE_WIDTH;
	
  const uint16_t levelOffset = (level - 1) * LEVEL_SIZE;
  uint8_t currentSprite = 3;
  
  for (uint8_t y = 0; y < 5; ++y)
    for (uint8_t x = 0; x < 5; ++x) {
      uint8_t piece = (uint8_t)pgm_read_byte(&levelData[(levelOffset + (solution ? 25 : 0)) + y * 5 + x]);
      board[y][x] = piece | 0x80; // set the high bit, to denote a piece that cannot be moved
      DrawMap(9 + x * 4, 1 + y * 4, MapName(piece));
      // Any pieces that are part of the inital setup can't be moved, so add a lock icon
      if ((piece != P_BLANK) && (currentSprite < (MAX_SPRITES - RESERVED_SPRITES))
	  && (solution == false)) {
      	sprites[currentSprite].tileIndex = 0;
      	sprites[currentSprite].x = (11 + x * 4) * TILE_WIDTH;
      	sprites[currentSprite].y = (3 + y * 4) * TILE_HEIGHT;
	++currentSprite;
      }
    }
  
  if (solution) {
    for (uint8_t x = 0; x < 5; ++x)
      DrawMap(9 + x * 4, 23, map_blank);
    return;
  }
    
  for (uint8_t x = 0; x < 5; ++x) {
    uint8_t piece = (uint8_t)pgm_read_byte(&levelData[(levelOffset + 50) + x]);
    hand[x] = piece;
    DrawMap(9 + x * 4, 23, MapName(piece));
  }
}

void SimulatePhoton(void)
{
  int8_t laser_x = 0;
  int8_t laser_y = 1;
  uint8_t laser_d = D_IN_L;

  bool bounce = false;
  bool halt = false;
  do {
    // Examine the piece under the current position (laser_x, laser_y)
    switch (board[laser_y][laser_x] & 0x0F) { // ignore the high word
    case P_BLANK:
      switch (laser_d) {
      case D_IN_T:
	laser[laser_y][laser_x] |= (D_IN_T | D_OUT_B);
	laser_y++;
	break;
      case D_IN_B:
	laser[laser_y][laser_x] |= (D_IN_B | D_OUT_T);
	laser_y--;
	break;
      case D_IN_L:
	laser[laser_y][laser_x] |= (D_IN_L | D_OUT_R);
	laser_x++;
	break;
      case D_IN_R:
	laser[laser_y][laser_x] |= (D_IN_R | D_OUT_L);
	laser_x--;
	break;
      }
      break;

    case P_BLOCKER:
      halt = true;
      break;

    case P_TARGET_T:
      if (laser_d == D_IN_T)
	laser[laser_y][laser_x] |= laser_d;
      halt = true;
      break;

    case P_TARGET_R:
      if (laser_d == D_IN_R)
	laser[laser_y][laser_x] |= laser_d;
      halt = true;
      break;

    case P_TARGET_B:
      if (laser_d == D_IN_B)
	laser[laser_y][laser_x] |= laser_d;
      halt = true;
      break;

    case P_TARGET_L:
      if (laser_d == D_IN_L)
	laser[laser_y][laser_x] |= laser_d;
      halt = true;
      break;

    case P_MIRROR_BL:
      switch (laser_d) {
      case D_IN_T:
	halt = true;
	break;
      case D_IN_B:
	laser[laser_y][laser_x] |= (D_IN_B | D_OUT_L);
	laser_d = D_IN_R;
	laser_x--;
	break;
      case D_IN_L:
	laser[laser_y][laser_x] |= (D_IN_L | D_OUT_B);
	laser_d = D_IN_T;
	laser_y++;
	break;
      case D_IN_R:
	halt = true;
	break;
      }
      break;

    case P_MIRROR_TL:
      switch (laser_d) {
      case D_IN_T:
	laser[laser_y][laser_x] |= (D_IN_T | D_OUT_L);
	laser_d = D_IN_R;
	laser_x--;
	break;
      case D_IN_B:
	halt = true;
	break;
      case D_IN_L:
	laser[laser_y][laser_x] |= (D_IN_L | D_OUT_T);
	laser_d = D_IN_B;
	laser_y--;
	break;
      case D_IN_R:
	halt = true;
	break;
      }
      break;

    case P_MIRROR_TR:
      switch (laser_d) {
      case D_IN_T:
	laser[laser_y][laser_x] |= (D_IN_T | D_OUT_R);
	laser_d = D_IN_L;
	laser_x++;
	break;
      case D_IN_B:
	halt = true;
	break;
      case D_IN_L:
	halt = true;
	break;
      case D_IN_R:
	laser[laser_y][laser_x] |= (D_IN_R | D_OUT_T);
	laser_d = D_IN_B;
	laser_y--;
	break;
      }
      break;

    case P_MIRROR_BR:
      switch (laser_d) {
      case D_IN_T:
	halt = true;
	break;
      case D_IN_B:
	laser[laser_y][laser_x] |= (D_IN_B | D_OUT_R);
	laser_d = D_IN_L;
	laser_x++;
	break;
      case D_IN_L:
	halt = true;
	break;
      case D_IN_R:
	laser[laser_y][laser_x] |= (D_IN_R | D_OUT_B);
	laser_d = D_IN_T;
	laser_y++;
	break;
      }
      break;

    case P_SPLIT_TLBR:
      // Generate a random number, and decide whether the beam passes through, or bounces
      bounce = (rand() > (RAND_MAX / 2));
      switch (laser_d) {
      case D_IN_T:
	if (bounce) {
	  laser[laser_y][laser_x] |= (D_IN_T | D_OUT_R);
	  laser_d = D_IN_L;
	  laser_x++;
	} else {
	  laser[laser_y][laser_x] |= (D_IN_T | D_OUT_B);
	  laser_y++;
	}
	break;
      case D_IN_B:
	if (bounce) {
	  laser[laser_y][laser_x] |= (D_IN_B | D_OUT_L);
	  laser_d = D_IN_R;
	  laser_x--;
	} else {
	  laser[laser_y][laser_x] |= (D_IN_B | D_OUT_T);
	  laser_y--;
	}
	break;
      case D_IN_L:
	if (bounce) {
	  laser[laser_y][laser_x] |= (D_IN_L | D_OUT_B);
	  laser_d = D_IN_T;
	  laser_y++;
	} else {
	  laser[laser_y][laser_x] |= (D_IN_L | D_OUT_R);
	  laser_x++;
	}
	break;
      case D_IN_R:
	if (bounce) {
	  laser[laser_y][laser_x] |= (D_IN_R | D_OUT_T);
	  laser_d = D_IN_B;
	  laser_y--;
	} else {
	  laser[laser_y][laser_x] |= (D_IN_R | D_OUT_L);
	  laser_x--;
	}
	break;
      }
      break;
      
    case P_SPLIT_TRBL:
      // Generate a random number, and decide whether the beam passes through, or bounces
      bounce = (rand() > (RAND_MAX / 2));
      switch (laser_d) {
      case D_IN_T:
	if (bounce) {
	  laser[laser_y][laser_x] |= (D_IN_T | D_OUT_L);
	  laser_d = D_IN_R;
	  laser_x--;
	} else {
	  laser[laser_y][laser_x] |= (D_IN_T | D_OUT_B);
	  laser_y++;
	}
	break;
      case D_IN_B:
	if (bounce) {
	  laser[laser_y][laser_x] |= (D_IN_B | D_OUT_R);
	  laser_d = D_IN_L;
	  laser_x++;
	} else {
	  laser[laser_y][laser_x] |= (D_IN_B | D_OUT_T);
	  laser_y--;
	}
	break;
      case D_IN_L:
	if (bounce) {
	  laser[laser_y][laser_x] |= (D_IN_L | D_OUT_T);
	  laser_d = D_IN_B;
	  laser_y--;
	} else {
	  laser[laser_y][laser_x] |= (D_IN_L | D_OUT_R);
	  laser_x++;
	}
	break;
      case D_IN_R:
	if (bounce) {
	  laser[laser_y][laser_x] |= (D_IN_R | D_OUT_B);
	  laser_d = D_IN_T;
	  laser_y++;
	} else {
	  laser[laser_y][laser_x] |= (D_IN_R | D_OUT_L);
	  laser_x--;
	}
	break;
      }
      break;
      
    }
  } while (!halt && laser_x >= 0 && laser_x <= 4 &&
	   laser_y >= 0 && laser_y <= 4);

}

void DrawLaser(void)
{
  DrawMap(7, 5, map_laser_source);
  for (uint8_t y = 0; y < 5; ++y) {
    for (uint8_t x = 0; x < 5; ++x) {
      uint8_t l = laser[y][x];
      switch (board[y][x] & 0x0F) { // ignore the high word
      case P_BLANK:
	{
	  bool h = ((l & D_IN_L) && (l & D_OUT_R)) || ((l & D_IN_R) && (l & D_OUT_L));
	  bool v = ((l & D_IN_T) && (l & D_OUT_B)) || ((l & D_IN_B) && (l & D_OUT_T));
	  if (h && v)
	    DrawMap(9 + x * 4, 1 + y * 4, map_blank_on_hv);
	  else if (h)
	    DrawMap(9 + x * 4, 1 + y * 4, map_blank_on_h);
	  else if (v)
	    DrawMap(9 + x * 4, 1 + y * 4, map_blank_on_v);
	}
	break;

      case P_TARGET_T:
	if (l & D_IN_T)
	  DrawMap(9 + x * 4, 1 + y * 4, map_target_t_on);
	break;

      case P_TARGET_R:
	if (l & D_IN_R)
	  DrawMap(9 + x * 4, 1 + y * 4, map_target_r_on);
	break;

      case P_TARGET_B:
	if (l & D_IN_B)
	  DrawMap(9 + x * 4, 1 + y * 4, map_target_b_on);
	break;

      case P_TARGET_L:
	if (l & D_IN_L)
	  DrawMap(9 + x * 4, 1 + y * 4, map_target_l_on);
	break;

      case P_MIRROR_BL:
	if (((l & D_IN_B) && (l & D_OUT_L)) ||
	    ((l & D_IN_L) && (l & D_OUT_B)))
	  DrawMap(9 + x * 4, 1 + y * 4, map_mirror_bl_on);
	break;

      case P_MIRROR_TL:
	if (((l & D_IN_T) && (l & D_OUT_L)) ||
	    ((l & D_IN_L) && (l & D_OUT_T)))
	  DrawMap(9 + x * 4, 1 + y * 4, map_mirror_tl_on);
	break;

      case P_MIRROR_TR:
	if (((l & D_IN_T) && (l & D_OUT_R)) ||
	    ((l & D_IN_R) && (l & D_OUT_T)))
	  DrawMap(9 + x * 4, 1 + y * 4, map_mirror_tr_on);
	break;

      case P_MIRROR_BR:
	if (((l & D_IN_B) && (l & D_OUT_R)) ||
	    ((l & D_IN_R) && (l & D_OUT_B)))
	  DrawMap(9 + x * 4, 1 + y * 4, map_mirror_br_on);
	break;

      case P_SPLIT_TLBR:
	{
	  bool in_l = (l & D_IN_L) && (l & D_OUT_R) && (l & D_OUT_B);
	  bool in_t = (l & D_IN_T) && (l & D_OUT_B) && (l & D_OUT_R);
	  bool in_r = (l & D_IN_R) && (l & D_OUT_L) && (l & D_OUT_T);
	  bool in_b = (l & D_IN_B) && (l & D_OUT_T) && (l & D_OUT_L);
	  if (in_l && !in_t && !in_r && !in_b)
	    DrawMap(9 + x * 4, 1 + y * 4, map_split_tlbr_on_l);
	  else if (in_t && !in_l && !in_r && !in_b)
	    DrawMap(9 + x * 4, 1 + y * 4, map_split_tlbr_on_t);
	  else if (in_r && !in_l && !in_t && !in_b)
	    DrawMap(9 + x * 4, 1 + y * 4, map_split_tlbr_on_r);
	  else if (in_b && !in_l && !in_t && !in_r)
	    DrawMap(9 + x * 4, 1 + y * 4, map_split_tlbr_on_b);
	  else if (in_l || in_t || in_r || in_b)
	    DrawMap(9 + x * 4, 1 + y * 4, map_split_tlbr_on_a);
	}
	break;

      case P_SPLIT_TRBL:
	{
	  bool in_l = (l & D_IN_L) && (l & D_OUT_R) && (l & D_OUT_T);
	  bool in_t = (l & D_IN_T) && (l & D_OUT_B) && (l & D_OUT_L);
	  bool in_r = (l & D_IN_R) && (l & D_OUT_L) && (l & D_OUT_B);
	  bool in_b = (l & D_IN_B) && (l & D_OUT_T) && (l & D_OUT_R);
	  if (in_l && !in_t && !in_r && !in_b)
	    DrawMap(9 + x * 4, 1 + y * 4, map_split_trbl_on_l);
	  else if (in_t && !in_l && !in_r && !in_b)
	    DrawMap(9 + x * 4, 1 + y * 4, map_split_trbl_on_t);
	  else if (in_r && !in_l && !in_t && !in_b)
	    DrawMap(9 + x * 4, 1 + y * 4, map_split_trbl_on_r);
	  else if (in_b && !in_l && !in_t && !in_r)
	    DrawMap(9 + x * 4, 1 + y * 4, map_split_trbl_on_b);
	  else if (in_l || in_t || in_r || in_b)
	    DrawMap(9 + x * 4, 1 + y * 4, map_split_trbl_on_a);
	}
	break;
      }
    }
  }
  // Fill in the gaps between squares with lasers
  for (uint8_t y = 0; y < 5; ++y)
    for (uint8_t x = 0; x < 4; ++x)
      if ((laser[y][x] & D_OUT_R) || (laser[y][x + 1] & D_OUT_L))
	DrawMap(12 + x * 4, 2 + y * 4, map_gap_h);
  for (uint8_t y = 0; y < 4; ++y)
    for (uint8_t x = 0; x < 5; ++x)
      if ((laser[y][x] & D_OUT_B) || (laser[y + 1][x] & D_OUT_T))
	DrawMap(10 + x * 4, 4 + y * 4, map_gap_v);
}

void EraseLaser(void)
{
  for (uint8_t y = 0; y < 5; ++y)
    for (uint8_t x = 0; x < 5; ++x)
      DrawMap(9 + x * 4, 1 + y * 4, MapName(board[y][x] & 0x0F));
      
  // Erase any lasers between squares
  for (uint8_t y = 0; y < 5; ++y)
    for (uint8_t x = 0; x < 4; ++x)
      if ((laser[y][x] & D_OUT_R) || (laser[y][x + 1] & D_OUT_L))
	SetTile(12 + x * 4, 2 + y * 4, TILE_BACKGROUND);
  for (uint8_t y = 0; y < 4; ++y)
    for (uint8_t x = 0; x < 5; ++x)
      if ((laser[y][x] & D_OUT_B) || (laser[y + 1][x] & D_OUT_T))
	SetTile(10 + x * 4, 4 + y * 4, TILE_BACKGROUND);
  DrawMap(7, 5, map_laser_source_off);
}

const int8_t hitMap[] PROGMEM = {
  0, 0, 0, -1,
  1, 1, 1, -1,
  2, 2, 2, -1,
  3, 3, 3, -1,
  4, 4, 4,
};

const uint8_t rotateClockwise[] PROGMEM = {
  P_BLANK, 
  P_BLOCKER, 
  P_TARGET_R,
  P_TARGET_B,
  P_TARGET_L,
  P_TARGET_T,
  P_MIRROR_TL,
  P_MIRROR_TR,
  P_MIRROR_BR,
  P_MIRROR_BL,
  P_SPLIT_TRBL,
  P_SPLIT_TLBR,
};

const uint8_t rotateCounterClockwise[] PROGMEM = {
  P_BLANK,
  P_BLOCKER,
  P_TARGET_L,
  P_TARGET_T,
  P_TARGET_R,
  P_TARGET_B,
  P_MIRROR_BR,
  P_MIRROR_BL,
  P_MIRROR_TL,
  P_MIRROR_TR,
  P_SPLIT_TRBL,
  P_SPLIT_TLBR,
};

int8_t old_piece = -1;
int8_t old_x = -1;
int8_t old_y = -1; // if this is 5, then it refers to hand

void TryRotation(const uint8_t* rotation_lut)
{
  if (old_piece == -1) { // nothing being dragged and dropped
    uint8_t tx = sprites[MAX_SPRITES - 1].x / TILE_WIDTH;
    uint8_t ty = sprites[MAX_SPRITES - 1].y / TILE_HEIGHT;
    if ((ty >= 1) && (ty <= 19) && (tx >= 9) && (tx <= 27)) { // from grid
      int8_t x = pgm_read_byte(&hitMap[tx - 9]);
      int8_t y = pgm_read_byte(&hitMap[ty - 1]);
      if ((x >= 0) && (y >= 0) && !(board[y][x] & 0x80)) { // respect lock bit
	board[y][x] = pgm_read_byte(&rotation_lut[board[y][x]]);
	DrawMap(9 + x * 4, 1 + y * 4, MapName(board[y][x]));
	TriggerNote(4, 3, 23, 255);
      }
    } else if ((ty >= 23) && (ty <= 25) && (tx >= 9) && (tx <= 27)) {
      int8_t x = pgm_read_byte(&hitMap[tx - 9]);
      if (x >= 0) {
	hand[x] = pgm_read_byte(&rotation_lut[hand[x]]);
	DrawMap(9 + x * 4, 23, MapName(hand[x]));
	TriggerNote(4, 3, 23, 255);
      }
    }
  } else {
    old_piece = pgm_read_byte(&rotation_lut[old_piece]);
    MapSprite2(MAX_SPRITES - 10, MapName(old_piece), SPRITE_BANK1);
    MoveSprite(MAX_SPRITES - 10, sprites[MAX_SPRITES - 1].x - 8, sprites[MAX_SPRITES - 1].y - 8, 3, 3);
    TriggerNote(4, 3, 23, 255);
  }
}

int main()
{
  BUTTON_INFO buttons;
  memset(&buttons, 0, sizeof(BUTTON_INFO));

  SetTileTable(instructions);
  DrawMap(0, 0, map_instructions);

  for (;;) {
    WaitVsync(1);
 
    // Read the current state of the player's controller
    buttons.prev = buttons.held;
    buttons.held = ReadJoypad(0);
    buttons.pressed = buttons.held & (buttons.held ^ buttons.prev);
    buttons.released = buttons.prev & (buttons.held ^ buttons.prev);

    if ((buttons.pressed & BTN_A) || (buttons.pressed & BTN_START))
      break;
  }
  
  memset(&buttons, 0, sizeof(BUTTON_INFO));

  SetTileTable(tileset);
  SetSpritesTileBank(0, mysprites);
  SetSpritesTileBank(1, tileset);
  InitMusicPlayer(patches);

  StartSong(midisong);

  uint8_t currentLevel = 1;
  LoadLevel(currentLevel, false);
  
  sprites[MAX_SPRITES - 1].tileIndex = 1;
  sprites[MAX_SPRITES - 1].x = 7 * TILE_WIDTH;
  sprites[MAX_SPRITES - 1].y = 24 * TILE_HEIGHT;
  uint8_t saved_cursor_x = 0;

  bool flashNext = false;
  uint8_t flashCounter = 0;
  
  for (;;) {
    WaitVsync(1);
 
    // Read the current state of the player's controller
    buttons.prev = buttons.held;
    buttons.held = ReadJoypad(0);
    buttons.pressed = buttons.held & (buttons.held ^ buttons.prev);
    buttons.released = buttons.prev & (buttons.held ^ buttons.prev);

    // This "solution view" is for debug purposes only!
    /* if (buttons.pressed & BTN_START) */
    /*   LoadLevel(currentLevel, true); */
    /* if (buttons.released & BTN_START) */
    /*   LoadLevel(currentLevel, false); */

    if (flashNext) {
      if (flashCounter == 0)
	DrawMap(PREV_NEXT_X + 2, PREV_NEXT_Y, map_next_red);
      else if (flashCounter == 19)
	DrawMap(PREV_NEXT_X, PREV_NEXT_Y, map_prev_next);
      else if (flashCounter == 39)
	flashCounter = 255;
      ++flashCounter;
    }
    
    if (buttons.pressed & BTN_Y) {
      // Hide the cursor when the laser is on
      saved_cursor_x = sprites[MAX_SPRITES - 1].x;

      if (!(buttons.held & BTN_A)) { // Don't turn the laser on if you are dragging and dropping
	sprites[MAX_SPRITES - 1].x = OFF_SCREEN;
	memset(laser, 0, sizeof(laser));
	for (uint8_t i = 0; i < 100; ++i)
	  SimulatePhoton();
      
	DrawLaser();
	// Check to see if the puzzle has been solved
	const uint16_t offset = (currentLevel - 1) * LEVEL_SIZE + 25;
	bool win = true;
	for (uint8_t y = 0; y < 5; ++y)
	  for (uint8_t x = 0; x < 5; ++x) {
	    uint8_t piece = (uint8_t)pgm_read_byte(&levelData[offset + y * 5 + x]);
	    if ((board[y][x] & 0x0F) != piece)
	      win = false;
	  }
	if (win) {
	  TriggerNote(4, 5, 15, 255);
	  flashNext = true;
	  sprites[2].tileIndex = 12;
	  sprites[2].flags = 0;
	  sprites[2].x = 4 * TILE_WIDTH;
	  sprites[2].y = (2 * TILE_WIDTH) + 4;
	  WaitVsync(180);
	} else {
	  sprites[2].tileIndex = 12;
	  sprites[2].flags = SPRITE_FLIP_Y;
	  sprites[2].x = 4 * TILE_WIDTH;
	  sprites[2].y = (2 * TILE_WIDTH) + 4;	  
	}
      }
    } else if (buttons.released & BTN_Y) {
      EraseLaser();
      sprites[2].x = OFF_SCREEN;
      // Restore the cursor when the laser is off
      sprites[MAX_SPRITES - 1].x = saved_cursor_x;
    }
        
#define CUR_SPEED 2
#define X_LB (1 * TILE_WIDTH)
#define X_UB ((SCREEN_TILES_H - 2) * TILE_WIDTH)
#define Y_LB (1 * TILE_HEIGHT)
#define Y_UB ((SCREEN_TILES_V - 2) * TILE_HEIGHT)
    
    if (!(buttons.held & BTN_Y)) { // Don't allow the hidden cursor to be moved if the laser is on
    
      // Move the "mouse cursor"
      if (buttons.held & BTN_RIGHT) {
	uint8_t x = sprites[MAX_SPRITES - 1].x;
	x += CUR_SPEED;
	if (x > X_UB)
	  x = X_UB;
	sprites[MAX_SPRITES - 1].x = x;
      } else if (buttons.held & BTN_LEFT) {
	uint8_t x = sprites[MAX_SPRITES - 1].x;
	x -= CUR_SPEED;
	if (x < X_LB)
	  x = X_LB;
	sprites[MAX_SPRITES - 1].x = x;
      }
      if (buttons.held & BTN_UP) {
	uint8_t y = sprites[MAX_SPRITES - 1].y;
	y -= CUR_SPEED;
	if (y < Y_LB)
	  y = Y_LB;
	sprites[MAX_SPRITES - 1].y = y;
      } else if (buttons.held & BTN_DOWN) {
	uint8_t y = sprites[MAX_SPRITES - 1].y;
	y += CUR_SPEED;
	if (y > Y_UB)
	  y = Y_UB;
	sprites[MAX_SPRITES - 1].y = y;
      }
      // Dragging
      if (old_piece != -1) {
	MoveSprite(MAX_SPRITES - 10, sprites[MAX_SPRITES - 1].x - 8, sprites[MAX_SPRITES - 1].y - 8, 3, 3);
      }
    }

    // Process rotations
    if (!(buttons.held & BTN_Y)) { // Don't process rotations if the laser is on
      if ((buttons.pressed & BTN_X) || (buttons.pressed & BTN_SR))
	TryRotation(rotateClockwise);
      else if ((buttons.pressed & BTN_B) || (buttons.pressed & BTN_SL))
	TryRotation(rotateCounterClockwise);
    }
    
    // Process any "mouse" clicks
    if (buttons.pressed & BTN_A) {
      uint8_t tx = sprites[MAX_SPRITES - 1].x / TILE_WIDTH;
      uint8_t ty = sprites[MAX_SPRITES - 1].y / TILE_HEIGHT;
      if ((ty == PREV_NEXT_Y) || (ty == PREV_NEXT_Y + 1)) {
	if ((tx >= PREV_NEXT_X) && (tx <= PREV_NEXT_X + 1)) {
	  if (--currentLevel == 0)
	    currentLevel = LEVELS;
	  TriggerNote(4, 3, 23, 255);
	  flashNext = false;
	  flashCounter = 0;
	  LoadLevel(currentLevel, false);
	}
	if ((tx >= PREV_NEXT_X + 2) && (tx <= PREV_NEXT_X + 3)) {
	  if (++currentLevel == LEVELS + 1)
	    currentLevel = 1;
	  TriggerNote(4, 3, 23, 255);
	  flashNext = false;
	  flashCounter = 0;
	  LoadLevel(currentLevel, false);
	}	
      }

      // Drag and drop
      if ((ty >= 1) && (ty <= 19) && (tx >= 9) && (tx <= 27)) { // from grid
	int8_t x = pgm_read_byte(&hitMap[tx - 9]);
	int8_t y = pgm_read_byte(&hitMap[ty - 1]);
	if ((x >= 0) && (y >= 0) && !(board[y][x] & 0x80) && (board[y][x] != P_BLANK)) { // respect lock bit
	  old_piece = board[y][x];
	  old_x = x;
	  old_y = y;
	  DrawMap(9 + x * 4, 1 + y * 4, map_blank);
	  board[y][x] = P_BLANK;
	  MapSprite2(MAX_SPRITES - 10, MapName(old_piece), SPRITE_BANK1);
	  MoveSprite(MAX_SPRITES - 10, sprites[MAX_SPRITES - 1].x - 8, sprites[MAX_SPRITES - 1].y - 8, 3, 3);
	  TriggerNote(4, 3, 23, 255);
	}
      } else if ((ty >= 23) && (ty <= 25) && (tx >= 9) && (tx <= 27)) { // from hand
	int8_t x = pgm_read_byte(&hitMap[tx - 9]);
	if ((x >= 0) && (hand[x] != P_BLANK)) {
	  old_piece = hand[x];
	  old_x = x;
	  old_y = 5; // this piece came from hand
	  DrawMap(9 + x * 4, 23, map_blank);
	  hand[x] = P_BLANK;
	  MapSprite2(MAX_SPRITES - 10, MapName(old_piece), SPRITE_BANK1);
	  MoveSprite(MAX_SPRITES - 10, sprites[MAX_SPRITES - 1].x - 8, sprites[MAX_SPRITES - 1].y - 8, 3, 3);
	  TriggerNote(4, 3, 23, 255);
	}
      }
      
    } else if (buttons.released & BTN_A) {
      if ((old_piece != -1) && (old_y != -1)) { // valid piece is being held
	uint8_t tx = sprites[MAX_SPRITES - 1].x / TILE_WIDTH;
	uint8_t ty = sprites[MAX_SPRITES - 1].y / TILE_HEIGHT;
	// Figure out where to drop it
	if ((ty >= 1) && (ty <= 19) && (tx >= 9) && (tx <= 27)) { // to grid
	  int8_t x = pgm_read_byte(&hitMap[tx - 9]);
	  int8_t y = pgm_read_byte(&hitMap[ty - 1]);
	  if ((x >= 0) && (y >= 0) && ((board[y][x] & 0x0F) == P_BLANK)) {
	    old_x = x;
	    old_y = y;
	  }
	} else if ((ty >= 23) && (ty <= 25) && (tx >= 9) && (tx <= 27)) { // to hand
	  int8_t x = pgm_read_byte(&hitMap[tx - 9]);
	  if ((x >= 0) && ((hand[x] & 0x0F) == P_BLANK)) {
	    old_x = x;
	    old_y = 5; // hand
	  }
	}
	
	// Drop it like it's hot
	for (uint8_t i = 0; i < 9; ++i)
	  sprites[i + MAX_SPRITES - 10].x = OFF_SCREEN;
	if (old_y == 5) {
	  DrawMap(9 + old_x * 4, 23, MapName(old_piece));
	  hand[old_x] = old_piece;
	} else {
	  DrawMap(9 + old_x * 4, 1 + old_y * 4, MapName(old_piece));
	  board[old_y][old_x] = old_piece;
	}
	old_piece = old_x = old_y = -1;
	TriggerNote(4, 4, 23, 255);
      }
    }
    
  }
}
