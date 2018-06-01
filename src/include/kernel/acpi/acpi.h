/*
 * File: acpi.h
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

#ifndef ACPI_H
#define ACPI_H

#include <main.h>
#include <acpi.h>
//#include <kernel/acpi/acpi_tables.h>

#ifdef X86_64
#define ACPI_TEMP_ADDRESS   0xFFFFFF00FEF00000
#define ACPI_FIRST_ADDRESS  0xFFFFFF00FEF01000
#define ACPI_LAST_ADDRESS   0xFFFFFF00FEFFF000
#else
#define ACPI_TEMP_ADDRESS   0xFEF00000
#define ACPI_FIRST_ADDRESS  0xFEF01000
#define ACPI_LAST_ADDRESS   0xFEFFF000
#endif

extern bool acpi_supported();
extern ACPI_SUBTABLE_HEADER *acpi_search_madt(uint8_t type, uint32_t requiredLength, uintptr_t start);
extern bool acpi_change_pic_mode(uint32_t value);
extern void acpi_init();

#endif
