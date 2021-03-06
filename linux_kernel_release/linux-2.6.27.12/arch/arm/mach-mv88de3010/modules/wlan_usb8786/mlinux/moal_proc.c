/** @file moal_proc.c
  *
  * @brief This file contains functions for proc file.
  * 
  * Copyright (C) 2008-2009, Marvell International Ltd. 
  * All Rights Reserved
  */

/********************************************************
Change log:
    10/21/2008: initial version
********************************************************/

#include	"moal_main.h"

/********************************************************
		Local Variables
********************************************************/
#ifdef CONFIG_PROC_FS
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
#define PROC_DIR	NULL
#define MWLAN_PROC_DIR  "mwlan/"
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
#define PROC_DIR	&proc_root
#else
#define PROC_DIR	proc_net
#endif

static char *szModes[] = {
    "Ad-hoc",
    "Managed",
    "Auto",
    "Unknown"
};

/********************************************************
		Global Variables
********************************************************/

/********************************************************
		Local Functions
********************************************************/
/** 
 *  @brief Proc read function for info
 *
 *  @param page	   Pointer to buffer
 *  @param start   Read data starting position
 *  @param offset  Offset
 *  @param count   Counter 
 *  @param eof     End of file flag
 *  @param data    Data to output
 *
 *  @return 	   Number of output data
 */
static int
woal_info_proc_read(char *page, char **start, off_t offset,
                    int count, int *eof, void *data)
{
    int i;
    char *p = page;
    struct net_device *netdev = data;
    struct dev_mc_list *mcptr = netdev->mc_list;
    char fmt[64];
    moal_private *priv = (moal_private *) netdev_priv(netdev);
    moal_handle *handle = priv->phandle;
    mlan_bss_info info;

    if (offset) {
        *eof = 1;
        goto exit;
    }
    woal_get_version(handle, fmt, sizeof(fmt) - 1);
    memset(&info, 0, sizeof(info));
    if (MLAN_STATUS_SUCCESS != woal_get_bss_info(priv, MOAL_PROC_WAIT, &info)) {
        *eof = 1;
        goto exit;
    }

    p += sprintf(p, "driver_name = " "\"wlan\"\n");
    p += sprintf(p, "driver_version = %s", fmt);
    p += sprintf(p, "\ninterface_name=\"%s\"\n", netdev->name);
    p += sprintf(p, "bss_mode=\"%s\"\n", szModes[info.bss_mode]);
    p += sprintf(p, "media_state=\"%s\"\n",
                 ((priv->media_connected == MFALSE) ?
                  "Disconnected" : "Connected"));
    p += sprintf(p, "mac_address=\"%02x:%02x:%02x:%02x:%02x:%02x\"\n",
                 netdev->dev_addr[0], netdev->dev_addr[1],
                 netdev->dev_addr[2], netdev->dev_addr[3],
                 netdev->dev_addr[4], netdev->dev_addr[5]);

    p += sprintf(p, "multicast_count=\"%d\"\n", netdev->mc_count);
    p += sprintf(p, "essid=\"%s\"\n", info.ssid.ssid);
    p += sprintf(p, "bssid=\"%02x:%02x:%02x:%02x:%02x:%02x\"\n",
                 info.bssid[0], info.bssid[1],
                 info.bssid[2], info.bssid[3], info.bssid[4], info.bssid[5]);
    p += sprintf(p, "channel=\"%d\"\n", (int) info.bss_chan);
    p += sprintf(p, "region_code = \"%02x\"\n", (t_u8) info.region_code);

    /* 
     * Put out the multicast list 
     */
    for (i = 0; i < netdev->mc_count; i++) {
        p += sprintf(p,
                     "multicast_address[%d]=\"%02x:%02x:%02x:%02x:%02x:%02x\"\n",
                     i,
                     mcptr->dmi_addr[0], mcptr->dmi_addr[1],
                     mcptr->dmi_addr[2], mcptr->dmi_addr[3],
                     mcptr->dmi_addr[4], mcptr->dmi_addr[5]);

        mcptr = mcptr->next;
    }
    p += sprintf(p, "num_tx_bytes = %lu\n", priv->stats.tx_bytes);
    p += sprintf(p, "num_rx_bytes = %lu\n", priv->stats.rx_bytes);
    p += sprintf(p, "num_tx_pkts = %lu\n", priv->stats.tx_packets);
    p += sprintf(p, "num_rx_pkts = %lu\n", priv->stats.rx_packets);
    p += sprintf(p, "num_tx_pkts_dropped = %lu\n", priv->stats.tx_dropped);
    p += sprintf(p, "num_rx_pkts_dropped = %lu\n", priv->stats.rx_dropped);
    p += sprintf(p, "num_tx_pkts_err = %lu\n", priv->stats.tx_errors);
    p += sprintf(p, "num_rx_pkts_err = %lu\n", priv->stats.rx_errors);
    p += sprintf(p, "carrier %s\n",
                 ((netif_carrier_ok(priv->netdev)) ? "on" : "off"));
    p += sprintf(p, "tx queue %s\n",
                 ((netif_queue_stopped(priv->netdev)) ? "stopped" : "started"));
  exit:
    return (p - page);
}

