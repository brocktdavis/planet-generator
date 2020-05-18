#ifndef CONFIG_H
#define CONFIG_H

// Elevation/noise constatns
const float elevation_divisor = 10.0f;
const float el_frequency = 1.0f;
const float el_amplitude = 1.0f;
const float el_lacunarity = 6.0f;
const float el_persistence = 1 / el_lacunarity;

const float kNear = 0.1f;
const float kFar = 1000.0f;
const float kFov = 45.0f;

const float kFloorXMin = -5.0f;
const float kFloorXMax = 5.0f;
const float kFloorZMin = -5.0f;
const float kFloorZMax = 5.0f;
const float kFloorY = -2.0f;

const float kScrollSpeed = 64.0f;

#endif
