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
#include <kernel/storage/storage.h>
#include <driver/fs/fat.h>

// The root VFS node.
vfs_node_t *RootVfsNode;

vfs_open_node_t *vfs_get_node(int32_t handle) {
    // Get current handle.
    process_t *currentProcess = tasking_process_get_current();
    if (handle < 0 || handle >= currentProcess->OpenFilesCount)
        return NULL;
    return currentProcess->OpenFiles[handle];
}

int32_t vfs_open(const char *path, int32_t flags) {

    // TODO: need to parse path, map node to handle, etc.

    kprintf("VFS: Opening %s with flags 0x%X...\n", path, flags);

    // Get handle.
    int32_t handle = tasking_process_get_file_handle();
    process_t *currentProcess = tasking_process_get_current();
    currentProcess->OpenFiles[handle] = (vfs_open_node_t*)kheap_alloc(sizeof(vfs_open_node_t));
    currentProcess->OpenFiles[handle]->Node = RootVfsNode;
    currentProcess->OpenFiles[handle]->CurrentPosition = 0;
    kprintf("VFS: Opened %s with handle %u!\n", path, handle);

    // Get filename.
    char *fileName = strrchr(path, '/') + 1;
    kprintf("VFS: Filename is %s\n", fileName);

    return handle;
}

int32_t vfs_get_dir_entries(uint32_t handle, vfs_dir_ent_t *directories, uint32_t count) {
    // Get current process.
    process_t *currentProcess = tasking_process_get_current();

    // Ensure handle is valid.
    if (handle < 0 || handle >= currentProcess->OpenFilesCount || currentProcess->OpenFiles[handle] == NULL)
        return -1; // Invalid handle.

    // Get node.
    vfs_node_t *node = currentProcess->OpenFiles[handle]->Node;

    // Get child nodes of /.
    vfs_node_t *dirNodes;
    uint32_t dirNodesCount = 0;
    node->GetDirNodes(node, &dirNodes, &dirNodesCount);

    // Copy nodes to buffer.
    uint32_t currentPosition = 0;
    for (uint32_t i = 0; i < dirNodesCount; i++) {
        // Get length of name.
        uint32_t nameLength = strlen(dirNodes[i].Name) + 1;
        uint32_t totalSize = nameLength + sizeof(vfs_dir_ent_t);

        // Make sure there is enough room. If not return the bytes written.
        if (currentPosition + totalSize > count)
            return currentPosition + 1;

        // Create entry.
        vfs_dir_ent_t *dirEntry = (vfs_dir_ent_t*)((uint8_t*)directories + currentPosition);
        dirEntry->NextOffset = i;
        dirEntry->Type = 0;
        dirEntry->Length = totalSize;
        strcpy(dirEntry->Name, dirNodes[i].Name);
        dirEntry->Name[nameLength] = '\0';
        currentPosition += totalSize;
    }

    // Return 0 to indicate end of directory.
    return 0;
}

void vfs_init(void) { // TODO: probably accept some sort of FS that is to be mounted as root.
    kprintf("VFS: Initializing...!\n");

    // Use the floppy as our / for now.
    fat_t *fatVolume = fat_init(storageDevices, PARTITION_NONE);

    RootVfsNode = (vfs_node_t*)kheap_alloc(sizeof(vfs_node_t));
    memset(RootVfsNode, 0, sizeof(vfs_node_t));
    RootVfsNode->Name = "/";
    RootVfsNode->FsObject = fatVolume;
    RootVfsNode->FsFileObject = fatVolume->RootDirectory;
    RootVfsNode->GetDirNodes = fat_vfs_get_dir_nodes;
    kprintf("VFS: Initialized root node at 0x%p!\n", RootVfsNode);

    int32_t rootDirHandle = vfs_open("/", 0);
   // int32_t df = vfs_open("/tmp/nou.txt", 0);

    // List our / test.

    uint8_t *buffer = (uint8_t*)kheap_alloc(512);


    int32_t result = vfs_get_dir_entries(rootDirHandle, buffer, 512);
    
    uint32_t current = 0;
    while (current < 512) {
        vfs_dir_ent_t *dirEntry = (vfs_dir_ent_t*)(buffer + current);

        // If entry is zero, we reached the end.
        if (dirEntry->Length == 0)
            break;

        kprintf("%s\n", dirEntry->Name);
        current += dirEntry->Length;
    }

    

    kprintf("VFS: Initialized!\n");
}
