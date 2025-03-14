/*
 * MIT License
 *
 * Copyright (c) 2020 Joey Castillo
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
#ifndef _WATCH_UART_H_INCLUDED
#define _WATCH_UART_H_INCLUDED
////< @file watch_uart.h

#include <stddef.h>
#include "watch.h"

/** @addtogroup uart UART
  * @brief This section covers functions related to the UART peripheral.
  **/
/// @{

/** @brief Initializes the UART.
  * @param tx_pin The pin the watch will use to transmit, or 0 for a receive-only UART.
  *               If specified, must be either HAL_GPIO_A2_pin() or HAL_GPIO_A4_pin().
  * @param rx_pin The pin the watch will use to receive, or 0 for a transmit-only UART.
  *               If specified, must be A1, A2, A3 or A4 (pin A0 cannot receive UART data).
  * @param baud The baud rate for the UART. A typical value is 19200.
  */
void watch_enable_uart(const uint16_t tx_pin, const uint16_t rx_pin, uint32_t baud);

/** @brief Transmits a string of bytes on the UART's TX pin.
  * @param s A null-terminated string containing the bytes you wish to transmit.
  */
void watch_uart_puts(char *s);

/** @brief Returns a string of bytes received on the UART's RX pin.
  * @param data A pointer to a buffer where the received bytes will be stored.
  * @param max_length The maximum number of bytes to receive.
  * @return The number of bytes actually received into the buffer.
  */
size_t watch_uart_gets(char *data, size_t max_length);

/// @}
#endif
