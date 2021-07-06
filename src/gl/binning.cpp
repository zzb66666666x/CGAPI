#include "configs.h"
#include "binning.h"
#include <glm/glm.hpp>

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))

struct scanline_data_t{
    bool has_overlap;
    std::array<float, 2> intersect_x;
};

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

CompareVec4Y compare_vec4_y;

Triangle2D::Triangle2D(glm::vec4 *p1, glm::vec4 *p2, glm::vec4 *p3){
    screen_pos_ptrs[0] = p1;
    screen_pos_ptrs[1] = p2;
    screen_pos_ptrs[2] = p3;
    std::sort(screen_pos_ptrs.begin(), screen_pos_ptrs.end(), compare_vec4_y);
}

Triangle2D::Triangle2D(glm::vec4 ** points){
    screen_pos_ptrs[0] = points[0];
    screen_pos_ptrs[1] = points[1];
    screen_pos_ptrs[2] = points[2];
    std::sort(screen_pos_ptrs.begin(), screen_pos_ptrs.end(), compare_vec4_y);
}

bool Triangle2D::intersect(float scanline_y, std::array<float, 2> * ans, Bin* bin){
    if(scanline_y > screen_pos_ptrs[2]->y || scanline_y < screen_pos_ptrs[0]->y){
        return false;
    }else{
        float boundx_1, boundx_2;
        if (screen_pos_ptrs[0]->x == screen_pos_ptrs[2]->x){
            boundx_1 = screen_pos_ptrs[0]->x;
        }else{
            boundx_1 = interp(screen_pos_ptrs[0]->y, screen_pos_ptrs[0]->x, screen_pos_ptrs[2]->y, screen_pos_ptrs[2]->x, scanline_y);
        }
        // find the second bound x
        int i = (scanline_y >= screen_pos_ptrs[1]->y)? 1 : 0; 
        if (screen_pos_ptrs[i]->x == screen_pos_ptrs[i+1]->x){
            boundx_2 = screen_pos_ptrs[i]->x;
        }else if (screen_pos_ptrs[i]->y == screen_pos_ptrs[i+1]->y){
            boundx_2 = screen_pos_ptrs[1]->x;
        }else{
            boundx_2 = interp(screen_pos_ptrs[i]->y, screen_pos_ptrs[i]->x, screen_pos_ptrs[i+1]->y, screen_pos_ptrs[i+1]->x, scanline_y);
        }
        float min_x, max_x;
        if (boundx_1 < boundx_2){
            min_x = boundx_1;
            max_x = boundx_2;
        }else{
            min_x = boundx_2;
            max_x = boundx_1;
        }   
        float bin_min_x = float(bin->pixel_bin_x);
        float bin_max_x = (float)(bin->pixel_bin_x + BIN_SIDE_LENGTH);
        if (min_x <= bin_max_x &&
            bin_min_x <= max_x){
            (*ans)[0] = MAX(min_x, bin_min_x);
            (*ans)[1] = MIN(max_x, bin_max_x);
            return true;
        }else{
            return false;
        }
    }
}

inline float Triangle2D::interp(float a, float Fa, float b, float Fb, float x){
    float t = (x - a)/(b - a);
    float Fx = Fa + t*(Fb - Fa);
    return Fx;
}


