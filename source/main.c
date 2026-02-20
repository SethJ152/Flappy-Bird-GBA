// (C) Seth Jones 2026
// Flappy Bird Game Beta
// Version "Angry Cow" (Beta 0.1.1)

#include <tonc.h>
#include <tonc_input.h>
#include <string.h>
#include <stdlib.h>       // for rand()
#include "Background.h"   // background image (256x256)
#include "tube.h"         // tube segment (32x16)
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

int startup(void) {
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

    // Load bird palette and tiles
    memcpy(&pal_obj_mem[16], birdPal, 16 * 2);
    memcpy(&tile_mem[4][sizeof(tubeTiles)/32], birdTiles, sizeof(birdTiles));
    return 0;
}

void play_game(void) {
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

    // Setup tube sprites (8 sprites per tube: 4 stacked for bottom, 4 for top)
    for(int i = 0; i < NUM_TUBES; i++) {
        // Bottom tube - 4 segments stacked
        obj_set_attr(&obj_buffer[i*8],
            ATTR0_WIDE,
            ATTR1_SIZE_32x16,
            ATTR2_PALBANK(0) | 0);
        obj_set_pos(&obj_buffer[i*8], tubeX[i], tubeY[i]);

        obj_set_attr(&obj_buffer[i*8+1],
            ATTR0_WIDE,
            ATTR1_SIZE_32x16,
            ATTR2_PALBANK(0) | 0);
        obj_set_pos(&obj_buffer[i*8+1], tubeX[i], tubeY[i] + 16);

        obj_set_attr(&obj_buffer[i*8+2],
            ATTR0_WIDE,
            ATTR1_SIZE_32x16,
            ATTR2_PALBANK(0) | 0);
        obj_set_pos(&obj_buffer[i*8+2], tubeX[i], tubeY[i] + 32);

        obj_set_attr(&obj_buffer[i*8+3],
            ATTR0_WIDE,
            ATTR1_SIZE_32x16,
            ATTR2_PALBANK(0) | 0);
        obj_set_pos(&obj_buffer[i*8+3], tubeX[i], tubeY[i] + 48);

        // Top tube - 4 segments stacked
        obj_set_attr(&obj_buffer[i*8+4],
            ATTR0_WIDE,
            ATTR1_SIZE_32x16,
            ATTR2_PALBANK(0) | 0);
        obj_set_pos(&obj_buffer[i*8+4], tubeX[i], tubeTopY[i]);

        obj_set_attr(&obj_buffer[i*8+5],
            ATTR0_WIDE,
            ATTR1_SIZE_32x16,
            ATTR2_PALBANK(0) | 0);
        obj_set_pos(&obj_buffer[i*8+5], tubeX[i], tubeTopY[i] + 16);

        obj_set_attr(&obj_buffer[i*8+6],
            ATTR0_WIDE,
            ATTR1_SIZE_32x16,
            ATTR2_PALBANK(0) | 0);
        obj_set_pos(&obj_buffer[i*8+6], tubeX[i], tubeTopY[i] + 32);

        obj_set_attr(&obj_buffer[i*8+7],
            ATTR0_WIDE,
            ATTR1_SIZE_32x16,
            ATTR2_PALBANK(0) | 0);
        obj_set_pos(&obj_buffer[i*8+7], tubeX[i], tubeTopY[i] + 48);
    }

    // Setup bird sprite
    obj_set_attr(&obj_buffer[NUM_TUBES*8],
        ATTR0_SQUARE,
        ATTR1_SIZE_32,
        ATTR2_PALBANK(1) | (sizeof(tubeTiles)/32));
    obj_set_pos(&obj_buffer[NUM_TUBES*8], birdX, birdY);

    // Push sprites to hardware
    oam_copy(oam_mem, obj_buffer, 128);

    // Main loop
    while(1) {
        vid_vsync();
        key_poll();

        // Check for menu (START key)
        if(key_is_down(KEY_START))
            break;

        // Move tubes left and reset when off-screen
        for(int i = 0; i < NUM_TUBES; i++) {
            tubeX[i] -= TUBE_SPEED;

            if(tubeX[i] < -32) {
                tubeX[i] = 240;
                tubeY[i] = 80 + (rand() % 40);
                tubeTopY[i] = tubeY[i] - TUBE_GAP - 64;
            }

            // Update all 8 tube segments
            obj_set_pos(&obj_buffer[i*8], tubeX[i], tubeY[i]);
            obj_set_pos(&obj_buffer[i*8+1], tubeX[i], tubeY[i] + 16);
            obj_set_pos(&obj_buffer[i*8+2], tubeX[i], tubeY[i] + 32);
            obj_set_pos(&obj_buffer[i*8+3], tubeX[i], tubeY[i] + 48);
            obj_set_pos(&obj_buffer[i*8+4], tubeX[i], tubeTopY[i]);
            obj_set_pos(&obj_buffer[i*8+5], tubeX[i], tubeTopY[i] + 16);
            obj_set_pos(&obj_buffer[i*8+6], tubeX[i], tubeTopY[i] + 32);
            obj_set_pos(&obj_buffer[i*8+7], tubeX[i], tubeTopY[i] + 48);
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
        int collision = 0;

        for(int i = 0; i < NUM_TUBES; i++) {
            int tubeLeft = tubeX[i];
            int tubeRight = tubeX[i] + 32;
            int tubeBottomTop = tubeTopY[i] + 64;
            int tubeTopBottom = tubeY[i];

            // Top tube collision
            if(birdRight > tubeLeft && birdLeft < tubeRight &&
               birdTop < tubeBottomTop) {
                collision = 1;
                break;
            }

            // Bottom tube collision
            if(birdRight > tubeLeft && birdLeft < tubeRight &&
               birdBottom > tubeTopBottom) {
                collision = 1;
                break;
            }
        }

        // If collision, return to menu
        if(collision)
            break;

        // Update and render sprites
        obj_set_pos(&obj_buffer[NUM_TUBES*8], birdX, birdY);
        oam_copy(oam_mem, obj_buffer, 128);
    }
}

int menu(void) {
    // Hide all sprites
    for(int i = 0; i < 128; i++) {
        obj_buffer[i].attr0 = ATTR0_HIDE;
    }
    oam_copy(oam_mem, obj_buffer, 128);
    
    // Wait for START to begin game
    while(1) {
        vid_vsync();
        key_poll();
        if(key_is_down(KEY_START)) {
            break;
        }
    }
    return 0;
}

int main(void) {
    startup();
    
    // Main loop: show menu, play game, repeat
    while(1) {
        menu();
        play_game();
    }

    return 0;
}

