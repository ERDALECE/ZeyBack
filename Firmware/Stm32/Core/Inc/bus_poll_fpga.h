/*
 * bus_poll_fpga.h
 *
 * STM32H750 → Artix-7 FMC 32-bit polling audio transfer
 * Supports PCM (I2S), Native DSD and DoP modes
 */

#ifndef BUS_POLL_FPGA_H_
#define BUS_POLL_FPGA_H_

#include <stdint.h>
#include "stm32h7xx_ll_gpio.h"
#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 * FMC Address Configuration
 *============================================================================*/
/* FMC Bank1 NE1 base address */
#define FPGA_FMC_BASE    0x60000000UL

/* Register addresses */
#define FPGA_DATA_REG    (*((volatile uint32_t *)(FPGA_FMC_BASE + 0x00UL)))
#define FPGA_CTRL_REG    (*((volatile uint32_t *)(FPGA_FMC_BASE + 0x04UL)))

/*
 * FPGA Status Register bit definitions (FPGA_CTRL_REG read):
 *   bit[0] = stream_en_s  : stream aktif
 *   bit[1] = dsd_mode_s   : DSD modu
 *   bit[4] = fifo_empty   : FIFO bos
 *   bit[5] = fifo_full    : FIFO dolu
 *   bit[6] = fifo_prog_full  : FIFO prog_full (NWAIT esigi)  ← KRITIK
 *   bit[7] = fifo_prog_empty : FIFO prog_empty
 */
#define FPGA_ST_STREAM_EN    (1u << 0)
#define FPGA_ST_DSD_MODE     (1u << 1)
#define FPGA_ST_FIFO_EMPTY   (1u << 4)
#define FPGA_ST_FIFO_FULL    (1u << 5)
#define FPGA_ST_PROG_FULL    (1u << 6)
#define FPGA_ST_PROG_EMPTY   (1u << 7)

/*============================================================================
 * GPIO Configuration
 *============================================================================*/
/* EN_STREAM GPIO - FPGA stream enable */
#define EN_STREAM_GPIO   GPIOC
#define EN_STREAM_PIN    LL_GPIO_PIN_8

/* DSD_MODE GPIO - FPGA DSD/PCM mode select */
/* TODO: Configure these to match your PCB */
#define DSD_MODE_GPIO    GPIOD           /* Change to actual port */
#define DSD_MODE_PIN     LL_GPIO_PIN_3   /* Change to actual pin */

/*============================================================================
 * Statistics Structure
 *============================================================================*/
typedef struct {
    uint32_t frames_sent;
    uint32_t underruns;
    uint32_t overflows;
    uint32_t resync_count;
} fpga_stats_t;

/*============================================================================
 * Public API
 *============================================================================*/

/* Initialize bus polling module */
void bus_poll_init(void);

/* Start/stop polling */
void bus_poll_start(void);
void bus_poll_stop(void);

/* Set FPGA stream enable (flush on rising edge) */
void fpga_set_stream_en(uint8_t en);

/* Set FPGA DSD/PCM mode (1=DSD, 0=PCM).
 * ONEMLI: bus_poll_init()'ten SONRA cagrılmalidir. */
void fpga_set_dsd_mode(uint8_t dsd_on);

/* Set DoP marker stripping mode (1=DoP aktif, 0=Native DSD veya PCM).
 * DoP modunda bus_poll_service() her sample'in MSB byte'ini maskeleyerek
 * FPGA'ya sadece 24-bit DSD datasi gonderir. */
void fpga_set_dop_mode(uint8_t dop_on);

/* Check if currently in DSD mode */
uint8_t fpga_is_dsd_mode(void);

/* Check if currently in DoP mode */
uint8_t fpga_is_dop_mode(void);

/* Read FPGA status register */
uint32_t fpga_read_status(void);

/* Send resync command to FPGA */
void fpga_send_resync(void);

/* Get/reset statistics */
fpga_stats_t fpga_get_stats(void);
void fpga_reset_stats(void);

/* Main polling service - call from main loop */
void bus_poll_service(uint32_t max_frames);

#ifdef __cplusplus
}
#endif

#endif /* BUS_POLL_FPGA_H_ */
