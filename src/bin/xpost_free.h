/*
 * Xpost - a Level-2 Postscript interpreter
 * Copyright (C) 2013, Michael Joshua Ryan
 * Copyright (C) 2013, Thorsten Behrens
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * - Neither the name of the Xpost software product nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef XPOST_FREE_H
#define XPOST_FREE_H

/**
 * @brief  initialize the FREE special entity which points
 *         to the head of the free list
 */
void xpost_free_init(Xpost_Memory_File *mem);

/**
 * @brief  print a dump of the free list
 */
void xpost_free_dump(Xpost_Memory_File *mem);

/**
 * @brief  allocate data, re-using garbage if possible
 */
unsigned xpost_free_alloc(Xpost_Memory_File *mem, unsigned sz, unsigned tag);

/**
 * @brief  explicitly add ent to free list
 */
unsigned xpost_free_memory_ent(Xpost_Memory_File *mem, unsigned ent);

/**
 * @brief reallocate data, preserving original contents
 
 * Use the free-list and tables to now provide a realloc for 
 * "raw" vm addresses (mem->base offsets rather than ents).
 * Assumes new size is larger than old size.
  
 * Allocate new entry, copy data, steal its adr, stash old adr, free it.
 */
unsigned xpost_free_realloc(Xpost_Memory_File *mem, unsigned oldadr, unsigned oldsize, unsigned newsize);

#endif
