/*
 * commandParser.c
 *
 *  Created on: Mar 12, 2023
 *      Author: Tom Fry
 */

/*			Includes			*/
#include "commandParser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>

/*			Types			*/
typedef struct CommandMap_t
{
    char* cmdName;
    void (*funcPtr)(char**);
} CommandMap_t;

/*			Global Variables			*/
// Buffers
CommandMap_t g_cmdMap[MAX_NUM_COMMANDS];

// Variables
uint32_t g_numCmds = 0;

/*			Function Prototypes			*/
static bool Process_Command(SerialBuffer_t *SerialBuffer);
static void Clear_Buffer(SerialBuffer_t *SerialBuffer);

/*			Functions			*/
static void Init_Command_Parser (SerialBuffer_t *SerialBuffer)
{
	Clear_Buffer(SerialBuffer);

	// Clear the command map
	for (int mapIndex = 0; mapIndex < MAX_NUM_COMMANDS; ++mapIndex)
	{
		g_cmdMap[mapIndex].cmdName = NULL;
		g_cmdMap[mapIndex].funcPtr = NULL;
	}
	// Clear the command counter
	g_numCmds = 0;
}

void Init_Command_Parser_IT(UART_HandleTypeDef *Uart, SerialBuffer_t *SerialBuffer)
{
	Init_Command_Parser(SerialBuffer);

	HAL_UART_Receive_IT(Uart, (uint8_t*)&SerialBuffer->charBuf[0], 1);
}

void Add_Command (char* CmdName, void *FuncPtr)
{
	g_cmdMap[g_numCmds].cmdName = CmdName;
	g_cmdMap[g_numCmds].funcPtr = FuncPtr;
	++g_numCmds;
}

void Read_Buffer (SerialBuffer_t *SerialBuffer)
{
	bool commandReady = false;
	bool commandFound = false;

	int bufIndex = 0;
	for (bufIndex = 0; bufIndex < SERIAL_BUFFER_LENGTH; ++bufIndex)
	{
		if (('\n' == SerialBuffer->charBuf[bufIndex]) || ('\r' == SerialBuffer->charBuf[bufIndex]))
		{
			SerialBuffer->charBuf[bufIndex] = '\0';
			SerialBuffer->tail = bufIndex;

			commandReady = true;
			commandFound = Process_Command(SerialBuffer);
			break;
		}
	}

	if (true == commandReady)
	{
		if (false == commandFound)
		{
			// Handle invalid command here
			Clear_Buffer(SerialBuffer);
		}
	}
}

static bool Process_Command (SerialBuffer_t *SerialBuffer)
{
	bool commandFound = false;
    char* token = strtok(SerialBuffer->charBuf, " ");
    char* args[MAX_NUM_ARGS];
    uint32_t argCount = 0;

    while ((NULL != token) && (MAX_NUM_ARGS > argCount))
    {
        args[argCount++] = token;
        token = strtok(NULL, " ");
    }

    // map the command string to a command enum using the command_map array
    for (int cmdIndex = 0; cmdIndex < g_numCmds; ++cmdIndex)
    {
        if (0 == strcmp(args[0], g_cmdMap[cmdIndex].cmdName))
        {
            // call the appropriate function with the parsed arguments
            g_cmdMap[cmdIndex].funcPtr(&args[1]);
            commandFound = true;
            break;
        }
    }

    Clear_Buffer(SerialBuffer);
    return (commandFound);
}

void UART_IT_ISR_Callback (UART_HandleTypeDef *Uart, SerialBuffer_t *SerialBuffer)
{
	if (&Uart->pRxBuffPtr[0] != (uint8_t*)&SerialBuffer->charBuf[(SerialBuffer->tail + 1)])
	{
		SerialBuffer->charBuf[SerialBuffer->tail] = *(Uart->pRxBuffPtr - 1);
	}

	++SerialBuffer->tail;
	HAL_UART_Receive_IT(Uart, (uint8_t*)&SerialBuffer->charBuf[SerialBuffer->tail], 1);
}

static void Clear_Buffer (SerialBuffer_t *SerialBuffer)
{
	// Set the buffer to the NULL termination
	for (int bufIndex = 0; bufIndex < SERIAL_BUFFER_LENGTH; ++bufIndex)
	{
		SerialBuffer->charBuf[bufIndex] = '\0';
	}
	// Set the buffer head and tail to zero
	SerialBuffer->tail = 0;
}
