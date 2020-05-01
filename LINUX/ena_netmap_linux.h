/*
 * Copyright (C) 2020 Amit Shinde. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * netmap support for: ena (linux version)
 */

#include <bsd_glue.h>
#include <net/netmap.h>
#include <netmap/netmap_kern.h>

/* restore kernel and user view of transmit ring. */
static int ena_netmap_txsync(struct netmap_kring *kring, int flags) {
	return 0;
}

/* restore kernel and user view of receive ring. */
static int ena_netmap_rxsync(struct netmap_kring *kring, int flags) {
	return 0;
}


/*
 * register/unregister with netmap. This function is called with netmap lock
 * also its only called on the first register or the last unregister.
 */
static int ena_netmap_reg(struct netmap_adapter *na, int onoff) {
	return 0;
}

/*
 * attach routine called from ena driver. This is the main routine which
 * set's up your netmap_attach() parameters.
 */
static void ena_netmap_attach(struct ena_adapter *adapter)
{
	struct netmap_adapter na;

	bzero(&na, sizeof(na));
	na.na_flags = NAF_MOREFRAG;
	na.ifp = adapter->ifp;
	/* adapter can have multiple rx and tx ring so ideally it should be
	 * adapter->rx/tx_ring[x]->ring_size but if you look at the driver
	 * initialization code all are set to requested_rx/tx_ring_size.
	 * todo : user can change the ring size using ethtool interface so we
	 * 	  should hanlde this here. we need a way to update netmap with the
	 * 	  new values. I need to see how netmap uses it, from my guess it
	 * 	  will process descriptor ring's based on these values. So we have to
	 * 	  update if user changes it by ethtool.
	 * Also I need to see what is the ENA_XDP_SUPPORT. It get's enabled on kernel
	 * version > 5.0. XDP has some different mechanism for queues looks like.
	 */
	na.num_tx_desc = adapter->requested_tx_ring_size;
	na.num_rx_desc = adapter->requested_rx_ring_size;
	/* again I don't know what this xdp is bug setting this values based on ena_open() function */
	na.num_tx_rings = adapter->num_io_queues/* + adapter->xdp_num_queues*/;
	na.num_rx_rings = adapter->num_io_queues/* + adapter->xdp_num_queues*/;
	na.rx_buf_maxsize = 1500; // copied from ixgbe driver as per them its overwritten by nm_config

	/* todo : I have to implement these function. */
	na.nm_txsync = ena_netmap_txsync;
	na.nm_rxsync = ena_netmap_rxsync;
	na.nm_register = ena_netmap_reg;

	/* should I implement this?? I don't think so but I might need nm_config implementation.
	na.nm_krings_create = ena_netmap_krings_create;
	na.nm_krings_delete = ena_netmap_krings_delete;
	na.nm_intr = ena_netmap_intr;
	na.nm_config = ena_netmap_config;
	*/

	/* as of now I don't see any need ena driver specific adapter info. so just calling regular
	 *  netmap_attach() to initialize netmap_hw_adapter structure.
	 * if (netmap_attach_ext(&na, sizeof(struct netmap_ena_adapter), 1)) {
	 */
	if (netmap_attach(&na) {
		pr_err("netmap: failed to attach netmap adapter");
		return;
	}

}

static void ena_netmap_detach(struct ena_adapter *adapter)
{
	netmap_detach(adapter->netdev);
}
