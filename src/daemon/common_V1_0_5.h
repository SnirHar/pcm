// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2009-2018, Intel Corporation
// written by Steven Briscoe

#pragma once

#include <cstring>
#include <stdint.h>
#include <algorithm>

#ifndef ALIGN64
#define ALIGN64 __attribute__((aligned((64))))
#endif

// snapshot of V1.0.5 interface

namespace PCMDaemon_V1_0_5 {

    constexpr auto VERSION_SIZE = 12;
    constexpr const char VERSION[] = "1.0.5";
    constexpr const char DEFAULT_SHM_ID_LOCATION[] = "/tmp/opcm-daemon-shm-id";
    constexpr auto MAX_CPU_CORES = 4096;
    constexpr auto MAX_SOCKETS = 256;
    constexpr auto MEMORY_MAX_IMC_CHANNELS = 12;
    constexpr auto QPI_MAX_LINKS = MAX_SOCKETS * 4;

    struct Version
    {
        char version[VERSION_SIZE]; // version (null-terminated string)
    };

    typedef int int32;
    typedef long int64;
    typedef unsigned int uint32;
    typedef unsigned long uint64;

    struct PCMSystem {
        uint32 numOfCores;
        uint32 numOfOnlineCores;
        uint32 numOfSockets;
        uint32 numOfOnlineSockets;
        uint32 numOfQPILinksPerSocket;
    public:
        PCMSystem() :
            numOfCores(0),
            numOfOnlineCores(0),
            numOfSockets(0),
            numOfOnlineSockets(0),
            numOfQPILinksPerSocket(0) {}
    } ALIGN64;

    typedef struct PCMSystem PCMSystem;

    struct PCMCoreCounter {
        uint64 coreId = 0;
        int32 socketId = 0;
        double instructionsPerCycle = 0.;
        uint64 cycles = 0;
        uint64 instructionsRetired = 0;
        double execUsage = 0.;
        double relativeFrequency = 0.;
        double activeRelativeFrequency = 0.;
        uint64 l3CacheMisses = 0;
        uint64 l3CacheReference = 0;
        uint64 l2CacheMisses = 0;
        double l3CacheHitRatio = 0.;
        double l2CacheHitRatio = 0.;
        double l3CacheMPI = 0.;
        double l2CacheMPI = 0.;
        bool l3CacheOccupancyAvailable;
        uint64 l3CacheOccupancy;
        bool localMemoryBWAvailable;
        uint64 localMemoryBW;
        bool remoteMemoryBWAvailable;
        uint64 remoteMemoryBW;
        uint64 localMemoryAccesses = 0;
        uint64 remoteMemoryAccesses = 0;
        int32 thermalHeadroom = 0;

    public:
        PCMCoreCounter() :
            l3CacheOccupancyAvailable(false),
            l3CacheOccupancy(0),
            localMemoryBWAvailable(false),
            localMemoryBW(0),
            remoteMemoryBWAvailable(false),
            remoteMemoryBW(0) {}
    } ALIGN64;

    typedef struct PCMCoreCounter PCMCoreCounter;

    struct PCMCore {
        PCMCoreCounter cores[MAX_CPU_CORES];
        bool packageEnergyMetricsAvailable;
        double energyUsedBySockets[MAX_SOCKETS] ALIGN64;

    public:
        PCMCore() :
            packageEnergyMetricsAvailable(false) {
            for (int i = 0; i < MAX_SOCKETS; ++i)
            {
                energyUsedBySockets[i] = -1.0;
            }
        }
    } ALIGN64;

    typedef struct PCMCore PCMCore;

    struct PCMMemoryChannelCounter {
        float read;
        float write;
        float total;

    public:
        PCMMemoryChannelCounter() :
            read(-1.0),
            write(-1.0),
            total(-1.0) {}
    } ALIGN64;

    typedef struct PCMMemoryChannelCounter PCMMemoryChannelCounter;

    struct PCMMemorySocketCounter {
        uint64 socketId = 0;
        PCMMemoryChannelCounter channels[MEMORY_MAX_IMC_CHANNELS];
        uint32 numOfChannels;
        float read;
        float write;
        float partialWrite;
        float total;
        double dramEnergy;

    public:
        PCMMemorySocketCounter() :
            numOfChannels(0),
            read(-1.0),
            write(-1.0),
            partialWrite(-1.0),
            total(-1.0),
            dramEnergy(0.0) {}
    } ALIGN64;

    typedef struct PCMMemorySocketCounter PCMMemorySocketCounter;

    struct PCMMemorySystemCounter {
        float read;
        float write;
        float total;

    public:
        PCMMemorySystemCounter() :
            read(-1.0),
            write(-1.0),
            total(-1.0) {}
    } ALIGN64;

    typedef struct PCMMemorySystemCounter PCMMemorySystemCounter;

    struct PCMMemory {
        PCMMemorySocketCounter sockets[MAX_SOCKETS];
        PCMMemorySystemCounter system;
        bool dramEnergyMetricsAvailable;

    public:
        PCMMemory() :
            dramEnergyMetricsAvailable(false) {}
    } ALIGN64;

    typedef struct PCMMemory PCMMemory;

    struct PCMQPILinkCounter {
        uint64 bytes;
        double utilization;

    public:
        PCMQPILinkCounter() :
            bytes(0),
            utilization(-1.0) {}
    } ALIGN64;

    typedef struct PCMQPILinkCounter PCMQPILinkCounter;

    struct PCMQPISocketCounter {
        uint64 socketId = 0;
        PCMQPILinkCounter links[QPI_MAX_LINKS];
        uint64 total;

    public:
        PCMQPISocketCounter() :
            total(0) {}
    } ALIGN64;

    typedef struct PCMQPISocketCounter PCMQPISocketCounter;

    struct PCMQPI {
        PCMQPISocketCounter incoming[MAX_SOCKETS];
        uint64 incomingTotal;
        PCMQPISocketCounter outgoing[MAX_SOCKETS];
        uint64 outgoingTotal;
        bool incomingQPITrafficMetricsAvailable;
        bool outgoingQPITrafficMetricsAvailable;

    public:
        PCMQPI() :
            incomingTotal(0),
            outgoingTotal(0),
            incomingQPITrafficMetricsAvailable(false),
            outgoingQPITrafficMetricsAvailable(false) {}
    } ALIGN64;

    typedef struct PCMQPI PCMQPI;

    struct SharedPCMCounters {
        PCMSystem system;
        PCMCore core;
        PCMMemory memory;
        PCMQPI qpi;
    } ALIGN64;

    typedef struct SharedPCMCounters SharedPCMCounters;

    struct SharedPCMState : public Version {
        uint64 lastUpdateTscBegin;
        uint64 timestamp;
        uint64 cyclesToGetPCMState;
        uint32 pollMs;
        SharedPCMCounters pcm;
        uint64 lastUpdateTscEnd;

    public:
        SharedPCMState() :
            lastUpdateTscBegin(0),
            timestamp(0),
            cyclesToGetPCMState(0),
            pollMs(-1),
            lastUpdateTscEnd(0)
        {
            std::fill(this->version, this->version + VERSION_SIZE, 0);
        }
    } ALIGN64;

    typedef struct SharedPCMState SharedPCMState;
}
