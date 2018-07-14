/*
 * File: vfs.c
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

#include <main.h>
#include <kprint.h>
#include <string.h>
#include <kernel/vfs/vfs.h>
#include <kernel/tasking.h>

#include <kernel/memory/kheap.h>

// The root VFS node.
vfs_node_t *RootVfsNode;

int32_t vfs_open(const char *path, int32_t flags) {
    kprintf("VFS: Opening %s with flags 0x%X...\n", path, flags);

    // Get handle.
    int32_t handle = tasking_process_get_file_handle();
    kprintf("VFS: Opened %s with handle %u!\n", path, handle);

    return handle;
}

void vfs_init(void) { // TODO: probably accept some sort of FS that is to be mounted as root.
    kprintf("VFS: Initializing...!\n");
    RootVfsNode = (vfs_node_t*)kheap_alloc(sizeof(vfs_node_t));
    memset(RootVfsNode, 0, sizeof(vfs_node_t));
    RootVfsNode->Name[0] = '/';

    int32_t dd = vfs_open("/", 0);
    int32_t df = vfs_open("/nou.txt", 0);

    kprintf("VFS: Initialized root node at 0x%p!\n", RootVfsNode);
}
