
//{{BLOCK(Background)

//======================================================================
//
//	Background, 256x256@4, 
//	+ palette 256 entries, not compressed
//	+ 1 tiles (t reduced) not compressed
//	+ regular map (in SBBs), not compressed, 32x32 
//	Total size: 512 + 32 + 2048 = 2592
//
//	Time-stamp: 2026-02-15, 23:41:26
//	Exported by Cearn's GBA Image Transmogrifier, v0.9.2
//	( http://www.coranac.com/projects/#grit )
//
//======================================================================

#ifndef GRIT_BACKGROUND_H
#define GRIT_BACKGROUND_H

#define BackgroundTilesLen 32
extern const unsigned int BackgroundTiles[8];

#define BackgroundMapLen 2048
extern const unsigned short BackgroundMap[1024];

#define BackgroundPalLen 512
extern const unsigned short BackgroundPal[256];

#endif // GRIT_BACKGROUND_H

//}}BLOCK(Background)
