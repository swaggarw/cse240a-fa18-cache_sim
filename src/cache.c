//========================================================//
//  cache.c                                               //
//  Source file for the Cache Simulator                   //
//                                                        //
//  Implement the I-cache, D-Cache and L2-cache as        //
//  described in the README                               //
//========================================================//

#include "cache.h"
#include <stdio.h>

//
// TODO:Student Information
//
const char *studentName = "Swapnil Aggarwal";
const char *studentID   = "A53271788";
const char *email       = "swaggarw@ucsd.edu";

//------------------------------------//
//        Cache Configuration         //
//------------------------------------//

uint32_t icacheSets;     // Number of sets in the I$
uint32_t icacheAssoc;    // Associativity of the I$
uint32_t icacheHitTime;  // Hit Time of the I$

uint32_t dcacheSets;     // Number of sets in the D$
uint32_t dcacheAssoc;    // Associativity of the D$
uint32_t dcacheHitTime;  // Hit Time of the D$

uint32_t l2cacheSets;    // Number of sets in the L2$
uint32_t l2cacheAssoc;   // Associativity of the L2$
uint32_t l2cacheHitTime; // Hit Time of the L2$
uint32_t inclusive;      // Indicates if the L2 is inclusive

uint32_t blocksize;      // Block/Line size
uint32_t memspeed;       // Latency of Main Memory

//------------------------------------//
//          Cache Statistics          //
//------------------------------------//

uint64_t icacheRefs;       // I$ references
uint64_t icacheMisses;     // I$ misses
uint64_t icachePenalties;  // I$ penalties

uint64_t dcacheRefs;       // D$ references
uint64_t dcacheMisses;     // D$ misses
uint64_t dcachePenalties;  // D$ penalties

uint64_t l2cacheRefs;      // L2$ references
uint64_t l2cacheMisses;    // L2$ misses
uint64_t l2cachePenalties; // L2$ penalties


// Declaring Data Structures for Cache
uint32_t*** icache;
uint32_t** icacheset_ds;
uint32_t* isetway_ds;

uint32_t*** dcache;
uint32_t** dcacheset_ds;
uint32_t* dsetway_ds;

uint32_t*** l2cache;
uint32_t** l2cacheset_ds;
uint32_t* l2setway_ds;

uint32_t ioffset;
uint32_t iindex;
uint32_t itag;

uint32_t doffset;
uint32_t dindex;
uint32_t dtag;

uint32_t l2offset;
uint32_t l2index;
uint32_t l2tag;

_Bool ihit;
_Bool dhit;
_Bool l2hit;

_Bool imiss;
_Bool dmiss;
_Bool l2miss;

uint32_t inclusiveAddr;
uint8_t inclusiveSrc;

//------------------------------------//
//        Cache Data Structures       //
//------------------------------------//

//
//TODO: Add your Cache data structures here
//

//------------------------------------//
//          Cache Functions           //
//------------------------------------//

// Initialize the Cache Hierarchy
//
void
init_cache()
{
  // Initialize cache stats
  icacheRefs        = 0;
  icacheMisses      = 0;
  icachePenalties   = 0;
  dcacheRefs        = 0;
  dcacheMisses      = 0;
  dcachePenalties   = 0;
  l2cacheRefs       = 0;
  l2cacheMisses     = 0;
  l2cachePenalties  = 0;
  int i, j, k;

  icache = malloc(icacheSets * sizeof(uint32_t**));

  for(i = 0; i < icacheSets; i++)
  {
    icache[i] = malloc(icacheAssoc * sizeof(uint32_t*));
    
    for(j = 0; j < icacheAssoc; j++)
    {
      icache[i][j] = malloc(3 * sizeof(uint32_t)); // tag || valid  || lru
      icache[i][j][1] = 0; // Valid Bit
      icache[i][j][2] = j;
    }
  }

  dcache = malloc(dcacheSets * sizeof(uint32_t**));

  for(i = 0; i < dcacheSets; i++)
  {
    dcache[i] = malloc(dcacheAssoc * sizeof(uint32_t*));
    
    for(j = 0; j < dcacheAssoc; j++)
    {
      dcache[i][j] = malloc(3 * sizeof(uint32_t)); // tag || valid || lru
      dcache[i][j][1] = 0;
      dcache[i][j][2] = j;
    }
  }

  l2cache = malloc(l2cacheSets * sizeof(uint32_t**));

  for(i = 0; i < l2cacheSets; i++)
  {
    l2cache[i] = malloc(l2cacheAssoc * sizeof(uint32_t*));
    
    for(j = 0; j < l2cacheAssoc; j++)
    {
      l2cache[i][j] = malloc(4 * sizeof(uint32_t)); // tag || valid || lru || data_source (I or D)
      l2cache[i][j][1] = 0;
      l2cache[i][j][2] = j;
      l2cache[i][j][3] = 0;
    }

  }

  
  //
  //TODO: Initialize Cache Simulator Data Structures
  //
}

