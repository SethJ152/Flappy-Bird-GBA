// (C) Seth Jones 2026
// Flappy Bird prototype with bird offsets and three tubes
#include <tonc.h>
#include <tonc_input.h>
#include <string.h>
#include <stdlib.h>       // for rand()
#include "Background.h"   // background image (256x256)
#include "tube.h"         // bottom tube (32x64)
#include "tube_top.h"     // top tube (32x64)
#include "bird.h"         // bird/player (32x32)

// --- Sprite OAM buffer ---
OBJ_ATTR obj_buffer[128];

// --- Game Constants ---
#define NUM_TUBES 3
#define TUBE_GAP 60       // vertical gap between top/bottom tube
#define TUBE_SPACING 120  // horizontal spacing between tubes
#define TUBE_SPEED 1
#define BIRD_GRAVITY 1
#define BIRD_JUMP -3
#define BIRD_TERMINAL 2

int menu(void) {
    // Placeholder for menu implementation
    for(int i = 0; i < 128; i++) {
        obj_buffer[i].attr0 = ATTR0_HIDE;
    }
    oam_copy(oam_mem, obj_buffer, 128);
    while (true) {
        vid_vsync();
        key_poll();
        if (key_is_down(KEY_START)) {
            break; // Exit menu on START button press
        }
    }
    return 0;
}

int main(void) {
    // Set display mode: Mode 0, sprites enabled, background layer 0
    REG_DISPCNT = DCNT_MODE0 | DCNT_OBJ | DCNT_OBJ_1D | DCNT_BG0;

    // Load background colors, tiles, and map
    memcpy(pal_bg_mem, BackgroundPal, BackgroundPalLen);
    memcpy(&tile_mem[0][0], BackgroundTiles, BackgroundTilesLen);
    memcpy(&se_mem[31][0], BackgroundMap, BackgroundMapLen);
    REG_BG0CNT = BG_CBB(0) | BG_SBB(31) | BG_4BPP | BG_REG_32x32;

    // Load tube palette and tiles
    memcpy(&pal_obj_mem[0], tubePal, 16 * 2);
    memcpy(&tile_mem[4][0], tubeTiles, sizeof(tubeTiles));
    memcpy(&tile_mem[4][sizeof(tubeTiles)/32], tube_topTiles, sizeof(tube_topTiles));

    // Load bird palette and tiles
    memcpy(&pal_obj_mem[16], birdPal, 16 * 2);
    memcpy(&tile_mem[4][64], birdTiles, sizeof(birdTiles));

    // Initialize tube positions
    int tubeX[NUM_TUBES];
    int tubeY[NUM_TUBES];
    int tubeTopY[NUM_TUBES];

    int score = 0;
    
    for(int i = 0; i < NUM_TUBES; i++) {
        tubeX[i] = 240 + i * TUBE_SPACING;
        tubeY[i] = 80 + (rand() % 40);
        tubeTopY[i] = tubeY[i] - TUBE_GAP - 64;
    }

    // Initialize bird
    int birdX = 80, birdY = 60;
    int birdVy = 0;

    // Setup tube sprites (3 pairs)
    for(int i = 0; i < NUM_TUBES; i++) {
        // Bottom tube
        obj_set_attr(&obj_buffer[i*2],
            ATTR0_TALL,
            ATTR1_SIZE_64,
            ATTR2_PALBANK(0) | 0);
        obj_set_pos(&obj_buffer[i*2], tubeX[i], tubeY[i]);

        // Top tube
        obj_set_attr(&obj_buffer[i*2+1],
            ATTR0_TALL,
            ATTR1_SIZE_64,
            ATTR2_PALBANK(0) | (sizeof(tubeTiles)/32));
        obj_set_pos(&obj_buffer[i*2+1], tubeX[i], tubeTopY[i]);
    }

    // Setup bird sprite
    obj_set_attr(&obj_buffer[NUM_TUBES*2],
        ATTR0_SQUARE,
        ATTR1_SIZE_32,
        ATTR2_PALBANK(1) | 64);
    obj_set_pos(&obj_buffer[NUM_TUBES*2], birdX, birdY);

    // Push sprites to hardware
    oam_copy(oam_mem, obj_buffer, 128);

    // Main loop
    while(1) {
        vid_vsync();
        key_poll();

        // Move tubes left and reset when off-screen
        for(int i = 0; i < NUM_TUBES; i++) {
            tubeX[i] -= TUBE_SPEED;

            if(tubeX[i] < -32) {
                tubeX[i] = 240;
                tubeY[i] = 80 + (rand() % 40);
                tubeTopY[i] = tubeY[i] - TUBE_GAP - 64;
            }

            obj_set_pos(&obj_buffer[i*2], tubeX[i], tubeY[i]);
            obj_set_pos(&obj_buffer[i*2+1], tubeX[i], tubeTopY[i]);
        }

        // Handle bird physics and jump
        if(key_is_down(KEY_A) || key_is_down(KEY_UP))
            birdVy = BIRD_JUMP;
        else {
            birdVy += BIRD_GRAVITY;
            if(birdVy > BIRD_TERMINAL)
                birdVy = BIRD_TERMINAL;
        }
        birdY += birdVy;

        if (key_is_down(KEY_START)) {
            menu();
        }

        // Clamp bird to screen
        if(birdY < 0)
            birdY = 0;
        if(birdY > 160 - 32)
            birdY = 160 - 32;

        // Collision detection
        int birdLeft = birdX + 5;
        int birdRight = birdX + 32 - 8;
        int birdTop = birdY + 6;
        int birdBottom = birdY + 32 - 14;

        for(int i = 0; i < NUM_TUBES; i++) {
            int tubeLeft = tubeX[i];
            int tubeRight = tubeX[i] + 32;
            int tubeBottomTop = tubeTopY[i] + 64;
            int tubeTopBottom = tubeY[i];

            // Top tube collision
            if(birdRight > tubeLeft && birdLeft < tubeRight &&
               birdTop < tubeBottomTop) {
                birdVy = 0;
                birdY = tubeBottomTop - 6;
            }

            // Bottom tube collision
            if(birdRight > tubeLeft && birdLeft < tubeRight &&
               birdBottom > tubeTopBottom) {
                birdVy = 0;
                birdY = tubeTopBottom - (32 - 14);
            }
        }

        // Update and render sprites
        obj_set_pos(&obj_buffer[NUM_TUBES*2], birdX, birdY);
        oam_copy(oam_mem, obj_buffer, 128);
    }

    return 0;
}

