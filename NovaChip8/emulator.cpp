/*
CHIP-8 VM
4096 (0x1000) memory locations, each one is 8 bits (1 byte)
CHIP-8 interpreter occupies first 512 bytes of memory (0x000-0x200)
	Most programs begin at memory location 512 (0x200) and don't access memory below that
Uppermost 256 bytes (0xF00-0xFFF) are for display refresh
96 bytes below that (0xEA0-0xEFF) are for call stack, internal use, other variables
16 8-bit data registers V0 to VF, VF is also a flag and should be avoided
	In addition VF is carry flag, subtraction VF is no borrow flag, draw is pixel collision
Address register I is 16 bits and used with several opcodes that involve memory operations

Stack is used to store return addresses for function calls, original version allocated 48 bytes for 24 levels of nesting

Delay Timer- count down at 60 hertz until reach 0, used for timing events in games, can be set and read
Sount timer- count down at 60 hertz until reach 0, used for sound effects, when value is nonzero a beeping sound is made

Input- Input is done with a hex keyboard that has 16 keys which range from 0 to F. 
The '8', '4', '6', and '2' keys are typically used for directional input. 
Three opcodes are used to detect input. 
	One skips an instruction if a specific key is pressed
	while another does the same if a specific key is not pressed
	The third waits for a key press, and then stores it in one of the data registers

Display resolution is 64x32 pixels, monochrome
Graphics are drawn by drawing sprites 8 pixels wide and 1 to 15 pixels in height
Sprite pixels are XORd with screen pixels, set ones flip color of screen pixel
carry flag VF is set to 1 if any screen pixels are flipped from set to unset when a sprite is drawn (for collision detection)

36 opcodes, all are two bytes long, stored in big-endian
*/

#include "emulator.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void Chip8::ClearRegisters()
{
	for (int i = 0; i < sizeof(registers); i++)
	{
		registers[i] = 0;
	}
}

void Chip8::ClearMemory()
{
	for (int i = 0; i < sizeof(memory); i++)
	{
		memory[i] = 0;
	}
}

void Chip8::ClearDisplay()
{
	for (int i = 0; i < sizeof(display); i++)
	{
		display[i] = 0;
	}
	drawFlag = true;
}

void Chip8::ClearStack()
{
	int stackIndicies = sizeof(stack) / sizeof(short);
	for (int i = 0; i < stackIndicies; i++)
	{
		stack[i] = 0;
	}
}

void Chip8::Initialize()
{
	printf("Initializing...\n");
	//clear system state
	currentOpcode = 0;
	I = 0;
	PC = 0x200; //application loaded at 0x200
	delayTimer = 0;
	soundTimer = 0;
	SP = 0;

	ClearRegisters();
	ClearMemory();
	ClearDisplay();
	ClearStack();
	
	printf("Loading fonts...\n");
	//load fonts
	for (int i = 0; i < 80; ++i)
	{
		memory[i] = fontset[i];
	}

	drawFlag = true; //need to draw a clear screen
	srand(time(NULL)); //initialize random number generator with current time as seed
	printf("Done..\n");
}

bool Chip8::LoadProgram(char** filePath)
{
	FILE* programFile;
	long fileSize;
	long availableSpace = 4096 - 512; //total memory minus the start location for a program

	printf("Loading program: %s...\n", *filePath);
	fopen_s(&programFile, *filePath, "rb");
	if (programFile == NULL)
	{
		fputs("Error reading file.", stderr);
		return false;
	}
	
	//get file size
	fseek(programFile, 0, SEEK_END);
	fileSize = ftell(programFile);
	rewind(programFile);

	//allocate temporary memory for program
	char* buffer = (char*)malloc(sizeof(char) * fileSize);
	if (buffer == NULL)
	{
		fputs("Error allocating memory.", stderr);
		return false;
	}

	//copy file into buffer
	size_t result = fread(buffer, 1, fileSize, programFile);
	if (result != fileSize)
	{
		fputs("Error reading file.", stderr);
		return false;
	}

	//copy buffer into CHIP-8 memory
	if (fileSize > availableSpace)
	{
		printf("Error: ROM too big for memory.");
		return false;
	}
	for (int i = 0; i < fileSize; ++i)
	{
		memory[i + 512] = buffer[i];
	}
	fclose(programFile);
	free(buffer);
	return true;
}

