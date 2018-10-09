/*
 * File: vfs.h
 * 
 * Copyright (c) 2017-2018 Sydney Erickson, John Davis
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef VFS_H
#define VFS_H

#include <main.h>

// Directory entry.
typedef struct {
    uint64_t NextOffset;
    uint16_t Length;
    uint8_t Type;
    char Name[];
} __attribute__((packed)) vfs_dir_ent_t;

// File in VFS (node).
typedef struct {
    char *Name;
    uint16_t Flags;

    // Permission mask, user, group IDs.
    uint32_t PermMask;
    uint32_t UserId;
    uint32_t GroupId;

    // Length in bytes.
    uint64_t Length;

    // Underlying FS objects.
    void *FsObject;
    void *FsFileObject;

    // Pointers to FS-specific methods.
    void (*GetDirNodes)(struct vfs_node_t *fsNode, struct vfs_node_t **outDirNodes, uint32_t *outCount);
} vfs_node_t;

// Open file node.
typedef struct {
    vfs_node_t *Node;
    uint64_t CurrentPosition;
} vfs_open_node_t;

extern vfs_node_t *RootVfsNode;

extern int32_t vfs_open(const char *path, int32_t flags);
extern int32_t vfs_get_dir_entries(uint32_t handle, vfs_dir_ent_t *directories, uint32_t count);
extern void vfs_init(void);

#endif
