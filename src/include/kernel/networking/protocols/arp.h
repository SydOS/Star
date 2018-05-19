typedef struct {
	uint16_t HardwareType;
	uint16_t ProtocolType;
	uint8_t HardwareSize;
	uint16_t ProtocolSize;
	uint16_t Opcode;
	uint8_t SenderMAC[6];
	uint8_t SenderIP[4];
	uint8_t TargetMAC[6];
	uint8_t TargetIP[4];
} __attribute__((packed)) arp_frame_t;

extern arp_frame_t* arp_request(uint8_t* SenderMAC, uint8_t* TargetIP);