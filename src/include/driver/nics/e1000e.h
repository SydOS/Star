/*
 * File: e1000e.h
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

#ifndef E1000E_H
#define E1000E_H

#include <main.h>
#include <kernel/lock.h>
#include <driver/pci.h>

#define E1000E_VENDOR_ID                0x8086

#define E1000E_REG_CTRL         0x00000
#define E1000E_REG_STATUS       0x00008
#define E1000E_REG_STRAP        0x0000C
#define E1000E_REG_EEC          0x00010
#define E1000E_REG_EERD         0x00014
#define E1000E_REG_CTRL_EXT     0x00018
#define E1000E_REG_MDIC         0x00020
#define E1000E_REG_FEXTNVM      0x00028
#define E1000E_REG_FEXTNVM      0x0002C
#define E1000E_REG_BUSNUM       0x00038

#define E1000E_REG_FCTTV        0x00170
#define E1000E_REG_FCRTV        0x05F40

#define E1000E_REG_LEDCTL       0x00E00
#define E1000E_REG_EXTCNF_CTRL  0x00F00
#define E1000E_REG_EXTCNF_SIZE  0x00F08
#define E1000E_REG_PHY_CTRL     0x00F10
#define E1000E_REG_PCIANACFG    0x00F18

#define E1000E_REG_PBA          0x01000
#define E1000E_REG_PBS          0x01008

#define E1000E_REG_ICR          0x000C0
#define E1000E_REG_ITR          0x000C4
#define E1000E_REG_ICS          0x000C8
#define E1000E_REG_IMS          0x000D0
#define E1000E_REG_IMC          0x000D8
#define E1000E_REG_IAM          0x000E0
#define E1000E_REG_RCTL         0x00100
#define E1000E_REG_RCTL1        0x00104
#define E1000E_REG_ERT          0x02008
#define E1000E_REG_FCRTH        0x02168
#define E1000E_REG_PSRCTL       0x02170

#define E1000E_REG_RDBAL0       0x02800
#define E1000E_REG_RDBAH0       0x02804
#define E1000E_REG_RDLEN0       0x02808
#define E1000E_REG_RDH0         0x02810
#define E1000E_REG_RDT0         0x02818
#define E1000E_REG_RDTR         0x02820
#define E1000E_REG_RXDCTL       0x02828

#define E1000E_REG_RADV         0x0282C
#define E1000E_REG_RDBAL1       0x02900
#define E1000E_REG_RDBAH1       0x02904
#define E1000E_REG_RDLEN1       0x02908
#define E1000E_REG_RDH1         0x02910
#define E1000E_REG_RDT1         0x02918

#define E1000E_REG_RSRPD        0x02C00
#define E1000E_REG_RAID         0x02C08
#define E1000E_REG_CPUVEC       0x02C10
#define E1000E_REG_RXCSUM       0x05000
#define E1000E_REG_RFCTL        0x05008

#define E1000E_REG_TCTL         0x00400
#define E1000E_REG_TIPG         0x00410
#define E1000E_REG_AIT          0x00458
#define E1000E_REG_TDBAL0       0x03800
#define E1000E_REG_TDBAH0       0x03804
#define E1000E_REG_TDLEN0       0x03808
#define E1000E_REG_TDH0         0x03810
#define E1000E_REG_TDT0         0x03818

#define E1000E_REG_TARC0        0x03840
#define E1000E_REG_TIDV         0x03820
#define E1000E_REG_TXDCTL0      0x03828
#define E1000E_REG_TADV         0x0382C
#define E1000E_REG_TDBAL1       0x03900
#define E1000E_REG_TDBAH1       0x03904
#define E1000E_REG_TDLEN1       0x03908
#define E1000E_REG_TDH1         0x03910
#define E1000E_REG_TDT1         0x03918
#define E1000E_REG_TXDCTL1      0x03928
#define E1000E_REG_TARC1        0x03940

#define E1000E_REG_MTA          0x05200
#define E1000E_REG_RAL          0x05400
#define E1000E_REG_RAH          0x05404
#define E1000E_REG_MRQC         0x05818
#define E1000E_REG_RSSIM        0x05864
#define E1000E_REG_RSSIR        0x05868
#define E1000E_REG_RETA         0x05C00
#define E1000E_REG_RSSRK        0x05C80

#define E1000E_REG_WUC          0x05800
#define E1000E_REG_WUFC         0x05808
#define E1000E_REG_WUS          0x05810
#define E1000E_REG_IPAV         0x05838
#define E1000E_REG_IP4AT        0x05840
#define E1000E_REG_IP6AT        0x05880
#define E1000E_REG_H2ME         0x05B50
#define E1000E_REG_FFLT         0x05F00
#define E1000E_REG_FFMT         0x09000
#define E1000E_REG_FFVT         0x09800

#define E1000E_REG_FWSM         0x00010

#define E1000E_REG_RSYNCRXCTL   0x0B620
#define E1000E_REG_RXSTMPL      0x0B624
#define E1000E_REG_RXSTMPH      0x0B628
#define E1000E_REG_RXSATRL      0x0B62C
#define E1000E_REG_RXSATRH      0x0B630
#define E1000E_REG_RXCFGL       0x0B634
#define E1000E_REG_RXUDP        0x0B638

#define E1000E_REG_TSYNCTXCTL   0x0B614
#define E1000E_REG_TXSTMPL      0x0B618
#define E1000E_REG_TXSTMPH      0x0B61C
#define E1000E_REG_SYSTIML      0x0B600
#define E1000E_REG_SYSTIMH      0x0B604
#define E1000E_REG_TIMINCA      0x0B608
#define E1000E_REG_TIMADJL      0x0B60C
#define E1000E_REG_TIMADJH      0x0B610

#define E1000E_REG_LSECTXCAP    0x0B000
#define E1000E_REG_LSECRXCAP    0x0B300
#define E1000E_REG_LSECTXCTRL   0x0B004
#define E1000E_REG_LSECRXCTRL   0x0B304
#define E1000E_REG_LSECTXSCL    0x0B008
#define E1000E_REG_LSECTXSCH    0x0B00C
#define E1000E_REG_LSECTXSA     0x0B010
#define E1000E_REG_LSECTXPN0    0x0B018
#define E1000E_REG_LSECTXPN1    0x0B01C
#define E1000E_REG_LSECTXKEY0   0x0B020
#define E1000E_REG_LSECTXKEY1   0x0B030
#define E1000E_REG_LSECRXSCL    0x0B3D0
#define E1000E_REG_LSECRXSCH    0x0B3E0
#define E1000E_REG_LSECRXSA     0x0B310
#define E1000E_REG_LSECRXSAPN   0x0B330
#define E1000E_REG_LSECRXKEY    0x0B350

#define E1000E_REG_LSECTXUT     0x04300
#define E1000E_REG_LSECTXPKTE   0x04304
#define E1000E_REG_LSECTXPKTP   0x04308
#define E1000E_REG_LSECTXOCTE   0x0430C
#define E1000E_REG_LSECTXOCTP   0x04310

#define E1000E_REG_LSECRXUT     0x04314
#define E1000E_REG_LSECRXOCTE   0x0431C
#define E1000E_REG_LSECRXOCTP   0x04320
#define E1000E_REG_LSECRXBAD    0x04324
#define E1000E_REG_LSECRXNOSCI  0x04328
#define E1000E_REG_LSECRXUNSCI  0x0432C
#define E1000E_REG_LSECRXUNCH   0x04330
#define E1000E_REG_LSECRXDELAY  0x04340
#define E1000E_REG_LSECRXLATE   0x04350
#define E1000E_REG_LSECRXOK     0x04360


// Status bits.
#define E1000E_STATUS_FD        (1 << 0)  // Full-duplex.
#define E1000E_STATUS_LU        (1 << 1)  // Link established.
#define E1000E_STATUS_TXOFF     (1 << 4)  // Transmission Paused.
#define E1000E_STATUS_PHYPWR    (1 << 5)  // PHY power state.
#define E1000E_STATUS_MRCB      (1 << 8)  // Master Read Completions Blocked.
#define E1000E_STATUS_INIT_DONE (1 << 9)  // LAN Init Done.
#define E1000E_STATUS_PHYRA     (1 << 10) // PHY Reset Asserted.
#define E1000E_STATUS_MASTER_EN (1 << 19) // Master Enable Status.
#define E1000E_STATUS_CLK_CNT14 (1 << 31) // Clock Control 1/4.

#define E1000E_STATUS_SPEED_10      0
#define E1000E_STATUS_SPEED_100     (1 << 6)
#define E1000E_STATUS_SPEED_1000    (1 << 7)
#define E1000E_STATUS_SPEED_1000_2  ((1 << 6) | (1 << 7))

// Receive Control Register bits.
#define E1000E_RCTL_EN          (1 << 1)
#define E1000E_RCTL_SBP         (1 << 2)
#define E1000E_RCTL_UPE         (1 << 3)
#define E1000E_RCTL_MPE         (1 << 4)
#define E1000E_RCTL_LPE         (1 << 5)
#define E1000E_RCTL_BAM         (1 << 15)
#define E1000E_RCTL_PMCF        (1 << 23)
#define E1000E_RCTL_BSEX        (1 << 25)
#define E1000E_RCTL_SECRC       (1 << 26)

#define E1000E_RCTL_RDMTS_HALF      0
#define E1000E_RCTL_RDMTS_QUARTER   (1 << 8)
#define E1000E_RCTL_RDMTS_EIGHTH    (1 << 9)

#define E1000E_RCTL_DTYP_LEGACY     0
#define E1000E_RCTL_DTYP_SPLIT      (1 << 10)

#define E1000E_RCTL_BSIZE_2048      0
#define E1000E_RCTL_BSIZE_1024      (1 << 16)
#define E1000E_RCTL_BSIZE_512       (1 << 17)
#define E1000E_RCTL_BSIZE_256       ((1 << 16) | (1 << 17))

// Transmit Control Register bits.
#define E1000E_TCTL_EN          (1 << 1)  // Transmit Enable.
#define E1000E_TCTL_PSP         (1 << 3)  // Pad Short Packets.
#define E1000E_TCTL_CT_SHIFT    4         // Collision Threshold.
#define E1000E_TCTL_COLD_SHIFT  12        // Collision Distance.
#define E1000E_TCTL_SWXOFF      (1 << 22) // Software XOFF Transmission.
#define E1000E_TCTL_RTLC        (1 << 24) // Re-transmit on Late Collision.

// Interrupt bits.
#define E1000E_INT_TXDW         (1 << 0)  // Transmit Descriptor Written Back Interrupt.
#define E1000E_INT_TXQE         (1 << 1)  // Transmit Queue Empty Interrupt.
#define E1000E_INT_LSC          (1 << 2)  // Link Status Change Interrupt.
#define E1000E_INT_RXDMT0       (1 << 4)  // Receive Descriptor Minimum Threshold Hit Interrupt.
#define E1000E_INT_DSW          (1 << 5)  // Block software write accesses.
#define E1000E_INT_RXO          (1 << 6)  // Receiver Overrun Interrupt.
#define E1000E_INT_RXT0         (1 << 7)  // Receiver Timer Interrupt.
#define E1000E_INT_MDAC         (1 << 9)  // MDI/O Access Complete Interrupt.
#define E1000E_INT_PHYINT       (1 << 12) // PHY interrupt.
#define E1000E_INT_LSECPN       (1 << 14) // LinkSec packet number interrupt.
#define E1000E_INT_TXD_LOW      (1 << 15) // Transmit Descriptor Low Threshold Hit.
#define E1000E_INT_SRPD         (1 << 16) // Small Receive Packet Detect Interrupt.
#define E1000E_INT_ACK          (1 << 17) // Receive ACK Frame Detect Interrupt.
#define E1000E_INT_MNG          (1 << 18) // Manageability Event interrupt.
#define E1000E_INT_EPRST        (1 << 20) // ME reset event.
#define E1000E_INT_ASSERTED     (1 << 31) // Interrupt Asserted.




#define E1000E_PHY_READY                (1 << 28)


// Receive descriptor.
typedef struct {
    uint64_t BufferAddress;
    uint16_t Length;
    uint16_t Checksum;
    uint8_t Status;
    uint8_t Errors;

    // Used for storing info for 802.1q VLAN packets.
    uint16_t VlanTag;
} __attribute__((packed)) e1000e_receive_desc_t;

#define E1000E_RECEIVE_DESC_COUNT       64
#define E1000E_RECEIVE_DESC_POOL_SIZE   (E1000E_RECEIVE_DESC_COUNT * sizeof(e1000e_receive_desc_t))

// Transmit descriptor.
typedef struct {
    uint64_t BufferAddress;
    uint16_t Length;
    uint8_t ChecksumOffsetStart;
    uint8_t Command;
    uint8_t Status;
    uint8_t Reserved;
    uint16_t Special;
} __attribute__((packed)) e1000e_transmit_desc_t;

#define E1000E_TRANSMIT_DESC_COUNT       16
#define E1000E_TRANSMIT_DESC_POOL_SIZE   (E1000E_TRANSMIT_DESC_COUNT * sizeof(e1000e_transmit_desc_t))

#define E1000E_TRANSMIT_CMD_EOP     (1 << 0) // End Of Packet.
#define E1000E_TRANSMIT_CMD_IFCS    (1 << 1) // Insert FCS.
#define E1000E_TRANSMIT_CMD_IC      (1 << 2) // Insert Checksum.
#define E1000E_TRANSMIT_CMD_RS      (1 << 3) // Report Status.
#define E1000E_TRANSMIT_CMD_VLE     (1 << 6) // VLAN Packet Enable.
#define E1000E_TRANSMIT_CMD_IDE     (1 << 7) // Interrupt Delay Enable.

typedef struct {
    void *BasePointer;
    uint8_t MacAddress[6];

    uint64_t DescPage;
    void *DescPtr;

    e1000e_receive_desc_t *ReceiveDescs;
    e1000e_transmit_desc_t *TransmitDescs;

    void *TransmitBuffers[E1000E_TRANSMIT_DESC_COUNT];

    lock_t TransmitIndexLock;
    uint8_t CurrentTransmitDesc;
} e1000e_t;

extern bool e1000e_init(pci_device_t *pciDevice);

#endif