uint32_t log_block(uint32_t sizeblock)
{
  int i = 0;
  while(sizeblock > 1)
  {
    sizeblock = sizeblock/2;
    i++;
  }

  return i;
} 

// Perform a memory access through the icache interface for the address 'addr'
// Return the access time for the memory operation
//
uint32_t icache_access(uint32_t addr)
{

  //
  //TODO: Implement I$
  //

  if(icacheSets == 0)
  {
    return l2cache_access(addr);
  }

  icacheRefs++;
  imiss = 0;
  ihit = 0;

  uint32_t ioffsetbits = log_block(blocksize);
  uint32_t iindexbits = log_block(icacheSets);
  uint32_t itagbits = (32 - ioffsetbits - iindexbits);

  uint32_t ioffsetmask = (1 << ioffsetbits) - 1;
  uint32_t iindexmask = (1 << iindexbits) - 1;
  uint32_t itagmask = (1 << itagbits) - 1;

  uint32_t tempaddr = addr;

  ioffset = tempaddr & ioffsetmask;
  tempaddr = tempaddr >> ioffsetbits;
  
  iindex = tempaddr & iindexmask;
  tempaddr = tempaddr >> iindexbits;
  
  itag = tempaddr & itagmask;

  int i;
  int j;

  for(i = 0; i < icacheAssoc; i++)
  {
    if((icache[iindex][i][0] == itag) && (icache[iindex][i][1] == 1))
    {
      ihit = 1;
      imiss = 0;

      for(j = 0; j < icacheAssoc; j++)
      {
        if(icache[iindex][j][2] > icache[iindex][i][2])
        {
          icache[iindex][j][2]--;
        }
      }

      icache[iindex][i][2] = (icacheAssoc - 1);
      break;
    }
  }

  if(ihit)
  {
    return icacheHitTime;
  }

  else
  {
    ihit = 0;
    imiss = 1;

    icacheMisses++;
    int l2memspeed;
    l2memspeed = l2cache_access(addr);

    if(l2hit)
    {
      // Insert L2 hit value in L1 Cache
      // Check and insert in invalid slot first 
      int iinserted = 0;

      for(i = 0; i < icacheAssoc; i++)
      {
        if(icache[iindex][i][1] == 0) // if data is not valid
        {
          icache[iindex][i][0] = itag;
          icache[iindex][i][1] = 1;
          for(j = 0; j < icacheAssoc; j++)
          {
            if(icache[iindex][j][2] > icache[iindex][i][2])     //Update only those LRU values that are greater than the inserted memory location's LRU value
            {
              icache[iindex][j][2]--;
            }
          }

          icache[iindex][i][2] == (icacheAssoc - 1);
          iinserted = 1;
          break;
        }
      }

      if(!iinserted)
      {
        for(i = 0; i < icacheAssoc; i++)
        {
          if(icache[iindex][i][2] == 0)
          {
            icache[iindex][i][0] = itag;
            icache[iindex][i][1] = 1;
            icache[iindex][i][2] = (icacheAssoc - 1);
          }

          else
          {
            icache[iindex][i][2]--;
          }
        }
      }
    }


    if(l2miss)
    {
      if((inclusive == 1) && (inclusiveSrc = 1))    //inclusive and data replaced was actually from icache
      {
        int inclusiveIndex = ((inclusiveAddr >> ioffsetbits) & iindexmask);
        int inclusiveTag = (((inclusiveAddr >> ioffsetbits) >> iindexbits) & itagmask);
        for(i = 0; i < icacheAssoc; i++)
        {
          if(icache[inclusiveIndex][i][0] == inclusiveTag)
          {
            icache[inclusiveIndex][i][1] = 0; // In the cache, invalidate the data removed in L2
            break;       
          }
        }
      }

      if((inclusive == 1) && (inclusiveSrc = 2))    //inclusive and data replaced was actually from dcache
      {
        uint32_t doffsetbits = log_block(blocksize);
        uint32_t dindexbits = log_block(dcacheSets);
        uint32_t dtagbits = (32 - doffsetbits - dindexbits);

        uint32_t dindexmask = (1 << dindexbits) - 1;
        uint32_t dtagmask = (1 << dtagbits) - 1;

        int inclusiveIndex = ((inclusiveAddr >> doffsetbits) & dindexmask);
        int inclusiveTag = (((inclusiveAddr >> doffsetbits) >> dindexbits) & dtagmask);

        for(i = 0; i < dcacheAssoc; i++)
        {
          if(dcache[inclusiveIndex][i][0] == inclusiveTag)
          {
            dcache[inclusiveIndex][i][1] = 0; // In the cache, invalidate the data removed in L2
            break;       
          }
        }
      }

      int iinserted = 0;
      
      for(i = 0; i < icacheAssoc; i++)
      {
        if(icache[iindex][i][1] == 0)
        {
          icache[iindex][i][0] = itag;
          icache[iindex][i][1] = 1;
          for(j = 0; j < icacheAssoc; j++)
          {
            if(icache[iindex][j][2] > icache[iindex][i][2])
            {
              icache[iindex][j][2]--; 
            }
          }

          icache[iindex][i][2] = (icacheAssoc - 1);
          iinserted = 1;
          break;
        }
      }

      if(!iinserted)
      {
        for(i = 0; i < icacheAssoc; i++)
        {
          if(icache[iindex][i][2] == 0)
          {
            icache[iindex][i][0] = itag;
            icache[iindex][i][1] = 1;
            icache[iindex][i][2] = (icacheAssoc - 1);
          }

          else
          {
            icache[iindex][i][2]--;
          }
        }
      }
    }

    icachePenalties = icachePenalties + l2memspeed;
    return (icacheHitTime + l2memspeed);
  }

  return memspeed;
}

