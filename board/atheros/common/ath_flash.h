/*
 * Copyright (c) 2013 Qualcomm Atheros, Inc.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef _ATH_FLASH_H
#define _ATH_FLASH_H

#define display(_x)

/*
 * primitives
 */

#define ath_be_msb(_val, _i) (((_val) & (1 << (7 - _i))) >> (7 - _i))

#define ath_spi_bit_banger(_byte)   do {                \
    int i;                              \
    for(i = 0; i < 8; i++) {                    \
        ath_reg_wr_nf(ATH_SPI_WRITE,                \
            ATH_SPI_CE_LOW | ath_be_msb(_byte, i));     \
        ath_reg_wr_nf(ATH_SPI_WRITE,                \
            ATH_SPI_CE_HIGH | ath_be_msb(_byte, i));    \
    }                               \
} while (0)

#define ath_spi_go()    do {                \
    ath_reg_wr_nf(ATH_SPI_WRITE, ATH_SPI_CE_LOW);   \
    ath_reg_wr_nf(ATH_SPI_WRITE, ATH_SPI_CS_DIS);   \
} while (0)


#define ath_spi_send_addr(__a) do {         \
    ath_spi_bit_banger(((__a & 0xff0000) >> 16));   \
    ath_spi_bit_banger(((__a & 0x00ff00) >> 8));    \
    ath_spi_bit_banger(__a & 0x0000ff);     \
} while (0)

#define ath_spi_delay_8()   ath_spi_bit_banger(0)
#define ath_spi_done()      ath_reg_wr_nf(ATH_SPI_FS, 0)

extern unsigned long flash_get_geom (flash_info_t *flash_info);

#ifdef FW_RECOVERY

#define FW_BUTTON_GPIO      (FW_RECOVERY_INPUT_BUTTON_GPIO)
#define FW_LED_GPIO         (FW_RECOVERY_OUTPUT_LED_GPIO)
#define FW_LED_ON           (FW_RECOVERY_OUTPUT_LED_ON)
#define FW_LED_OFF          (!FW_LED_ON)

void    ath_auf_gpio_init(void);
int     ath_is_rst_btn_pressed(void);
void    ath_fw_led_on(void);
void    ath_fw_led_off(void);
#endif

#endif /* _ATH_FLASH_H */
