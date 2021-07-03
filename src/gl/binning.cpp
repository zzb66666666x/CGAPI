#include "configs.h"
#include "binning.h"
#include <glm/glm.hpp>
#include <set>

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))

static int tile_overlap_triangle(Triangle* tri, int tile_begin_x, int tile_begin_y);

void Bin::get_tile(int tile_x, int tile_y, int * pixel_tile_x, int * pixel_tile_y){
    if (pixel_tile_x == nullptr || pixel_tile_y == nullptr || tile_x < 0 || tile_y < 0)
        return;
    *pixel_tile_x = pixel_bin_x + TILE_SIDE_LENGTH * tile_x;
    *pixel_tile_y = pixel_bin_y + TILE_SIDE_LENGTH * tile_y;
}

Bin* ScreenBins::get_bin(int pixel_pos_x, int pixel_pos_y){
    int x = pixel_pos_x / BIN_SIDE_LENGTH;
    int y = pixel_pos_y / BIN_SIDE_LENGTH;
    int idx = x + num_bins_along_x * y;
    if (idx >= bins.size())
        return nullptr;
    return &(bins[idx]);
}

Bin* ScreenBins::get_bin_by_index(int idx){
    if (idx >= bins.size())
        return nullptr;
    return &(bins[idx]);
}

void ScreenBins::reset_counter(){
    counter = 0;
}

Bin* ScreenBins::pop_bin(){
    Bin* ans = get_bin_by_index(counter);
    counter ++;
    return ans;
}

std::set<Bin*> binning_overlap(Triangle* tri, ScreenBins* screen_bins){
    std::set<Bin*> ans;
    if (tri == nullptr)
        return ans;
    // AABB
    glm::vec4* screen_pos = tri->screen_pos;
    int minx, maxx, miny, maxy, x, y;   // origin point at left down corner
    minx = MIN(screen_pos[0].x, MIN(screen_pos[1].x, screen_pos[2].x));
    miny = MIN(screen_pos[0].y, MIN(screen_pos[1].y, screen_pos[2].y));
    maxx = MAX(screen_pos[0].x, MAX(screen_pos[1].x, screen_pos[2].x));
    maxy = MAX(screen_pos[0].y, MAX(screen_pos[1].y, screen_pos[2].y));
    ans.insert(screen_bins->get_bin(minx, miny));
    ans.insert(screen_bins->get_bin(maxx, maxy));
    ans.insert(screen_bins->get_bin(maxx, miny));
    ans.insert(screen_bins->get_bin(minx, maxy));
    return ans;
}

static int tile_overlap_triangle(Triangle* tri, int tile_begin_x, int tile_begin_y){
    if (tri->inside(tile_begin_x + 0.5f, tile_begin_y + 0.5f) || 
        tri->inside(tile_begin_x + TILE_SIDE_LENGTH - 0.5f, tile_begin_y) ||
        tri->inside(tile_begin_x + 0.5f, tile_begin_y + TILE_SIDE_LENGTH - 0.5f) ||
        tri->inside(tile_begin_x + TILE_SIDE_LENGTH - 0.5f, tile_begin_y + TILE_SIDE_LENGTH -0.5f))
        return 1;
    return 0;
}

// return bitmask has 64bits
// arranged in the order of going from left down corner to top right corner
// for the bit mask, this order goes from least significant bit to the most
// aka. 2^0 maps to left down corner, 2^63 maps to top right corner
uint64_t compute_cover_mask(Triangle* tri, Bin* bin){
    uint64_t ans = 0;
    uint64_t bit_mask = 1;
    int pixel_tile_x, pixel_tile_y;
    for (int i=0; i<TILE_NUM_PER_AXIS; i++){
        for (int j=0; j<TILE_NUM_PER_AXIS; j++){
            bin->get_tile(i, j, &pixel_tile_x, &pixel_tile_y);
            if (tile_overlap_triangle(tri, pixel_tile_x, pixel_tile_y)){
                ans = ans | bit_mask;
            }   
            bit_mask = bit_mask << 1;
        }
    }
    return ans;
}