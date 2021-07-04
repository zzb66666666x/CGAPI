#ifndef _BINNING_H
#define _BINNING_H

#include <stdint.h>
#include <queue>
#include <array>
#include <set>
#include <pthread.h>
#include "geometry.h"

//////////////////////// NOTE ////////////////////////
// in this file: screen starts from left down corner!

#define BIN_SIDE_LENGTH     64
#define TILE_SIDE_LENGTH    8
#define TILE_NUM_PER_AXIS   8 // 64/8 = 8

typedef struct{
    Triangle* tri;
    uint64_t cover_mask;
}primitive_t;

class Bin{
    public:
        Bin(int x, int y): bin_x(x), bin_y(y){
            pixel_bin_x = bin_x * BIN_SIDE_LENGTH;
            pixel_bin_y= bin_y * BIN_SIDE_LENGTH;
            is_full = true; // default
            lock = PTHREAD_MUTEX_INITIALIZER;
        }

        void get_tile(int tile_x, int tile_y, int * pixel_tile_x, int * pixel_tile_y);

        std::queue<primitive_t> tasks;
        int bin_x;
        int bin_y;
        int pixel_bin_x;
        int pixel_bin_y;
        pthread_mutex_t lock;
        bool is_full;
};

class ScreenBins{
    public:
    ScreenBins(int w, int h): width(width), height(h){
        num_bins_along_x = (w % BIN_SIDE_LENGTH == 0) ? w/BIN_SIDE_LENGTH : w/BIN_SIDE_LENGTH+1;
        num_bins_along_y = (h % BIN_SIDE_LENGTH == 0) ? h/BIN_SIDE_LENGTH : h/BIN_SIDE_LENGTH+1;
        // storage order: first go from origin to x dir, then go up in y dir, then go right in x dir
        for (int j=0; j<num_bins_along_y; j++){
            for (int i=0; i<num_bins_along_x; i++){
                Bin bin(i,j);
                if (bin.pixel_bin_x + BIN_SIDE_LENGTH > w || bin.pixel_bin_y + BIN_SIDE_LENGTH > h)
                    bin.is_full == false;
                bins.push_back(bin);
            }
        }
        counter = 0;
    }
    Bin* get_bin(int pixel_pos_x, int pixel_pos_y);
    Bin* get_bin_by_index(int idx);
    void reset_counter();
    Bin* pop_bin();

    int width;
    int height;
    int num_bins_along_x;
    int num_bins_along_y;
    std::vector<Bin> bins;
    int counter;
};

std::set<Bin*> binning_overlap(Triangle* tri, ScreenBins* screen_bins);

uint64_t compute_cover_mask(Triangle* tri, Bin* bin);

#endif