void Chip8::Cycle()
{
	unsigned char reg;
	unsigned char reg2;
	unsigned char constA;
	unsigned short address;
	//FETCH:
	//opcode is 2 bytes, so need to fetch the byte at the PC and the byte after it and combine it
	currentOpcode = memory[PC] << 8 | memory[PC + 1];

	//DECODE, EXECUTE, MEMORY, WRITE BACK, PC UPDATE:
	switch (currentOpcode & 0xF000) //see first 4 bits to determine type
	{
		case 0x0000:
			switch (currentOpcode & 0x00FF)
			{
				case 0x00E0: //00E0 clears display
					ClearDisplay();
					PC += 2;
					break;
				case 0x00EE: //00EE returns
					--SP;
					PC = stack[SP];
					PC += 2; //next instruction after the call
					break;
				default: //0NNN Calls RCA 1802 program at address NNN
					stack[SP] = PC; //store PC in stack
					++SP; //update stack pointer to new free stack location
					PC = currentOpcode & 0x0FFF;
					break;
			}
			break;
		case 0x1000: //(1NNN) jump to address NNN
			PC = currentOpcode & 0x0FFF;
			break;
		case 0x2000: //(2NNN) call subroutine at NNN
			stack[SP] = PC; //store PC in stack
			++SP; //update stack pointer to new free stack location
			PC = currentOpcode & 0x0FFF;
			break;
		case 0x3000: // (3XNN) skip next if rX == NN
			reg = (currentOpcode & 0x0F00 ) >> 8; //move 2 hex and each hex is 4 bits
			constA = currentOpcode & 0x00FF;

			if (registers[reg] == constA)
			{
				PC += 2; //skip additional instruction
			}
			PC += 2;
			break;
		case 0x4000: // (4XNN) skip next if rX != NN
			reg = (currentOpcode & 0x0F00) >> 8; //move 2 hex and each hex is 4 bits
			constA = currentOpcode & 0x00FF;

			if (registers[reg] != constA)
			{
				PC += 2; //skip additional instruction
			}
			PC += 2;
			break;
		case 0x5000: // (5XY0) skip next if rX == rY
			reg = (currentOpcode & 0x0F00) >> 8; //move 2 hex and each hex is 4 bits
			reg2 = (currentOpcode & 0x00F0) >> 4; //move 1 hex

			if (registers[reg] == registers[reg2])
			{
				PC += 2; //skip additional instruction
			}
			PC += 2;
			break;
		case 0x6000: //(6XNN) rX = constA
			reg = (currentOpcode & 0x0F00) >> 8;
			constA = (currentOpcode & 0x00FF);
			registers[reg] = constA;
			PC += 2;
			break;
		case 0x7000: //(7XNN) rX += NN
			reg = (currentOpcode & 0x0F00) >> 8;
			constA = currentOpcode & 0x00FF;
			registers[reg] += constA;
			PC += 2;
			break;
		case 0x8000:
			reg = (currentOpcode & 0x0F00) >> 8;
			reg2 = (currentOpcode & 0x00F0) >> 4;
			switch (currentOpcode & 0x000F)
			{
				case 0x0000: //(8XY0) rX = rY
					registers[reg] = registers[reg2];
					PC += 2;
					break;
				case 0x0001: //(8XY1) rX = rX | rY
					registers[reg] = registers[reg] | registers[reg2];
					PC += 2;
					break;
				case 0x0002: //(8XY2) rX = rX & rY
					registers[reg] = registers[reg] & registers[reg2];
					PC += 2;
					break;
				case 0x0003: //(8XY3) rX = rX ^ rY
					registers[reg] = registers[reg] ^ registers[reg2];
					PC += 2;
					break;
				case 0x0004: //(8XY4) rX += rY, set rF if carry occurs
					if (registers[reg] + (unsigned int)registers[reg2] > 255)
					{
						registers[15] = 1;
					}
					else
					{
						registers[15] = 0;
					}
					registers[reg] += registers[reg2];
					PC += 2;
					break;
				case 0x0005: //(8XY5) rX -= rY, set rF if NO borrow occurs
					if (registers[reg] > registers[reg2])
					{
						registers[15] = 1;
					}
					else
					{
						registers[15] = 0;
					}
					registers[reg] -= registers[reg2];
					PC += 2;
					break;
				case 0x0006: //(8XY6) rX = rY >> 1, set rF to least significant bit of rY before shift
					registers[15] = registers[reg2] & 0x01;
					registers[reg] = registers[reg2] >> 1;
					PC += 2;
					break;
				case 0x0007: //(8XY7) rX = rY - rX, set rF if NO borrow occurs
					if (registers[reg2] > registers[reg])
					{
						registers[15] = 1;
					}
					else
					{
						registers[15] = 0;
					}
					registers[reg] = registers[reg2] - registers[reg];
					PC += 2;
					break;
				case 0x000E: //(8XYE) rX = rY = rY << 1
					registers[reg] = registers[reg2] = registers[reg2] << 1;
					PC += 2;
					break;
			}
			break;
		case 0x9000: //(9XY0) skip next instruction if rX != rY
			reg = (currentOpcode & 0x0F00) >> 8;
			reg2 = (currentOpcode & 0x00F0) >> 4;
			if (registers[reg] != registers[reg2])
			{
				PC += 2; //skip additional instruction
			}
			PC += 2;
			break;
		case 0xA000: //(ANNN) Sets I to address NNN
			address = (currentOpcode & 0x0FFF);
			I = address;
			PC += 2;
			break;
		case 0xB000: //(BNNN) Jumps to address NNN + r0
			address = currentOpcode & 0x0FFF;
			PC = registers[0] + address;
			break;
		case 0xC000: //(CXNN) Sets rX = to rand(0, 255) & NN
			reg = (currentOpcode & 0x0F00) >> 8;
			constA = currentOpcode & 0x00FF;
			registers[reg] = (rand() % 255) & constA;
			PC += 2;
			break;
		case 0xD000:
		{//(DXYN) Draw at coordinate (rX, rY) with width of 8 pixels and height of N pixels
			//each row of 8 pixels is read starting from memory location I.
			//rF is set to 1 if any screen pixels are flipped from set to unset when sprite is drawn (collision)
			reg = (currentOpcode & 0x0F00) >> 8;
			reg2 = (currentOpcode & 0x00F0) >> 4;
			constA = currentOpcode & 0x000F;
			unsigned short pixel;

			registers[15] = 0;
			for (int y = 0; y < constA; y++)
			{
				pixel = memory[I + y];
				for (int x = 0; x < 8; x++)
				{
					if ((pixel & (0x80 >> x)) != 0)
					{
						if (display[(registers[reg] + x + ((registers[reg2] + y) * 64))] == 1)
						{
							registers[15] = 1;
						}
						display[registers[reg] + x + ((registers[reg2] + y) * 64)] ^= 1;
					}
				}
			}
			drawFlag = true;
			PC += 2;
		}
		break;
		case 0xE000:
			reg = (currentOpcode & 0x0F00) >> 8;
			switch (currentOpcode & 0x00FF)
			{
				case 0x009E: //(EX9E) Skips next instruction if key in vX is pressed
					if (key[registers[reg]] != 0)
					{
						PC += 2;
					}
					PC += 2;
					break;
				case 0x00A1: //(EXA1) Skips next instruction if key in vX is not pressed
					if (key[registers[reg]] == 0)
					{
						PC += 2;
					}
					PC += 2;
					break;
			}
			break;
		case 0xF000:
			reg = (currentOpcode & 0x0F00) >> 8;
			switch (currentOpcode & 0x00FF)
			{
				case 0x0007: //(FX07) rX = delay_timer
				{
					registers[reg] = delayTimer;
					PC += 2;
					break;
				}
				case 0x000A: //(FX0A) block, halting instructions while waiting for key press, then rX = key_pressed
				{
					bool keyPress = false;
					for (int i = 0; i < 16; ++i)
					{
						if (key[i] != 0)
						{
							registers[reg] = i;
							keyPress = true;
						}
					}
					if (keyPress == false) //if no key, restart the cycle
					{
						return; //don't increment PC so this instruction will be called again
					}
					PC += 2;
					break;
				}
				case 0x0015: //(FX15) delayTimer = rX
				{
					delayTimer = registers[reg];
					PC += 2;
					break;
				}
				case 0x0018: //(FX18) soundTimer = rX
				{
					soundTimer = registers[reg];
					PC += 2;
					break;
				}
				case 0x001E: //(FX1E) I += rX (rF is set to 1 when (I + VX > 0xFFF) (range overflow) and 0 when not, undocumented feature used by Spacefight 2091!)
				{
					if (I + registers[reg] > 0xFFF)
					{
						registers[15] = 1;
					}
					else
					{
						registers[15] = 0;
					}
					I += registers[reg];
					PC += 2;
					break;
				}
				case 0x0029: //(FX29) I = address for sprite for character in rX
				{
					I = registers[reg] * 0x5;
					PC += 2;
					break;
				}
				case 0x0033: //(FX33) stores binary-coded decimal of rX
				{
					//most signficant of three digits at I, middle digit in I+1, least digit in I+2
					memory[I] = registers[reg] / 100;
					memory[I + 1] = (registers[reg] / 10) % 10;
					memory[I + 2] = (registers[reg] % 100) % 10;
					PC += 2;
					break;
				}
				case 0x0055: //(FX55) stores r0 to rX in memory starting at address I
				{
					for (int i = 0; i <= reg; i++)
					{
						memory[I] = registers[i];
						I++;
					}
					PC += 2;
					break;
				}
				case 0x0065: //(FX65) Fills r0 to rX with values from memory starting at address I.
				{
					for (int i = 0; i <= reg; i++)
					{
						registers[i] = memory[I];
						I++;
					}
					PC += 2;
					break;
				}
			}
			break;
	}
	if (delayTimer > 0)
	{
		delayTimer--;
	}
	if (soundTimer > 0)
	{
		if (soundTimer == 1)
		{
			printf("BEEP!\n");
		}
		soundTimer--;
	}
	printf("%x\n", currentOpcode);
}