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
    // === DISPLAY SETUP ===
    // MODE0 = text mode, OBJ = enable sprites, OBJ_1D = linear OAM, BG0 = enable background layer
    REG_DISPCNT = DCNT_MODE0 | DCNT_OBJ | DCNT_OBJ_1D | DCNT_BG0;

    // === LOAD BACKGROUND ===
    // Copy background color palette into GBA memory
    memcpy(pal_bg_mem, BackgroundPal, BackgroundPalLen);
    // Copy background tiles (8x8 pixel graphics) into tile memory block 0
    memcpy(&tile_mem[0][0], BackgroundTiles, BackgroundTilesLen);
    // Copy background map (which tiles go where) into screen entry memory block 31
    memcpy(&se_mem[31][0], BackgroundMap, BackgroundMapLen);
    // Configure background control: use tile block 0, screen block 31, 4-bit color, 32x32 screen size
    REG_BG0CNT = BG_CBB(0) | BG_SBB(31) | BG_4BPP | BG_REG_32x32;

    // === LOAD TUBE GRAPHICS ===
    // Copy tube palette into sprite palette bank 0
    memcpy(&pal_obj_mem[0], tubePal, 16 * 2);
    // Copy bottom tube tile graphics into tile block 4
    memcpy(&tile_mem[4][0], tubeTiles, sizeof(tubeTiles));
    // Copy top tube tile graphics right after bottom tube tiles in tile block 4
    memcpy(&tile_mem[4][sizeof(tubeTiles)/32], tube_topTiles, sizeof(tube_topTiles));

    // === LOAD BIRD GRAPHICS ===
    // Copy bird palette into sprite palette bank 1 (starting at index 16)
    memcpy(&pal_obj_mem[16], birdPal, 16 * 2);
    // Copy bird tile graphics after tube tiles in tile block 4
    memcpy(&tile_mem[4][64], birdTiles, sizeof(birdTiles));

    // === INITIALIZE TUBE POSITIONS ===
    // Create arrays to store X, Y bottom, and Y top positions for 3 tubes
    int tubeX[NUM_TUBES];
    int tubeY[NUM_TUBES];      // stores bottom of bottom tube
    int tubeTopY[NUM_TUBES];   // stores top of top tube
    
    // Loop through each tube and set starting positions
    for(int i = 0; i < NUM_TUBES; i++)
    {
        // Space tubes 120 pixels apart, starting at X=240 (right edge of screen)
        tubeX[i] = 240 + i * TUBE_SPACING;
        // Random Y position between 80-120, creates variation in difficulty
        tubeY[i] = 80 + (rand() % 40);
        // Top tube goes above bottom tube, gap is 60 pixels, each tube is 64 pixels tall
        tubeTopY[i] = tubeY[i] - TUBE_GAP - 64;
    }

    // === INITIALIZE BIRD POSITION ===
    // Bird starts at X=80 (left side), Y=60 (upper middle)
    int birdX = 80, birdY = 60;
    // Vertical velocity starts at 0 (not moving)
    int birdVy = 0;

    // === CONFIGURE SPRITE ATTRIBUTES ===
    // Loop through each pair of tubes (bottom and top)
    for(int i = 0; i < NUM_TUBES; i++)
    {
        // === BOTTOM TUBE ===
        // ATTR0: TALL shape (32x64 pixels)
        // ATTR1: SIZE_64 (second dimension = 64)
        // ATTR2: Use palette bank 0, tile offset 0
        obj_set_attr(&obj_buffer[i*2],
            ATTR0_TALL,
            ATTR1_SIZE_64,
            ATTR2_PALBANK(0) | 0);
        // Set position of bottom tube sprite
        obj_set_pos(&obj_buffer[i*2], tubeX[i], tubeY[i]);

        // === TOP TUBE ===
        // Same attributes as bottom tube
        obj_set_attr(&obj_buffer[i*2+1],
            ATTR0_TALL,
            ATTR1_SIZE_64,
            // Tile offset = size of bottom tube tiles (so it starts after them)
            ATTR2_PALBANK(0) | (sizeof(tubeTiles)/32));
        // Set position of top tube sprite
        obj_set_pos(&obj_buffer[i*2+1], tubeX[i], tubeTopY[i]);
    }

    // === CONFIGURE BIRD SPRITE ===
    // ATTR0: SQUARE shape (32x32 pixels)
    // ATTR1: SIZE_32 (both dimensions = 32)
    // ATTR2: Use palette bank 1, tile offset 64 (after tube tiles)
    obj_set_attr(&obj_buffer[NUM_TUBES*2],
        ATTR0_SQUARE,
        ATTR1_SIZE_32,
        ATTR2_PALBANK(1) | 64);
    // Set bird's starting position
    obj_set_pos(&obj_buffer[NUM_TUBES*2], birdX, birdY);

    // === PUSH SPRITES TO GBA ===
    // Copy all 128 sprites from obj_buffer to OAM (hardware sprite registers) to make them visible
    oam_copy(oam_mem, obj_buffer, 128);

    // === MAIN GAME LOOP ===
    while(1)
    {
        // Wait for next screen refresh (60 FPS on GBA)
        vid_vsync();
        // Read controller input into internal buffer
        key_poll();

        // === MOVE TUBES LEFT ===
        for(int i = 0; i < NUM_TUBES; i++)
        {
            // Move each tube left by TUBE_SPEED (1 pixel/frame)
            tubeX[i] -= TUBE_SPEED;

            // When tube goes off-screen left, reset it to right side
            if(tubeX[i] < -32)
            {
                // Reset X to right edge of screen
                tubeX[i] = 240;
                // Generate new random Y position
                tubeY[i] = 80 + (rand() % 40);
                // Calculate new top tube position based on bottom
                tubeTopY[i] = tubeY[i] - TUBE_GAP - 64;
            }

            // Update sprite positions in OAM buffer
            obj_set_pos(&obj_buffer[i*2], tubeX[i], tubeY[i]);         // move bottom tube
            obj_set_pos(&obj_buffer[i*2+1], tubeX[i], tubeTopY[i]);    // move top tube
        }

        // === BIRD PHYSICS ===
        // If A button is pressed, jump (set upward velocity)
        if(key_is_down(KEY_A))
            birdVy = BIRD_JUMP;     // BIRD_JUMP = -5 (negative = up)
        else
        {
            // Apply gravity (increase downward velocity each frame)
            birdVy += BIRD_GRAVITY; // BIRD_GRAVITY = 1
            // Cap maximum fall speed
            if(birdVy > BIRD_TERMINAL)
                birdVy = BIRD_TERMINAL; // BIRD_TERMINAL = 3
        }
        // Update bird Y position based on velocity
        birdY += birdVy;

        // === KEEP BIRD ON SCREEN ===
        // Prevent bird from going above top
        if(birdY < 0)
            birdY = 0;
        // Prevent bird from going below bottom (32 = bird height)
        if(birdY > 160 - 32)
            birdY = 160 - 32;

        // === COLLISION DETECTION ===
        // Define bird's visible collision box (with padding around sprite edges)
        int birdLeft = birdX + 5;       // 5 pixel padding from left
        int birdRight = birdX + 32 - 8; // 8 pixels from right edge
        int birdTop = birdY + 6;        // 6 pixel padding from top
        int birdBottom = birdY + 32 - 14; // 14 pixels from bottom

        // Check collision against each tube
        for(int i = 0; i < NUM_TUBES; i++)
        {
            // Define tube collision boxes
            int tubeLeft = tubeX[i];      // Left edge of tube
            int tubeRight = tubeX[i] + 32; // Right edge (tube is 32 pixels wide)
            int tubeBottomTop = tubeTopY[i] + 64; // Bottom edge of top tube (top tube is 64 pixels tall)
            int tubeTopBottom = tubeY[i]; // Top edge of bottom tube

            // === CHECK TOP TUBE COLLISION ===
            // AABB (axis-aligned bounding box) collision check
            if(birdRight > tubeLeft && birdLeft < tubeRight && // X overlap?
               birdTop < tubeBottomTop) // Y overlap (bird above bottom of top tube)?
            {
                // Collision! Stop bird vertical movement
                birdVy = 0;
                // Push bird down so it doesn't overlap
                birdY = tubeBottomTop - 6;
            }

            // === CHECK BOTTOM TUBE COLLISION ===
            if(birdRight > tubeLeft && birdLeft < tubeRight && // X overlap?
               birdBottom > tubeTopBottom) // Y overlap (bird below top of bottom tube)?
            {
                // Collision! Stop bird vertical movement
                birdVy = 0;
                // Push bird up so it doesn't overlap
                birdY = tubeTopBottom - (32 - 14); // account for collision box padding
            }
        }

        // === UPDATE AND RENDER ===
        // Update bird position in sprite buffer
        obj_set_pos(&obj_buffer[NUM_TUBES*2], birdX, birdY);
        // Copy all sprite positions from buffer to GBA hardware (makes changes visible)
        oam_copy(oam_mem, obj_buffer, 128);
    }

    return 0;
}

