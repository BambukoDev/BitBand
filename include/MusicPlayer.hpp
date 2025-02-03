#pragma once

#include <pico/stdlib.h>

#define DMA_BUFFER_LENGTH 2200
#define RAM_BUFFER_LENGTH (4 * DMA_BUFFER_LENGTH)

static uint wrap;             // Largest value a sample can be + 1
static int mid_point;         // wrap divided by 2
static float fraction = 1;    // Divider used for PWM
static int repeat_shift = 1;  // Defined by the sample rate

static int dma_channel[2];       // The 2 DMA channels used for DMA ping pong

// Have 2 buffers in RAM that are used to DMA the samples to the PWM engine
static uint32_t dma_buffer[2][DMA_BUFFER_LENGTH];
static int dma_buffer_index = 0;  // Index into active DMA buffer

