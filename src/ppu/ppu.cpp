#include "ppu.h"
#include "../mem/Bus.h"
#include <cstdio>
#include <fstream>
#include <SDL2/SDL.h>

uint8_t inidisp;
uint8_t coldata;

int scanline = 0;
int cur_cycles = 0;
int frames = 0;

uint8_t vram[32*1024];
uint16_t vram_addr;
uint16_t cg_addr = 0;
uint8_t* cgram;

uint8_t vmain;

uint8_t bgmode, bg_tmap_start[4];

SDL_Window* window;
SDL_Renderer* renderer;
SDL_Texture* screen_texture;
uint32_t framebuffer[256*256];

void PPU::Init()
{
    window = SDL_CreateWindow("SuperNinty", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1024, 896, 0);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    screen_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC, 256, 256);

    cgram = new uint8_t[512];
}

void RenderScreen()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_QUIT:
            SDL_Quit();
            exit(1);
        }
    }

    uint16_t base_addr = (bg_tmap_start[0] >> 2) << 11;

    printf("Drawing screen\n");

    for (int y = 0; y < 32; y++)
    {
        for (int x = 0; x < 32; x++)
        {
            uint16_t addr = base_addr + (y*32*2) + (x*2);
            
            uint16_t tile = *(uint16_t*)&vram[addr];
            // printf("Reading tile (%d,%d) (0x%04x) from 0x%04x\n", x, y, tile, addr);

            uint8_t palette = (tile >> 10) & 0x7;
            tile &= 0x3FF;
            
            for (int tile_y = 0; tile_y < 8; tile_y++)
            {
                // 8 groups of 4 bitplanes per tile
                uint16_t addr = (tile * 32) + (tile_y*4);
                uint8_t bplane_1 = vram[addr];
                uint8_t bplane_2 = vram[addr+1];
                uint8_t bplane_3 = vram[addr+2];
                uint8_t bplane_4 = vram[addr+3];

                // printf("Read tile bitplanes (%x, %x, %x, %x) from 0x%04x\n", bplane_1, bplane_2, bplane_3, bplane_4, addr);

                for (int tile_x = 0; tile_x < 8; tile_x++)
                {
                    uint8_t pal_num;
                    switch (tile_x)
                    {
                    case 0: pal_num = (((bplane_1 >> 7) & 1) << 0) | (((bplane_2 >> 7) & 1) << 1) | (((bplane_3 >> 7) & 1) << 2) | (((bplane_4 >> 7) & 1) << 3); break;
                    case 1: pal_num = (((bplane_1 >> 6) & 1) << 0) | (((bplane_2 >> 6) & 1) << 1) | (((bplane_3 >> 6) & 1) << 2) | (((bplane_4 >> 6) & 1) << 3); break;
                    case 2: pal_num = (((bplane_1 >> 5) & 1) << 0) | (((bplane_2 >> 5) & 1) << 1) | (((bplane_3 >> 5) & 1) << 2) | (((bplane_4 >> 5) & 1) << 3); break;
                    case 3: pal_num = (((bplane_1 >> 4) & 1) << 0) | (((bplane_2 >> 4) & 1) << 1) | (((bplane_3 >> 4) & 1) << 2) | (((bplane_4 >> 4) & 1) << 3); break;
                    case 4: pal_num = (((bplane_1 >> 3) & 1) << 0) | (((bplane_2 >> 3) & 1) << 1) | (((bplane_3 >> 3) & 1) << 2) | (((bplane_4 >> 3) & 1) << 3); break;
                    case 5: pal_num = (((bplane_1 >> 2) & 1) << 0) | (((bplane_2 >> 2) & 1) << 1) | (((bplane_3 >> 2) & 1) << 2) | (((bplane_4 >> 2) & 1) << 3); break;
                    case 6: pal_num = (((bplane_1 >> 1) & 1) << 0) | (((bplane_2 >> 1) & 1) << 1) | (((bplane_3 >> 1) & 1) << 2) | (((bplane_4 >> 1) & 1) << 3); break;
                    case 7: pal_num = (((bplane_1 >> 0) & 1) << 0) | (((bplane_2 >> 0) & 1) << 1) | (((bplane_3 >> 0) & 1) << 2) | (((bplane_4 >> 0) & 1) << 3); break;
                    default: exit(1);
                    }

                    uint16_t color = *(uint16_t*)&cgram[(palette*16)+(pal_num*4)];
                    int r = color & 0x1F;
                    int g = (color >> 5) & 0x1F;
                    int b = (color >> 10) & 0x1F;

                    framebuffer[(y*256*8)+(x*8) + tile_x + (tile_y*256)] = (r << 24) | (g << 16) | (b << 8) | 0xff;
                }
            }
        }
    }

    SDL_UpdateTexture(screen_texture, NULL, framebuffer, 256*sizeof(uint32_t));
    SDL_RenderCopy(renderer, screen_texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

void PPU::Dump()
{
    std::ofstream out("vram.bin");

    out.write((char*)vram, 32*1024);
    out.close();

    printf("Dumping cgram\n");

    out.open("cgram.bin");

    out.write((char*)cgram, 512);
    out.close();

    printf("Done\n");

    RenderScreen();
    for (int i = 0; i < 10000000; i++)
    {
        i -= 1;
        i += 1;
    }
}
#define printf(x, ...) 0

void PPU::Tick(int cycles)
{
    cur_cycles += cycles;

    if (cur_cycles >= 341)
    {
        if (scanline == 0)
        {
            Bus::SetVblank(false);
        }
        else if (scanline == 225)
        {
            RenderScreen();
            Bus::SetVblank(true);
        }
        cur_cycles -= 341;
        scanline++;
        if (scanline == 262)
        {
            frames++;
            scanline = 0;
        }
    }

    printf("V:%3d H:%3d F:%2d\n", scanline, cur_cycles, frames);
}

#undef printf

void PPU::WriteINIDISP(uint8_t data)
{
    inidisp = data;
    if (data == 0x0f)
        RenderScreen();
}

void PPU::WriteCOLDATA(uint8_t data)
{
    coldata = data;
}

void PPU::WriteBGMODE(uint8_t data)
{
    printf("[PPU]: Writing 0x%02x to MODE\n", data);
    bgmode = data;
}

void PPU::WriteBGTMAPSTART(int index, uint8_t data)
{
    bg_tmap_start[index] = data;
}

void PPU::WriteVMADD(uint16_t data)
{
    vram_addr = data;
    printf("[PPU]: Setting VRAM addr to 0x%04x\n", vram_addr<<1);
}

void PPU::WriteVMAIN(uint8_t data)
{
    vmain = data;
}

void PPU::WriteVMDATA(uint16_t data)
{
    printf("Sending 0x%04x to VRAM (0x%04x)\n", data, vram_addr<<1);
    *(uint16_t*)&vram[(vram_addr << 1) & 0x7FFF] = data;
    vram_addr++;
}

void PPU::WriteVMDATALow(uint8_t data)
{
    vram[(vram_addr << 1) & 0x7FFF] = data;
    if (!((vmain >> 7) & 1))
        vram_addr++;
}

void PPU::WriteVMDATAHi(uint8_t data)
{
    vram[((vram_addr << 1) & 0x7FFF)+1] = data;
    if (((vmain >> 7) & 1))
        vram_addr++;
}

void PPU::WriteCGDATA(uint8_t data)
{
    cgram[cg_addr++] = data;
}
