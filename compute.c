#include <stdlib.h>
#include <math.h>
#include <cuda_runtime.h>
#include <device_launch_parameters.h>

#include "vector.h"
#include "config.h"


static vector3 *d_pos = NULL;
static vector3 *d_vel = NULL;
static double  *d_mass = NULL;
static vector3 *d_accels = NULL;

static int initialized = 0;


__global__ void compute_accels(vector3* accels, vector3* pos, double* mass, int N) {
    int i = blockIdx.y * blockDim.y + threadIdx.y;
    int j = blockIdx.x * blockDim.x + threadIdx.x;

    if (i >= N || j >= N) return;

    if (i == j) {
        accels[i * N + j][0] = 0.0;
        accels[i * N + j][1] = 0.0;
        accels[i * N + j][2] = 0.0;
        return;
    }

    double dx = pos[i][0] - pos[j][0];
    double dy = pos[i][1] - pos[j][1];
    double dz = pos[i][2] - pos[j][2];

    double dist_sq = dx*dx + dy*dy + dz*dz;
    double dist = sqrt(dist_sq);

    double accelmag = -GRAV_CONSTANT * mass[j] / dist_sq;

    accels[i * N + j][0] = accelmag * dx / dist;
    accels[i * N + j][1] = accelmag * dy / dist;
    accels[i * N + j][2] = accelmag * dz / dist;
}


__global__ void update(vector3* accels, vector3* pos, vector3* vel, int N) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= N) return;

    double ax = 0.0;
    double ay = 0.0;
    double az = 0.0;

    for (int j = 0; j < N; j++) {
        ax += accels[i * N + j][0];
        ay += accels[i * N + j][1];
        az += accels[i * N + j][2];
    }

    vel[i][0] += ax * INTERVAL;
    vel[i][1] += ay * INTERVAL;
    vel[i][2] += az * INTERVAL;

    pos[i][0] += vel[i][0] * INTERVAL;
    pos[i][1] += vel[i][1] * INTERVAL;
    pos[i][2] += vel[i][2] * INTERVAL;
}


extern "C" void compute()
    int N = NUMENTITIES;

    // Allocate + copy once
    if (!initialized) {
        cudaMalloc((void**)&d_pos, sizeof(vector3) * N);
        cudaMalloc((void**)&d_vel, sizeof(vector3) * N);
        cudaMalloc((void**)&d_mass, sizeof(double) * N);
        cudaMalloc((void**)&d_accels, sizeof(vector3) * N * N);

        cudaMemcpy(d_pos, hPos, sizeof(vector3) * N, cudaMemcpyHostToDevice);
        cudaMemcpy(d_vel, hVel, sizeof(vector3) * N, cudaMemcpyHostToDevice);
        cudaMemcpy(d_mass, mass, sizeof(double) * N, cudaMemcpyHostToDevice);

        initialized = 1;
    }

    // Launch kernel 1 (2D grid)
    dim3 block(16, 16);
    dim3 grid((N + 15) / 16, (N + 15) / 16);

    compute_accels<<<grid, block>>>(d_accels, d_pos, d_mass, N);
    cudaDeviceSynchronize();

    // Launch kernel 2 (1D grid)
    dim3 block2(256);
    dim3 grid2((N + 255) / 256);

    update<<<grid2, block2>>>(d_accels, d_pos, d_vel, N);
    cudaDeviceSynchronize();

    // Copy results back to CPU
    cudaMemcpy(hPos, d_pos, sizeof(vector3) * N, cudaMemcpyDeviceToHost);
    cudaMemcpy(hVel, d_vel, sizeof(vector3) * N, cudaMemcpyDeviceToHost);
}