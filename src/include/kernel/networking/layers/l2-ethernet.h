typedef struct {
	uint8_t MACDest[6];
	uint8_t MACSrc[6];
	uint16_t Ethertype;
	// The payload comes after, but can be variable length, so just allocate
	// the memory size to be bigger than this actual struct to fit it.
} __attribute__((packed)) ethernet_frame_t;

ethernet_frame_t* l2_ethernet_create_frame(uint8_t* MACDest, uint8_t* MACSrc, 
							  uint16_t Ethertype, uint16_t payloadSize, 
							  void* payloadPointer,  uint16_t *FrameSize);