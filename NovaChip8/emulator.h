class Chip8
{
private:
	unsigned short currentOpcode; //opcode is 2 bytes (4 hex) in CHIP-8
	unsigned char memory[4096]; //4096 1 byte memory locations (4k memory)
	unsigned char registers[16]; //16 1 byte registers
	unsigned short I; //I register for memory operations, 2 bytes
	unsigned short PC; //PC can have value from 0x000 to 0xFFF

	unsigned char delayTimer;
	unsigned char soundTimer;

	unsigned short stack[24]; //24 levels of nesting, 48 bytes total
	unsigned short SP;

	unsigned char fontset[80] =
	{
		0xF0, 0x90, 0x90, 0x90, 0xF0, //0
		0x20, 0x60, 0x20, 0x20, 0x70, //1
		0xF0, 0x10, 0xF0, 0x80, 0xF0, //2
		0xF0, 0x10, 0xF0, 0x10, 0xF0, //3
		0x90, 0x90, 0xF0, 0x10, 0x10, //4
		0xF0, 0x80, 0xF0, 0x10, 0xF0, //5
		0xF0, 0x80, 0xF0, 0x90, 0xF0, //6
		0xF0, 0x10, 0x20, 0x40, 0x40, //7
		0xF0, 0x90, 0xF0, 0x90, 0xF0, //8
		0xF0, 0x90, 0xF0, 0x10, 0xF0, //9
		0xF0, 0x90, 0xF0, 0x90, 0x90, //A
		0xE0, 0x90, 0xE0, 0x90, 0xE0, //B
		0xF0, 0x80, 0x80, 0x80, 0xF0, //C
		0xE0, 0x90, 0x90, 0x90, 0xE0, //D
		0xF0, 0x80, 0xF0, 0x80, 0xF0, //E
		0xF0, 0x80, 0xF0, 0x80, 0x80  //F
	};

	void ClearRegisters();
	void ClearMemory();
	void ClearDisplay();
	void ClearStack();
public:
	unsigned char display[64 * 32]; //pixel state, 1 white or 0 black 2048 pixels
	unsigned char key[16]; //key 0x0-0xF for hex based keypad
	bool drawFlag;

	void Initialize();
	bool LoadProgram(char** filePath);
	void Cycle();
};