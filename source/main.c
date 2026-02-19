// (C) Seth Jones 2026
// Flappy Bird prototype with bird offsets and three tubes
#include <tonc.h>
#include <tonc_input.h>
#include <string.h>
#include <stdlib.h>       // for rand()
#include "Background.h"   // background exported by grit
#include "tube.h"         // bottom tube
#include "tube_top.h"     // top tube
#include "bird.h"         // 32x32 bird sprite

// --- Sprite OAM buffer ---
OBJ_ATTR obj_buffer[128];

// --- Game Constants ---
#define NUM_TUBES 3
#define TUBE_GAP 60       // vertical gap between top/bottom tube
#define TUBE_SPACING 120  // horizontal spacing between tubes
#define TUBE_SPEED 1
#define BIRD_GRAVITY 1
#define BIRD_JUMP -5
#define BIRD_TERMINAL 3

int main(void)
{
    // --- Display Setup ---
    REG_DISPCNT = DCNT_MODE0 | DCNT_OBJ | DCNT_OBJ_1D | DCNT_BG0;

    // --- Load Background ---
    memcpy(pal_bg_mem, BackgroundPal, BackgroundPalLen);
    memcpy(&tile_mem[0][0], BackgroundTiles, BackgroundTilesLen);
    memcpy(&se_mem[31][0], BackgroundMap, BackgroundMapLen);
    REG_BG0CNT = BG_CBB(0) | BG_SBB(31) | BG_4BPP | BG_REG_32x32;

    // --- Load Tube Palette & Tiles ---
    memcpy(&pal_obj_mem[0], tubePal, 16 * 2);
    memcpy(&tile_mem[4][0], tubeTiles, sizeof(tubeTiles));
    memcpy(&tile_mem[4][sizeof(tubeTiles)/32], tube_topTiles, sizeof(tube_topTiles));

    // --- Load Bird Palette & Tiles ---
    memcpy(&pal_obj_mem[16], birdPal, 16 * 2); // palette bank 1
    memcpy(&tile_mem[4][64], birdTiles, sizeof(birdTiles)); // after tubes

    // --- Initialize Tube Arrays ---
    int tubeX[NUM_TUBES];
    int tubeY[NUM_TUBES];      // bottom tube Y position
    int tubeTopY[NUM_TUBES];   // top tube Y position

    for(int i = 0; i < NUM_TUBES; i++)
    {
        tubeX[i] = 240 + i * TUBE_SPACING;
        tubeY[i] = 80 + (rand() % 40);           // bottom tube Y
        tubeTopY[i] = tubeY[i] - TUBE_GAP - 64;  // top tube position
    }

    // --- Bird Position & Physics ---
    int birdX = 80, birdY = 60;
    int birdVy = 0;

    // --- Configure Tube Sprites ---
    for(int i = 0; i < NUM_TUBES; i++)
    {
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

    // --- Configure Bird Sprite ---
    obj_set_attr(&obj_buffer[NUM_TUBES*2],
        ATTR0_SQUARE,
        ATTR1_SIZE_32,
        ATTR2_PALBANK(1) | 64);
    obj_set_pos(&obj_buffer[NUM_TUBES*2], birdX, birdY);

    // --- Push initial OAM ---
    oam_copy(oam_mem, obj_buffer, 128);

    // --- Main Loop ---
    while(1)
    {
        vid_vsync();
        key_poll();

        // --- Move Tubes ---
        for(int i = 0; i < NUM_TUBES; i++)
        {
            tubeX[i] -= TUBE_SPEED;

            // Reset tubes when offscreen
            if(tubeX[i] < -32)
            {
                tubeX[i] = 240;
                tubeY[i] = 80 + (rand() % 40);
                tubeTopY[i] = tubeY[i] - TUBE_GAP - 64;
            }

            // Update OAM
            obj_set_pos(&obj_buffer[i*2], tubeX[i], tubeY[i]);      // bottom
            obj_set_pos(&obj_buffer[i*2+1], tubeX[i], tubeTopY[i]); // top
        }

        // --- Bird Gravity & Jump ---
        if(key_is_down(KEY_A)) birdVy = BIRD_JUMP;
        else
        {
            birdVy += BIRD_GRAVITY;
            if(birdVy > BIRD_TERMINAL) birdVy = BIRD_TERMINAL;
        }
        birdY += birdVy;

        // --- Keep Bird On Screen ---
        if(birdY < 0) birdY = 0;
        if(birdY > 160 - 32) birdY = 160 - 32;

        // --- Collision Detection ---
        // Bird visible rectangle
        int birdLeft = birdX + 5;
        int birdRight = birdX + 32 - 8;
        int birdTop = birdY + 6;
        int birdBottom = birdY + 32 - 14;

        for(int i = 0; i < NUM_TUBES; i++)
        {
            int tubeLeft = tubeX[i];
            int tubeRight = tubeX[i] + 32;
            int tubeBottomTop = tubeTopY[i] + 64; // top tube bottom edge
            int tubeTopBottom = tubeY[i];          // bottom tube top edge

            // Check top tube collision
            if(birdRight > tubeLeft && birdLeft < tubeRight &&
               birdTop < tubeBottomTop)
            {
                // Collision detected
                birdVy = 0;
                birdY = tubeBottomTop - 6; // push bird below
            }

            // Check bottom tube collision
            if(birdRight > tubeLeft && birdLeft < tubeRight &&
               birdBottom > tubeTopBottom)
            {
                // Collision detected
                birdVy = 0;
                birdY = tubeTopBottom - (32 - 14); // push bird above
            }
        }

        // --- Update Bird OAM ---
        obj_set_pos(&obj_buffer[NUM_TUBES*2], birdX, birdY);

        // --- Push All Sprites ---
        oam_copy(oam_mem, obj_buffer, 128);
    }

    return 0;
}

