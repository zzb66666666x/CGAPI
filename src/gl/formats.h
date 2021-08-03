#ifndef _FORMATS_H
#define _FORMATS_H

// F: float
// U: unsigned
// C: channel
// <depth> <F|U> <C> <channel_num>

typedef struct{
    float R;
    float G;
    float B;
}COLOR_32FC3;

typedef struct{
    unsigned char R;
    unsigned char G;
    unsigned char B;
}COLOR_8UC3;

typedef struct{
    float R;
    float G;
    float B;
    float A;
}COLOR_32FC4;

typedef struct{
    unsigned char R;
    unsigned char G;
    unsigned char B;
    unsigned char A;
}COLOR_8UC4;

typedef struct{
    float R;
}COLOR_32FC1;


#define FORMAT_COLOR_32FC3  1
#define FORMAT_COLOR_8UC3   2
#define FORMAT_COLOR_32FC4  3
#define FORMAT_COLOR_8UC4   4
#define FORMAT_COLOR_32FC1  5


#endif
