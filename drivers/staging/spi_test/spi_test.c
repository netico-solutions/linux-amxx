/*
 * main.c
 * This file is part of module_template
 *
 * Copyright (C) 2014 - Nenad Radulović
 *
 * module_template is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * module_template is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with module_template. If not, see <http://www.gnu.org/licenses/>.
 */
#include <linux/spi/spi.h>
#include <linux/printk.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/uaccess.h>

#define THIS_MODULE_NAME                "spi_test"
#define THIS_MODULE_AUTHOR              "Nenad Radulovic <nenad.b.radulovic@gmail.com"
#define THIS_MODULE_DESCRIPTION         "Generic Linux Kernel template"
#define THIS_MODULE_VERSION_MAJOR       "1"
#define THIS_MODULE_VERSION_MINOR       "0"
#define THIS_MODULE_VERSION             THIS_MODULE_VERSION_MAJOR "." THIS_MODULE_VERSION_MINOR

#define KML_OFF                             0

#define KML_ERR_LEVEL                       1
#define KML_WARN_LEVEL                      2
#define KML_NOTICE_LEVEL                    3
#define KML_INFO_LEVEL                      4
#define KML_DBG_LEVEL                       5

#define KML_GLOBAL_LEVEL                    KML_DBG_LEVEL  

#if (KML_GLOBAL_LEVEL >= KML_INFO_LEVEL)
#define KML_DBG(text, ...)                                                      \
    printk(KERN_DEBUG THIS_MODULE_NAME ": " text, ## __VA_ARGS__)
#else
#define KML_DBG()                           (void)0
#endif

#if (KML_GLOBAL_LEVEL >= KML_DBG_LEVEL)
#define KML_INFO(text, ...)                                                     \
    printk(KERN_INFO THIS_MODULE_NAME ": " text, ## __VA_ARGS__)
#else
#define KML_INFO()                          (void)0
#endif

#if (KML_GLOBAL_LEVEL >= KML_NOTICE_LEVEL)
#define KML_NOTICE(text, ...)                                                   \
    printk(KERN_NOTICE THIS_MODULE_NAME ": " text, ## __VA_ARGS__)
#else
#define KML_NOTICE()                        (void)0
#endif

#if (KML_GLOBAL_LEVEL >= KML_WARN_LEVEL)
#define KML_WARN(text, ...)                                                     \
    printk(KERN_WARNING THIS_MODULE_NAME ": " text, ## __VA_ARGS__)
#else
#define KML_WARN()                          (void)0
#endif

#if (KML_GLOBAL_LEVEL >= KML_ERR_LEVEL)
#define KML_ERR(text, ...)                                                      \
    printk(KERN_ERR THIS_MODULE_NAME ": " text, ## __VA_ARGS__)
#else
#define KML_ERR()                           (void)0
#endif

static int g_bus_id = 1;
module_param(g_bus_id, int, S_IRUGO);
MODULE_PARM_DESC(g_bus_id, "SPI bus ID");

static int g_packets_no = 16;
module_param(g_packets_no, int, S_IRUGO);
MODULE_PARM_DESC(g_packets_no, "Number of packets");

static int g_packets_size = 16;
module_param(g_packets_size, int, S_IRUGO);
MODULE_PARM_DESC(g_packets_size, "Size of packet");

static struct spi_device * g_spi;

static int __init module_initialize(void)
{
    int ret;
    int cnt;
    int packet_no;
    char * buffer_rx;
    char * buffer_tx;
    
    struct spi_master *     master;
    struct spi_board_info   kml_device_info = {
            .modalias     = THIS_MODULE_NAME,
            .max_speed_hz = 1000000ul,
            .bus_num      = g_bus_id,
    };
    KML_NOTICE("registering " THIS_MODULE_NAME " device driver\n");
    KML_NOTICE("BUILD: " __TIME__ "\n");

    master = spi_busnum_to_master(g_bus_id);

    if (!master) {
            KML_ERR("invalid SPI bus id: %d\n", g_bus_id);

            return (-ENODEV);
    }
    g_spi = spi_new_device(master, &kml_device_info);

    if (!g_spi) {
            KML_ERR("could not create SPI device\n");

            return (-ENODEV);
    }
    KML_NOTICE("packets: %d, packet size: %d", g_packets_no, g_packets_size);
    buffer_rx = kzalloc(g_packets_size, GFP_KERNEL);
    
    if (!buffer_rx) {
        KML_ERR("failed to create RX buffer, err: %d", -ENOMEM);
        return (-ENOMEM);
    }
    buffer_tx = kzalloc(g_packets_size, GFP_KERNEL);
    
    if (!buffer_tx) {
        KML_ERR("failed to create TX buffer, err: %d", -ENOMEM);
        kfree(buffer_rx);
        
        return (-ENOMEM);
    }    
    g_spi->bits_per_word = 8;
    g_spi->max_speed_hz  = 1000000u;
    
    ret = spi_setup(g_spi);
    
    if (ret) {
        KML_ERR("failed to setup SPI, err: %d", ret);
        kfree(buffer_rx);
        kfree(buffer_tx);
        return (ret);
    }
    
    for (packet_no = 0; packet_no < g_packets_no; packet_no++) {
        struct spi_transfer	t = {
			.tx_buf		= buffer_tx,
			.rx_buf     = buffer_rx,
			.len		= g_packets_size,
		};
	    struct spi_message	m;
	    
        KML_INFO("-- PACKET: %d --", packet_no);
        memset(buffer_tx, packet_no | 0xf0, g_packets_size);
        memset(buffer_rx, 0xde, g_packets_size);
        
        spi_message_init(&m);
    	spi_message_add_tail(&t, &m);
    	spi_sync(g_spi, &m);
        
        for (cnt = 0; cnt < g_packets_size / 8u; cnt++) {
            KML_INFO(" %d: %x %x %x %x %x %x %x %x\n",
                cnt,
                buffer_tx[(cnt * 8u) + 0u],
                buffer_tx[(cnt * 8u) + 1u],
                buffer_tx[(cnt * 8u) + 2u],
                buffer_tx[(cnt * 8u) + 3u],
                buffer_tx[(cnt * 8u) + 4u],
                buffer_tx[(cnt * 8u) + 5u],
                buffer_tx[(cnt * 8u) + 6u],
                buffer_tx[(cnt * 8u) + 7u]);
        }
        for (cnt = 0; cnt < g_packets_size / 8u; cnt++) {
            KML_INFO(" %d: %x %x %x %x %x %x %x %x\n",
                cnt,
                buffer_rx[(cnt * 8u) + 0u],
                buffer_rx[(cnt * 8u) + 1u],
                buffer_rx[(cnt * 8u) + 2u],
                buffer_rx[(cnt * 8u) + 3u],
                buffer_rx[(cnt * 8u) + 4u],
                buffer_rx[(cnt * 8u) + 5u],
                buffer_rx[(cnt * 8u) + 6u],
                buffer_rx[(cnt * 8u) + 7u]);
        }
        
        if (g_packets_size % 8u) {
            KML_INFO(" truncated  8");
        }
    }
    kfree(buffer_rx);
    kfree(buffer_tx);
    
    return (0);
    
}

static void __exit module_terminate(void)
{
    KML_NOTICE("Unloading module\n");
    spi_unregister_device(g_spi);
}

module_init(module_initialize);
module_exit(module_terminate);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(THIS_MODULE_AUTHOR);
MODULE_DESCRIPTION(THIS_MODULE_DESCRIPTION);