std::set<Bin*> binning_overlap(Triangle* tri, ScreenBins* screen_bins){
    std::set<Bin*> ans;
    if (tri == nullptr)
        return ans;
    // AABB
    #if(SAVE_POINTERS_IN_TRIANGLE == 0)
    glm::vec4* screen_pos = tri->screen_pos;
    int minx, maxx, miny, maxy, x, y; 
    minx = MIN(screen_pos[0].x, MIN(screen_pos[1].x, screen_pos[2].x));
    miny = MIN(screen_pos[0].y, MIN(screen_pos[1].y, screen_pos[2].y));
    maxx = MAX(screen_pos[0].x, MAX(screen_pos[1].x, screen_pos[2].x));
    maxy = MAX(screen_pos[0].y, MAX(screen_pos[1].y, screen_pos[2].y));
    #endif
    #if(SAVE_POINTERS_IN_TRIANGLE == 1)
    glm::vec4** screen_pos = tri->screen_pos_ptrs;
    int minx, maxx, miny, maxy, x, y; 
    minx = MIN(screen_pos[0]->x, MIN(screen_pos[1]->x, screen_pos[2]->x));
    miny = MIN(screen_pos[0]->y, MIN(screen_pos[1]->y, screen_pos[2]->y));
    maxx = MAX(screen_pos[0]->x, MAX(screen_pos[1]->x, screen_pos[2]->x));
    maxy = MAX(screen_pos[0]->y, MAX(screen_pos[1]->y, screen_pos[2]->y));
    #endif
    ans.insert(screen_bins->get_bin(minx, miny));
    ans.insert(screen_bins->get_bin(maxx, maxy));
    ans.insert(screen_bins->get_bin(maxx, miny));
    ans.insert(screen_bins->get_bin(minx, maxy));
    if (ans.find(nullptr) != ans.end()){
        throw std::runtime_error("you shouldn't find triangles outside of screen\n");
    }
    return ans;
}

// return bitmask has 64bits
// arranged in the order of going from left down corner to top right corner
// for the bit mask, this order goes from least significant bit to the most
// aka. 2^0 maps to left down corner, 2^63 maps to top right corner
uint64_t compute_cover_mask(Triangle* tri, Bin* bin){
    if (tri==nullptr || bin==nullptr)
        throw std::runtime_error("cannot compute mask because of nullptr params\n");
    uint64_t ans = 0;    
    // scanline algorithm
    #if(SAVE_POINTERS_IN_TRIANGLE == 0)
    Triangle2D tri_2d(&(tri->screen_pos[0]), &(tri->screen_pos[1]), &(tri->screen_pos[2]));
    #endif
    #if(SAVE_POINTERS_IN_TRIANGLE == 1)
    Triangle2D tri_2d(tri->screen_pos_ptrs);
    #endif
    scanline_data_t scanline_data[BIN_SIDE_LENGTH];
    for (int tile_y=0; tile_y<TILE_NUM_PER_AXIS; tile_y++){
        float min_x = bin->pixel_bin_x + BIN_SIDE_LENGTH;
        float max_x = bin->pixel_bin_x;
        float cur_scanline_y = (float)(bin->pixel_bin_y + tile_y * TILE_SIDE_LENGTH);
        bool flag = false;
        if (cur_scanline_y <= tri_2d.screen_pos_ptrs[2]->y &&
            tri_2d.screen_pos_ptrs[0]->y <= cur_scanline_y+TILE_SIDE_LENGTH-1){
            for (int i = 0; i<TILE_SIDE_LENGTH; i++){
                scanline_data_t& line = scanline_data[tile_y * TILE_SIDE_LENGTH + i];
                bool ret = tri_2d.intersect(cur_scanline_y + (float)i, &(line.intersect_x), bin);
                flag |= ret;
                line.has_overlap = ret;
                if (ret){
                    min_x = MIN(min_x, line.intersect_x[0]);
                    max_x = MAX(max_x, line.intersect_x[1]);
                }
            }
        }
        // the cover mask is not all zero for the whole tile row
        if (flag){
            // loop over tiles along x dir
            int pixel_tile_begin_x, pixel_tile_end_x;
            for (int tile_x = 0; tile_x<TILE_NUM_PER_AXIS; tile_x++){
                pixel_tile_begin_x = bin->pixel_bin_x + TILE_SIDE_LENGTH*tile_x;
                pixel_tile_end_x = pixel_tile_begin_x + TILE_SIDE_LENGTH;
                if (pixel_tile_begin_x <= max_x &&
                    min_x <= pixel_tile_end_x){
                    // found overlap, set cover mask
                    uint64_t mask = (uint64_t)1 << (tile_x + TILE_NUM_PER_AXIS*tile_y);
                    ans |= mask;
                }
            }
        }
    }
    return ans;
}