/********************************************************
		Global Functions
********************************************************/
/** 
 *  @brief Create the top level proc directory
 *
 *  @param handle   Pointer to woal_handle
 *
 *  @return 	    N/A
 */
void
woal_proc_init(moal_handle * handle)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26)
    struct proc_dir_entry *pde = PROC_DIR;
#endif
    ENTER();
    PRINTM(INFO, "Create Proc Interface\n");
    if (!handle->proc_mwlan) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26)
        /* Check if directory already exists */
        for (pde = pde->subdir; pde; pde = pde->next) {
            if (pde->namelen && !strcmp("mwlan", pde->name)) {
                /* Directory exists */
                PRINTM(WARN, "proc interface already exists!\n");
                handle->proc_mwlan = pde;
                break;
            }
        }
        if (pde == NULL) {
            handle->proc_mwlan = proc_mkdir("mwlan", PROC_DIR);
            if (!handle->proc_mwlan)
                PRINTM(ERROR, "Cannot create proc interface!\n");
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)
            else
                atomic_set(&handle->proc_mwlan->count, 1);
#endif
        }
#endif
    }

    LEAVE();
}

/** 
 *  @brief Remove the top level proc directory
 *
 *  @param handle  pointer moal_handle
 *
 *  @return 	   N/A
 */
void
woal_proc_exit(moal_handle * handle)
{
    ENTER();

    PRINTM(INFO, "Remove Proc Interface\n");
    if (handle->proc_mwlan) {
        /* Remove only if we are the only instance using this */
        if (atomic_read(&(handle->proc_mwlan->count)) > 1) {
            PRINTM(WARN, "More than one interface using proc!\n");
        } else {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)
            atomic_dec(&(handle->proc_mwlan->count));
#endif
            remove_proc_entry("mwlan", PROC_DIR);
            handle->proc_mwlan = NULL;
        }
    }

    LEAVE();
}

/** 
 *  @brief Create proc file for interface
 *
 *  @param priv	   pointer moal_private
 *
 *  @return 	   N/A
 */
void
woal_create_proc_entry(moal_private * priv)
{
    struct net_device *dev = priv->netdev;
    struct proc_dir_entry *r;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
    char proc_dir_name[20];
#endif
    ENTER();
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
    if (!priv->proc_entry) {
        memset(proc_dir_name, 0, sizeof(proc_dir_name));
        strcpy(proc_dir_name, MWLAN_PROC_DIR);
        strcat(proc_dir_name, dev->name);
        /* Try to create mwlan/mlanX first */
        priv->proc_entry = proc_mkdir(proc_dir_name, PROC_DIR);
        if (priv->proc_entry) {
            /* Success. Continue normally */
            if (!priv->phandle->proc_mwlan) {
                priv->phandle->proc_mwlan = priv->proc_entry->parent;
            }
            atomic_inc(&(priv->phandle->proc_mwlan->count));
        } else {
            /* Failure. mwlan may not exist. Try to create that first */
            priv->phandle->proc_mwlan = proc_mkdir("mwlan", PROC_DIR);
            if (!priv->phandle->proc_mwlan) {
                /* Failure. Something broken */
                LEAVE();
                return;
            } else {
                /* Success. Now retry creating mlanX */
                priv->proc_entry = proc_mkdir(proc_dir_name, PROC_DIR);
                atomic_inc(&(priv->phandle->proc_mwlan->count));
            }
        }
#else
    if (priv->phandle->proc_mwlan && !priv->proc_entry) {
        priv->proc_entry = proc_mkdir(dev->name, priv->phandle->proc_mwlan);
        atomic_inc(&(priv->phandle->proc_mwlan->count));
#endif
        strcpy(priv->proc_entry_name, dev->name);
        if (priv->proc_entry) {
            r = create_proc_read_entry("info", 0, priv->proc_entry,
                                       woal_info_proc_read, dev);
        }
    }
    LEAVE();
}

/** 
 *  @brief Remove proc file
 *
 *  @param priv	   Pointer moal_private
 *
 *  @return 	   N/A
 */
void
woal_proc_remove(moal_private * priv)
{
    ENTER();
    if (priv->phandle->proc_mwlan && priv->proc_entry) {
        remove_proc_entry("info", priv->proc_entry);
        remove_proc_entry(priv->proc_entry_name, priv->phandle->proc_mwlan);
        atomic_dec(&(priv->phandle->proc_mwlan->count));
        priv->proc_entry = NULL;
    }
    LEAVE();
}
#endif