// Perform a memory access through the dcache interface for the address 'addr'
// Return the access time for the memory operation
//
uint32_t dcache_access(uint32_t addr)
{
  //
  //TODO: Implement I$
  //

  if(dcacheSets == 0)
  {
    return l2cache_access(addr);
  }

  dcacheRefs++;
  dmiss = 0;
  dhit = 0;

  uint32_t doffsetbits = log_block(blocksize);
  uint32_t dindexbits = log_block(dcacheSets);
  uint32_t dtagbits = (32 - doffsetbits - dindexbits);

  uint32_t doffsetmask = (1 << doffsetbits) - 1;
  uint32_t dindexmask = (1 << dindexbits) - 1;
  uint32_t dtagmask = (1 << dtagbits) - 1;

  uint32_t tempaddr = addr;

  doffset = tempaddr & doffsetmask;
  tempaddr = tempaddr >> doffsetbits;
  
  dindex = tempaddr & dindexmask;
  tempaddr = tempaddr >> dindexbits;
  
  dtag = tempaddr & dtagmask;

  int i;
  int j;
  // int plru;
  // int nlru;

  for(i = 0; i < dcacheAssoc; i++)
  {
    if((dcache[dindex][i][0] == dtag) && (dcache[dindex][i][1] == 1))
    {
      dhit = 1;
      dmiss = 0;

      for(j = 0; j < dcacheAssoc; j++)
      {
        if(dcache[dindex][j][2] > dcache[dindex][i][2])
        {
          dcache[dindex][j][2]--;
        }
      }

      dcache[dindex][i][2] = (dcacheAssoc - 1);
      break;
    }
  }

  if(dhit)
  {
    return dcacheHitTime;
  }

  else
  {
    dhit = 0;
    dmiss = 1;

    dcacheMisses++;
    int l2memspeed;
    l2memspeed = l2cache_access(addr);

    if(l2hit)
    {
      // Insert L2 hit value in L1 Cache
      // Check and insert in invalid slot first 
      int iinserted = 0;

      for(i = 0; i < dcacheAssoc; i++)
      {
        if(dcache[dindex][i][1] == 0) // if data is not valid
        {
          dcache[dindex][i][0] = dtag;
          dcache[dindex][i][1] = 1;
          for(j = 0; j < dcacheAssoc; j++)
          {
            if(dcache[dindex][j][2] > dcache[dindex][i][2])     //Update only those LRU values that are greater than the inserted memory location's LRU value
            {
              dcache[dindex][j][2]--;
            }
          }

          dcache[dindex][i][2] == (dcacheAssoc - 1);
          iinserted = 1;
          break;
        }
      }

      if(!iinserted)
      {
        for(i = 0; i < dcacheAssoc; i++)
        {
          if(dcache[dindex][i][2] == 0)
          {
            dcache[dindex][i][0] = dtag;
            dcache[dindex][i][1] = 1;
            dcache[dindex][i][2] = (dcacheAssoc - 1);
          }

          else
          {
            dcache[dindex][i][2]--;
          }
        }
      }
    }

    if(l2miss)
    {
      if((inclusive == 1) && (inclusiveSrc == 2))  // Removed Data is in Dcache
      {
        int inclusiveIndex = ((inclusiveAddr >> doffsetbits) & dindexmask);
        int inclusiveTag = (((inclusiveAddr >> doffsetbits) >> dindexbits) & dtagmask);
        for(i = 0; i < dcacheAssoc; i++)
        {
          if(dcache[inclusiveIndex][i][0] == inclusiveTag)
          {
            dcache[inclusiveIndex][i][1] = 0; // In the cache, invalidate the data removed in L2 (if inclusive)
            break;
          }
        }
      }

      if((inclusive == 1) && (inclusiveSrc == 1))  // Removed Data is in Icache
      {
        uint32_t ioffsetbits = log_block(blocksize);
        uint32_t iindexbits = log_block(icacheSets);
        uint32_t itagbits = (32 - ioffsetbits - iindexbits);

        uint32_t iindexmask = (1 << iindexbits) - 1;
        uint32_t itagmask = (1 << itagbits) - 1; 

        int inclusiveIndex = ((inclusiveAddr >> ioffsetbits) & iindexmask);
        int inclusiveTag = (((inclusiveAddr >> ioffsetbits) >> iindexbits) & itagmask);
        for(i = 0; i < icacheAssoc; i++)
        {
          if(icache[inclusiveIndex][i][0] == inclusiveTag)
          {
            icache[inclusiveIndex][i][1] = 0; // In the cache, invalidate the data removed in L2 (if inclusive)
            break;
          }
        }
      }      

      int iinserted = 0;
      
      for(i = 0; i < dcacheAssoc; i++)
      {
        if(dcache[dindex][i][1] == 0)
        {
          dcache[dindex][i][0] = dtag;
          dcache[dindex][i][1] = 1;
          for(j = 0; j < dcacheAssoc; j++)
          {
            if(dcache[dindex][j][2] > dcache[dindex][i][2])
            {
              dcache[dindex][j][2]--; 
            }
          }

          dcache[dindex][i][2] = (dcacheAssoc - 1);
          iinserted = 1;
          break;
        }
      }

      if(!iinserted)
      {
        for(i = 0; i < dcacheAssoc; i++)
        {
          if(dcache[dindex][i][2] == 0)
          {
            dcache[dindex][i][0] = dtag;
            dcache[dindex][i][1] = 1;
            dcache[dindex][i][2] = (dcacheAssoc - 1);
          }

          else
          {
            dcache[dindex][i][2]--;
          }
        }
      }
    }

    dcachePenalties = dcachePenalties + l2memspeed;
    return (dcacheHitTime + l2memspeed);
  }

  return memspeed;
}

