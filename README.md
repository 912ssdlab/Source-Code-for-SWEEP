# SWEEP:Gathering Hot Read/Write Data Together to Minimize Read Reclaims in SSDs
## Introduction
SWEEP is an optimization solution designed to address the Read Disturb issue in Solid-State Drives (SSDs). Read disturb is a circuit-level noise in high-density SSDs, which may corrupt existing data and lead to high read error rates.

The conventional Read Reclaim (RR) approach mitigates the negative effects of read disturbs by migrating valid data pages, but this adversely impacts both I/O responsiveness and SSD lifetime.

SWEEP intelligently aggregates hot read and hot write data pages and leverages the Garbage Collection (GC) mechanism to reset read disturbs, thereby minimizing the number of Read Reclaim operations.

## 2.Requirements
1.gcc 17.0
2.make 3.81

## 3.Directory
ssd.c:Create SSD instance
initialize.c:Configure SSD according to the parameter file
pagemap.c:Used to map host logical addresses to flash physical addresses
flash.c:Simulate read, write, and erase operations of flash memory arrays
LRU.c:Implementation of LRU Algorithm
avlTree.c:Implementation of avltree Algorithm

