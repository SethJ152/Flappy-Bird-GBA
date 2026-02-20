// (C) Seth Jones 2026
// Flappy Bird Game (Educational Purposes Only)
// Version "Swift Eagle" (Alpha 1.0.0)

#include <tonc.h>
#include <tonc_input.h>
#include <string.h>
#include <stdlib.h>       // for rand()
#include "Background.h"   // background image (256x256)
#include "tube.h"         // tube segment (32x16)
#include "tube_top_seal.h"    // top tube cap (32x16)
#include "tube_bottom_seal.h" // bottom tube cap (32x16)
#include "bird.h"         // bird/player (32x32)

// --- SRAM Storage ---
#define SRAM_HIGHSCORE_ADDR ((u32*)0xE000000)

// Load high score from SRAM
uint32_t load_high_score(void) {
    uint32_t score = 0;
    volatile u8* sram = (volatile u8*)0xE000000;
    score |= (*sram++);
    score |= (*sram++) << 8;
    score |= (*sram++) << 16;
    score |= (*sram) << 24;
    return score;
}

// Save high score to SRAM
void save_high_score(uint32_t score) {
    volatile u8* sram = (volatile u8*)0xE000000;
    *sram++ = score & 0xFF;
    *sram++ = (score >> 8) & 0xFF;
    *sram++ = (score >> 16) & 0xFF;
    *sram = (score >> 24) & 0xFF;
}

// --- Sprite OAM buffer ---
OBJ_ATTR obj_buffer[128];

// --- Tile offsets (calculated during startup) ---
int birdTileOffset;
int topSealOffset;
int bottomSealOffset;

// --- Game Constants ---
#define NUM_TUBES 3
#define TUBE_GAP 100       // vertical gap between top/bottom tube
#define TUBE_SPACING 160  // horizontal spacing between tubes
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
    birdTileOffset = sizeof(tubeTiles)/32;
    memcpy(&tile_mem[4][birdTileOffset], birdTiles, sizeof(birdTiles));

    // Load tube seals
    topSealOffset = birdTileOffset + sizeof(birdTiles)/32;
    memcpy(&tile_mem[4][topSealOffset], tube_top_sealTiles, sizeof(tube_top_sealTiles));
    bottomSealOffset = topSealOffset + sizeof(tube_top_sealTiles)/32;
    memcpy(&tile_mem[4][bottomSealOffset], tube_bottom_sealTiles, sizeof(tube_bottom_sealTiles));
    return 0;
}

