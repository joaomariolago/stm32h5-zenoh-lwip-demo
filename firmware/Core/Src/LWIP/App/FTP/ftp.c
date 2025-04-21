

void ftpd_server_init()
{
  err_t err;
  unsigned port = 69;

  /* create a new UDP PCB structure  */
  UDPpcb = udp_new();
  if (UDPpcb)
  {
    /* Bind this PCB to port 69  */
    err = udp_bind(UDPpcb, IP_ADDR_ANY, port);
    if (err == ERR_OK)
    {
      /* TFTP server start  */
      udp_recv(UDPpcb, recv_callback_tftp, NULL);
    }
  }
}
