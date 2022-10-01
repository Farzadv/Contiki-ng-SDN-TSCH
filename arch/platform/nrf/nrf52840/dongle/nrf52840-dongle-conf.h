/*
 * Copyright (C) 2020 Yago Fontoura do Rosario <yago.rosario@hotmail.com.br>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*---------------------------------------------------------------------------*/
/**
 * \addtogroup nrf-platforms
 * @{
 *
 * \addtogroup nrf52840-dk
 * @{
 *
 * \file
 *         nRF52840 DK specific configuration.
 * \author
 *         Yago Fontoura do Rosario <yago.rosario@hotmail.com.br>
 */
/*---------------------------------------------------------------------------*/
#ifndef NRF52840_DK_CONF_H
#define NRF52840_DK_CONF_H
/*---------------------------------------------------------------------------*/
#define PLATFORM_HAS_UARTE              0
#define PLATFORM_HAS_BUTTON             1
#define PLATFORM_SUPPORTS_BUTTON_HAL    1
/*---------------------------------------------------------------------------*/
#define NRF_BUTTON1_PIN     16
#define NRF_BUTTON1_PORT    1
/*---------------------------------------------------------------------------*/
#define NRF_LED1_PIN        6
#define NRF_LED1_PORT       0
#define NRF_LED2_PIN        8
#define NRF_LED2_PORT       0
#define NRF_LED3_PIN        9
#define NRF_LED3_PORT       1
#define NRF_LED4_PIN        12
#define NRF_LED4_PORT       0
/*---------------------------------------------------------------------------*/
#define LEDS_CONF_COUNT     4
#define LEDS_CONF_RED       2
#define LEDS_CONF_GREEN     4
#define LEDS_CONF_BLUE      8
/*---------------------------------------------------------------------------*/
#endif /* NRF52840_DK_CONF_H */
/*---------------------------------------------------------------------------*/
/** 
 * @} 
 * @} 
 */