void play_game(void) {
    // Load high score from SRAM
    uint32_t highScore = load_high_score();
    uint32_t currentScore = 0;

    // Initialize tube positions
    int tubeX[NUM_TUBES];
    int tubeY[NUM_TUBES];
    int tubeTopY[NUM_TUBES];
    int tubeScored[NUM_TUBES] = {0, 0, 0};  // Track which tubes have been scored
    
    for(int i = 0; i < NUM_TUBES; i++) {
        tubeX[i] = 240 + i * TUBE_SPACING;
        // Make bottom tube reach screen bottom at 160: tubeY + 48 (seal) = 160, so tubeY = 112
        // Then add randomness while keeping bottom tube at screen bottom
        tubeY[i] = 96 + (rand() % 40);  // 96-136, with seal extending to 112-152
        tubeTopY[i] = tubeY[i] - TUBE_GAP - 48;  // Top tube is 48 pixels, then 60 pixel gap
    }

    // Initialize bird
    int birdX = 80, birdY = 60;
    int birdVy = 0;

    // Setup tube sprites (12 sprites per tube: 6 for top + 6 for bottom)
    for(int i = 0; i < NUM_TUBES; i++) {
        int topSealTile = birdTileOffset + sizeof(birdTiles)/32;
        int bottomSealTile = topSealTile + sizeof(tube_top_sealTiles)/32;
        int regularTile = 0;

        // Top tube - 5 regular segments + seal (80 pixels tall)
        obj_set_attr(&obj_buffer[i*12],
            ATTR0_WIDE, ATTR1_SIZE_32x16, ATTR2_PALBANK(0) | regularTile);
        obj_set_pos(&obj_buffer[i*12], tubeX[i], tubeTopY[i]);

        obj_set_attr(&obj_buffer[i*12+1],
            ATTR0_WIDE, ATTR1_SIZE_32x16, ATTR2_PALBANK(0) | regularTile);
        obj_set_pos(&obj_buffer[i*12+1], tubeX[i], tubeTopY[i] + 16);

        obj_set_attr(&obj_buffer[i*12+2],
            ATTR0_WIDE, ATTR1_SIZE_32x16, ATTR2_PALBANK(0) | regularTile);
        obj_set_pos(&obj_buffer[i*12+2], tubeX[i], tubeTopY[i] + 32);

        obj_set_attr(&obj_buffer[i*12+3],
            ATTR0_WIDE, ATTR1_SIZE_32x16, ATTR2_PALBANK(0) | regularTile);
        obj_set_pos(&obj_buffer[i*12+3], tubeX[i], tubeTopY[i] + 48);

        obj_set_attr(&obj_buffer[i*12+4],
            ATTR0_WIDE, ATTR1_SIZE_32x16, ATTR2_PALBANK(0) | regularTile);
        obj_set_pos(&obj_buffer[i*12+4], tubeX[i], tubeTopY[i] + 64);

        obj_set_attr(&obj_buffer[i*12+5],
            ATTR0_WIDE, ATTR1_SIZE_32x16, ATTR2_PALBANK(0) | topSealTile);
        obj_set_pos(&obj_buffer[i*12+5], tubeX[i], tubeTopY[i] + 80);

        // Bottom tube - seal + 5 regular segments (80 pixels tall)
        obj_set_attr(&obj_buffer[i*12+6],
            ATTR0_WIDE, ATTR1_SIZE_32x16, ATTR2_PALBANK(0) | bottomSealTile);
        obj_set_pos(&obj_buffer[i*12+6], tubeX[i], tubeY[i]);

        obj_set_attr(&obj_buffer[i*12+7],
            ATTR0_WIDE, ATTR1_SIZE_32x16, ATTR2_PALBANK(0) | regularTile);
        obj_set_pos(&obj_buffer[i*12+7], tubeX[i], tubeY[i] + 16);

        obj_set_attr(&obj_buffer[i*12+8],
            ATTR0_WIDE, ATTR1_SIZE_32x16, ATTR2_PALBANK(0) | regularTile);
        obj_set_pos(&obj_buffer[i*12+8], tubeX[i], tubeY[i] + 32);

        obj_set_attr(&obj_buffer[i*12+9],
            ATTR0_WIDE, ATTR1_SIZE_32x16, ATTR2_PALBANK(0) | regularTile);
        obj_set_pos(&obj_buffer[i*12+9], tubeX[i], tubeY[i] + 48);

        obj_set_attr(&obj_buffer[i*12+10],
            ATTR0_WIDE, ATTR1_SIZE_32x16, ATTR2_PALBANK(0) | regularTile);
        obj_set_pos(&obj_buffer[i*12+10], tubeX[i], tubeY[i] + 64);

        obj_set_attr(&obj_buffer[i*12+11],
            ATTR0_WIDE, ATTR1_SIZE_32x16, ATTR2_PALBANK(0) | regularTile);
        obj_set_pos(&obj_buffer[i*12+11], tubeX[i], tubeY[i] + 80);
    }

    // Setup bird sprite
    obj_set_attr(&obj_buffer[NUM_TUBES*12],
        ATTR0_SQUARE,
        ATTR1_SIZE_32,
        ATTR2_PALBANK(1) | birdTileOffset);
    obj_set_pos(&obj_buffer[NUM_TUBES*12], birdX, birdY);

    // Push sprites to hardware
    oam_copy(oam_mem, obj_buffer, 128);

    // Delay 1 second at start (60 frames at 60 FPS)
    for(int delay = 0; delay < 60; delay++) {
        vid_vsync();
    }

    // Main loop
    while(1) {
        vid_vsync();
        key_poll();

        // Check for menu (START key)
        if(key_is_down(KEY_START)) {
            // Wait for key release
            while(key_is_down(KEY_START)) {
                vid_vsync();
                key_poll();
            }
            break;
        }

        // Move tubes left and reset when off-screen
        for(int i = 0; i < NUM_TUBES; i++) {
            tubeX[i] -= TUBE_SPEED;

            if(tubeX[i] < -32) {
                // Increment score when tube passes off-screen
                if(tubeScored[i] == 0) {
                    currentScore++;
                    tubeScored[i] = 1;
                }
                // Only reset after scoring
                tubeX[i] = 240;
                tubeY[i] = 96 + (rand() % 40);
                tubeTopY[i] = tubeY[i] - TUBE_GAP - 48;
                tubeScored[i] = 0;  // Reset score flag for new tube
            }

            // Top tube - 5 regular + seal (80 pixels)
            obj_set_pos(&obj_buffer[i*12], tubeX[i], tubeTopY[i]);
            obj_set_pos(&obj_buffer[i*12+1], tubeX[i], tubeTopY[i] + 16);
            obj_set_pos(&obj_buffer[i*12+2], tubeX[i], tubeTopY[i] + 32);
            obj_set_pos(&obj_buffer[i*12+3], tubeX[i], tubeTopY[i] + 48);
            obj_set_pos(&obj_buffer[i*12+4], tubeX[i], tubeTopY[i] + 64);
            obj_set_pos(&obj_buffer[i*12+5], tubeX[i], tubeTopY[i] + 80);
            // Bottom tube - seal + 5 regular (80 pixels)
            obj_set_pos(&obj_buffer[i*12+6], tubeX[i], tubeY[i]);
            obj_set_pos(&obj_buffer[i*12+7], tubeX[i], tubeY[i] + 16);
            obj_set_pos(&obj_buffer[i*12+8], tubeX[i], tubeY[i] + 32);
            obj_set_pos(&obj_buffer[i*12+9], tubeX[i], tubeY[i] + 48);
            obj_set_pos(&obj_buffer[i*12+10], tubeX[i], tubeY[i] + 64);
            obj_set_pos(&obj_buffer[i*12+11], tubeX[i], tubeY[i] + 80);
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
        int birdLeft = birdX + 8;
        int birdRight = birdX + 32 - 8;
        int birdTop = birdY + 10;
        int birdBottom = birdY + 32 - 10;
        int collision = 0;

        for(int i = 0; i < NUM_TUBES; i++) {
            int tubeLeft = tubeX[i];
            int tubeRight = tubeX[i] + 32;
            int tubeBottomTop = tubeTopY[i] + 96;  // Top tube is now 96 pixels
            int tubeTopBottom = tubeY[i];          // Bottom tube starts here

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

        // If collision, save high score and return to menu
        if(collision) {
            // Update high score if current is better
            if(currentScore > highScore) {
                save_high_score(currentScore);
            }
            break;
        }

        // Update and render sprites
        obj_set_pos(&obj_buffer[NUM_TUBES*12], birdX, birdY);
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
            // Wait for key release
            while(key_is_down(KEY_START)) {
                vid_vsync();
                key_poll();
            }
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