// Perform a memory access to the l2cache for the address 'addr'
// Return the access time for the memory operation
//
uint32_t l2cache_access(uint32_t addr)
{
  //
  //TODO: Implement L2$
  //

  l2cacheRefs++;

  l2miss = 0;
  l2hit = 0;

  uint32_t l2offsetbits = log_block(blocksize);
  uint32_t l2indexbits = log_block(l2cacheSets);
  uint32_t l2tagbits = (32 - l2offsetbits - l2indexbits);

  uint32_t l2offsetmask = (1 << l2offsetbits) - 1;
  uint32_t l2indexmask = (1 << l2indexbits) - 1;
  uint32_t l2tagmask = (1 << l2tagbits) - 1;

  l2offset = addr & l2offsetmask;
  addr = addr >> l2offsetbits;
  
  l2index = addr & l2indexmask;
  addr = addr >> l2indexbits;
  
  l2tag = addr & l2tagmask;

  int i;
  int j;
  int plru;
  int nlru;

  for(i = 0; i < l2cacheAssoc; i++)
  {
    if((l2cache[l2index][i][0] == l2tag) && (l2cache[l2index][i][1] == 1))
    {
      l2hit = 1;
      l2miss = 0;

      for(j = 0; j < l2cacheAssoc; j++)
      {
        if(l2cache[l2index][j][2] > l2cache[l2index][i][2])
        {
          l2cache[l2index][j][2]--;
        }
      }

      l2cache[l2index][i][2] = (l2cacheAssoc - 1);
      break;
    }
  }

  if(l2hit)
  {
     return l2cacheHitTime;
  }

  else
  {
    l2hit = 0;
    l2miss = 1;

    _Bool l2invalid = 0;

    l2cacheMisses++;

    for(i = 0; i < l2cacheAssoc; i++)
    {
      if(l2cache[l2index][i][1] == 0)
      {
        l2cache[l2index][i][0] = l2tag;
        l2cache[l2index][i][1] = 1;
        // if(inclusive)
        // {
        //   l2cache[l2index][i][3] = 1; 
        // }

        for(j = 0; j < l2cacheAssoc; j++)
        {
          if(l2cache[l2index][j][2] > l2cache[l2index][i][2])
          {
            l2cache[l2index][j][2]--;
          }
        }

        l2cache[l2index][i][2] = (l2cacheAssoc - 1);
        l2invalid = 1;
        
        if(imiss)
          l2cache[l2index][i][3] = 1; // (1 for icache)
        else if(dmiss)
          l2cache[l2index][i][3] = 2; // (2 for dcache)

        break;
      }
    }

    if(!(l2invalid))
    {
      for(i = 0; i < l2cacheAssoc; i++)
      {
        if(l2cache[l2index][i][2] == 0)
        {
          // Checking for inclusive 

          //if(l2cache[l2index][i][3] == 1)           // Check for inclusive bit

          if(inclusive)
          {
            inclusiveAddr = 0;
            inclusiveAddr = (((l2cache[l2index][i][0] << l2indexbits) + l2index) << l2offsetbits);
            inclusiveSrc = l2cache[l2index][i][3];
          }


          // Adding new value at miss 

          // if(inclusive)
          // {
          //   l2cache[l2index][i][3] = 1;
          // }

          l2cache[l2index][i][0] = l2tag;
          l2cache[l2index][i][1] = 1;
          l2cache[l2index][i][2] = (l2cacheAssoc - 1);
          if(imiss)
            l2cache[l2index][i][3] = 1; // (1 for icache)
          else if(dmiss)
            l2cache[l2index][i][3] = 2; // (2 for dcache)
        }

        else
        { 
          l2cache[l2index][i][2]--;
        }
      }
    }

    l2cachePenalties = l2cachePenalties + memspeed;
    return (l2cacheHitTime + memspeed);
  }

  return memspeed;
}
