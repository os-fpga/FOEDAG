#############
#
# Fabric clock assignment
#
#############
# This is fabric clock needed by logic but also primitive core clock
# set_clock_pin -device_clock clk[0] -design_clock clk0 (Physical port name)
# set_clock_pin -device_clock clk[0] -design_clock $clk_buf_$ibuf_clk0 (Original clock primitive out-net to fabric)
set_clock_pin   -device_clock clk[0] -design_clock $clk_buf_$ibuf_clk0

# This is fabric clock needed by primitive core clock
# set_clock_pin -device_clock clk[1] -design_clock clk1 (Physical port name)

# This is fabric clock needed by logic but also primitive core clock
# set_clock_pin -device_clock clk[2] -design_clock clk1 (Physical port name)
# set_clock_pin -device_clock clk[2] -design_clock pll_clk (Original clock primitive out-net to fabric)
set_clock_pin   -device_clock clk[2] -design_clock pll_clk

# This is fabric clock needed by primitive core clock
# set_clock_pin -device_clock clk[3] -design_clock clk1 (Physical port name)

# This is fabric clock needed by primitive core clock
# set_clock_pin -device_clock clk[4] -design_clock BOOT_CLOCK#0 (Physical port name)

# This is fabric clock needed by logic
# set_clock_pin -device_clock clk[5] -design_clock clk2 (Physical port name)
# set_clock_pin -device_clock clk[5] -design_clock $clk_buf_$ibuf_clk2 (Original clock primitive out-net to fabric)
set_clock_pin   -device_clock clk[5] -design_clock $clk_buf_$ibuf_clk2

# This is fabric clock buffer
# set_clock_pin -device_clock clk[6] -design_clock FABRIC_CLKBUF#0 (Physical port name)
# set_clock_pin -device_clock clk[6] -design_clock $fclk_buf_clk0_div (Original clock primitive out-net to fabric)
set_clock_pin   -device_clock clk[6] -design_clock $fclk_buf_clk0_div

# For fabric clock buffer output
# set_clock_out -device_clock clk[0] -design_clock clk0_div
set_clock_out   -device_clock clk[0] -design_clock clk0_div

#############
#
# Each pin mode and location assignment
#
#############
# Clock data from object clk0 port O is not routed to fabric
# Pin      clk0                      :: I_BUF |-> CLK_BUF

# Object clk1 is primitive \PLL but data signal is not defined
# Pin      clk1                      :: I_BUF |-> CLK_BUF |-> PLL

# Clock data from object clk2 port O is not routed to fabric
# Pin      clk2                      :: I_BUF |-> CLK_BUF

# Pin      din                       :: I_BUF |-> I_DELAY
# set_mode MODE_BP_DIR_A_RX          HP_1_20_10P
# set_io   din                       HP_1_20_10P                  --> (original)
set_io     din_delay                 HP_1_20_10P                  -mode          MODE_BP_DIR_A_RX -internal_pin g2f_rx_in[0]_A

# Pin      din_clk2                  :: I_BUF
# set_mode MODE_BP_DIR_A_RX          HR_5_0_0P
# set_io   din_clk2                  HR_5_0_0P                    --> (original)
set_io     $ibuf_din_clk2            HR_5_0_0P                    -mode          MODE_BP_DIR_A_RX -internal_pin g2f_rx_in[0]_A

# Pin      din_serdes                :: I_BUF |-> I_SERDES
# set_mode MODE_RATE_8_A_RX          HR_2_0_0P
# set_io   din_serdes                HR_2_0_0P                    --> (original)
set_io     serdes_data[0]            HR_2_0_0P                    -mode          MODE_RATE_8_A_RX -internal_pin g2f_rx_in[0]_A
set_io     serdes_data[1]            HR_2_0_0P                    -mode          MODE_RATE_8_A_RX -internal_pin g2f_rx_in[1]_A
set_io     serdes_data[2]            HR_2_0_0P                    -mode          MODE_RATE_8_A_RX -internal_pin g2f_rx_in[2]_A
set_io     serdes_data[3]            HR_2_0_0P                    -mode          MODE_RATE_8_A_RX -internal_pin g2f_rx_in[3]_A
set_io     serdes_data[4]            HR_2_0_0P                    -mode          MODE_RATE_8_A_RX -internal_pin g2f_rx_in[4]_A
set_io     serdes_data[5]            HR_2_0_0P                    -mode          MODE_RATE_8_A_RX -internal_pin g2f_rx_in[5]_A
set_io     serdes_data[6]            HR_2_0_0P                    -mode          MODE_RATE_8_A_RX -internal_pin g2f_rx_in[6]_A
set_io     serdes_data[7]            HR_2_0_0P                    -mode          MODE_RATE_8_A_RX -internal_pin g2f_rx_in[7]_A

# Pin      din_serdes_clk_out        :: I_BUF
# set_mode MODE_BP_DIR_A_RX          HR_2_6_3P
# set_io   din_serdes_clk_out        HR_2_6_3P                    --> (original)
set_io     $ibuf_din_serdes_clk_out  HR_2_6_3P                    -mode          MODE_BP_DIR_A_RX -internal_pin g2f_rx_in[0]_A

# Pin location is not assigned
# Pin      enable                    :: I_BUF

# Pin      reset                     :: I_BUF
# set_mode MODE_BP_DIR_A_RX          HP_1_0_0P
# set_io   reset                     HP_1_0_0P                    --> (original)
set_io     $ibuf_reset               HP_1_0_0P                    -mode          MODE_BP_DIR_A_RX -internal_pin g2f_rx_in[0]_A

# Object clk_out is primitive \O_SERDES_CLK but data signal is not defined
# Pin      clk_out                   :: O_SERDES_CLK |-> O_BUFT

# Pin      delay_tap[0]              :: O_BUFT
# set_mode MODE_BP_DIR_A_TX          HR_2_20_10P
# set_io   delay_tap[0]              HR_2_20_10P                  --> (original)
set_io     $obuf_delay_tap[0]        HR_2_20_10P                  -mode          MODE_BP_DIR_A_TX -internal_pin f2g_tx_out[0]_A

# Pin      delay_tap[1]              :: O_BUFT
# set_mode MODE_BP_DIR_A_TX          HR_2_22_11P
# set_io   delay_tap[1]              HR_2_22_11P                  --> (original)
set_io     $obuf_delay_tap[1]        HR_2_22_11P                  -mode          MODE_BP_DIR_A_TX -internal_pin f2g_tx_out[0]_A

# Pin      delay_tap[2]              :: O_BUFT
# set_mode MODE_BP_DIR_A_TX          HR_2_24_12P
# set_io   delay_tap[2]              HR_2_24_12P                  --> (original)
set_io     $obuf_delay_tap[2]        HR_2_24_12P                  -mode          MODE_BP_DIR_A_TX -internal_pin f2g_tx_out[0]_A

# Pin      delay_tap[3]              :: O_BUFT
# set_mode MODE_BP_DIR_A_TX          HR_2_26_13P
# set_io   delay_tap[3]              HR_2_26_13P                  --> (original)
set_io     $obuf_delay_tap[3]        HR_2_26_13P                  -mode          MODE_BP_DIR_A_TX -internal_pin f2g_tx_out[0]_A

# Pin      delay_tap[4]              :: O_BUFT
# set_mode MODE_BP_DIR_A_TX          HR_2_28_14P
# set_io   delay_tap[4]              HR_2_28_14P                  --> (original)
set_io     $obuf_delay_tap[4]        HR_2_28_14P                  -mode          MODE_BP_DIR_A_TX -internal_pin f2g_tx_out[0]_A

# Pin      delay_tap[5]              :: O_BUFT
# set_mode MODE_BP_DIR_A_TX          HR_2_30_15P
# set_io   delay_tap[5]              HR_2_30_15P                  --> (original)
set_io     $obuf_delay_tap[5]        HR_2_30_15P                  -mode          MODE_BP_DIR_A_TX -internal_pin f2g_tx_out[0]_A

# Pin      dout                      :: O_DELAY |-> O_BUFT
# set_mode MODE_BP_DIR_A_TX          HP_2_20_10P
# set_io   dout                      HP_2_20_10P                  --> (original)
set_io     dout_pre_delay            HP_2_20_10P                  -mode          MODE_BP_DIR_A_TX -internal_pin f2g_tx_out[0]_A

# Pin      dout_clk2                 :: O_BUFT
# set_mode MODE_BP_DIR_B_TX          HR_5_1_0N
# set_io   dout_clk2                 HR_5_1_0N                    --> (original)
set_io     $obuf_dout_clk2           HR_5_0_0P                    -mode          MODE_BP_DIR_B_TX -internal_pin f2g_tx_out[5]_A

# Pin      dout_serdes               :: O_SERDES |-> O_BUFT
# set_mode MODE_RATE_8_A_TX          HR_2_2_1P
# set_io   dout_serdes               HR_2_2_1P                    --> (original)
set_io     $auto_540                 HR_2_2_1P                    -mode          MODE_RATE_8_A_TX -internal_pin f2g_tx_out[0]_A
set_io     $auto_541                 HR_2_2_1P                    -mode          MODE_RATE_8_A_TX -internal_pin f2g_tx_out[1]_A
set_io     $auto_542                 HR_2_2_1P                    -mode          MODE_RATE_8_A_TX -internal_pin f2g_tx_out[2]_A
set_io     $auto_543                 HR_2_2_1P                    -mode          MODE_RATE_8_A_TX -internal_pin f2g_tx_out[3]_A
set_io     $auto_544                 HR_2_2_1P                    -mode          MODE_RATE_8_A_TX -internal_pin f2g_tx_out[4]_A
set_io     $auto_545                 HR_2_2_1P                    -mode          MODE_RATE_8_A_TX -internal_pin f2g_tx_out[5]_A
set_io     $auto_546                 HR_2_2_1P                    -mode          MODE_RATE_8_A_TX -internal_pin f2g_tx_out[6]_A
set_io     $auto_547                 HR_2_2_1P                    -mode          MODE_RATE_8_A_TX -internal_pin f2g_tx_out[7]_A

# Pin      dout_serdes_clk_out       :: O_BUFT
# set_mode MODE_BP_DIR_B_TX          HR_2_7_3N
# set_io   dout_serdes_clk_out       HR_2_7_3N                    --> (original)
set_io     $obuf_dout_serdes_clk_out HR_2_6_3P                    -mode          MODE_BP_DIR_B_TX -internal_pin f2g_tx_out[5]_A

# Skip this because 'This is secondary pin. But IO bitstream generation will still make sure it is used in pair. Otherwise the IO bitstream will be invalid'
# Pin      din_n                     :: I_BUF_DS |-> I_DDR

# Pin      din_p                     :: I_BUF_DS |-> I_DDR
# set_mode MODE_BP_DDR_A_RX          HP_1_4_2P
# set_io   din_p                     HP_1_4_2P                    --> (original)
set_io     o_ddr_d[0]                HP_1_4_2P                    -mode          MODE_BP_DDR_A_RX -internal_pin g2f_rx_in[0]_A
set_io     o_ddr_d[1]                HP_1_4_2P                    -mode          MODE_BP_DDR_A_RX -internal_pin g2f_rx_in[1]_A

# Skip this because 'This is secondary pin. But IO bitstream generation will still make sure it is used in pair. Otherwise the IO bitstream will be invalid'
# Pin      dout_n                    :: O_DDR |-> O_BUF_DS

# Pin      dout_p                    :: O_DDR |-> O_BUF_DS
# set_mode MODE_BP_DDR_A_TX          HP_1_8_4P
# set_io   dout_p                    HP_1_8_4P                    --> (original)
set_io     $auto_536                 HP_1_8_4P                    -mode          MODE_BP_DDR_A_TX -internal_pin f2g_tx_out[0]_A
set_io     $auto_537                 HP_1_8_4P                    -mode          MODE_BP_DDR_A_TX -internal_pin f2g_tx_out[1]_A

# Skip this because 'This is secondary pin. But IO bitstream generation will still make sure it is used in pair. Otherwise the IO bitstream will be invalid'
# Pin      dout_osc_n                :: O_DDR |-> O_BUF_DS

# Pin      dout_osc_p                :: O_DDR |-> O_BUF_DS
# set_mode MODE_BP_DDR_A_TX          HP_2_22_11P
# set_io   dout_osc_p                HP_2_22_11P                  --> (original)
set_io     $auto_538                 HP_2_22_11P                  -mode          MODE_BP_DDR_A_TX -internal_pin f2g_tx_out[0]_A
set_io     $auto_539                 HP_2_22_11P                  -mode          MODE_BP_DDR_A_TX -internal_pin f2g_tx_out[1]_A

#############
#
# Internal Control Signals
#
#############
# Module: I_BUF
# LinkedObject: clk0
# Location: HR_1_CC_18_9P
# Port: EN
# Signal: in:f2g_in_en_{A|B}
set_io   $auto_500                        HR_1_CC_18_9P  -mode MODE_BP_DIR_A_RX -internal_pin f2g_in_en_A

# Module: I_BUF
# LinkedObject: clk1
# Location: HP_1_CC_18_9P
# Port: EN
# Signal: in:f2g_in_en_{A|B}
set_io   $auto_501                        HP_1_CC_18_9P  -mode MODE_BP_DIR_A_RX -internal_pin f2g_in_en_A

# Module: PLL
# LinkedObject: clk1
# Location: HP_1_CC_18_9P
# Port: LOCK
# Signal: out:TO_BE_DETERMINED
# Skip reason: User design does not utilize linked-object clk1 wrapped-instance port LOCK

# Module: PLL
# LinkedObject: clk1
# Location: HP_1_CC_18_9P
# Port: PLL_EN
# Signal: in:TO_BE_DETERMINED
# Skip reason: TO_BE_DETERMINED
# set_io $auto_534                        HP_1_CC_18_9P  -mode MODE_BP_DIR_A_RX -internal_pin TO_BE_DETERMINED

# Module: I_BUF
# LinkedObject: clk2
# Location: HR_5_CC_38_19P
# Port: EN
# Signal: in:f2g_in_en_{A|B}
set_io   $auto_502                        HR_5_CC_38_19P -mode MODE_BP_DIR_A_RX -internal_pin f2g_in_en_A

# Module: I_BUF
# LinkedObject: din
# Location: HP_1_20_10P
# Port: EN
# Signal: in:f2g_in_en_{A|B}
set_io   $auto_503                        HP_1_20_10P    -mode MODE_BP_DIR_A_RX -internal_pin f2g_in_en_A

# Module: I_DELAY
# LinkedObject: din
# Location: HP_1_20_10P
# Port: DLY_ADJ
# Signal: in:rule=half-first:f2g_trx_dly_adj
# Remap location from HP_1_20_10P to HP_1_20_10P
set_io   $auto_521                        HP_1_20_10P    -mode MODE_BP_DIR_A_RX -internal_pin f2g_trx_dly_adj

# Module: I_DELAY
# LinkedObject: din
# Location: HP_1_20_10P
# Port: DLY_INCDEC
# Signal: in:rule=half-first:f2g_trx_dly_inc
# Remap location from HP_1_20_10P to HP_1_20_10P
set_io   $auto_522                        HP_1_20_10P    -mode MODE_BP_DIR_A_RX -internal_pin f2g_trx_dly_inc

# Module: I_DELAY
# LinkedObject: din
# Location: HP_1_20_10P
# Port: DLY_LOAD
# Signal: in:rule=half-first:f2g_trx_dly_ld
# Remap location from HP_1_20_10P to HP_1_20_10P
set_io   $auto_523                        HP_1_20_10P    -mode MODE_BP_DIR_A_RX -internal_pin f2g_trx_dly_ld

# Module: I_DELAY
# LinkedObject: din
# Location: HP_1_20_10P
# Port: DLY_TAP_VALUE
# Signal: out:rule=half-first:g2f_trx_dly_tap
# Remap location from HP_1_20_10P to HP_1_20_10P
set_io   $ifab_$obuf_delay_tap[0]         HP_1_20_10P    -mode MODE_BP_DIR_A_RX -internal_pin g2f_trx_dly_tap[0]
set_io   $ifab_$obuf_delay_tap[1]         HP_1_20_10P    -mode MODE_BP_DIR_A_RX -internal_pin g2f_trx_dly_tap[1]
set_io   $ifab_$obuf_delay_tap[2]         HP_1_20_10P    -mode MODE_BP_DIR_A_RX -internal_pin g2f_trx_dly_tap[2]
set_io   $ifab_$obuf_delay_tap[3]         HP_1_20_10P    -mode MODE_BP_DIR_A_RX -internal_pin g2f_trx_dly_tap[3]
set_io   $ifab_$obuf_delay_tap[4]         HP_1_20_10P    -mode MODE_BP_DIR_A_RX -internal_pin g2f_trx_dly_tap[4]
set_io   $ifab_$obuf_delay_tap[5]         HP_1_20_10P    -mode MODE_BP_DIR_A_RX -internal_pin g2f_trx_dly_tap[5]

# Module: I_BUF
# LinkedObject: din_clk2
# Location: HR_5_0_0P
# Port: EN
# Signal: in:f2g_in_en_{A|B}
set_io   $auto_504                        HR_5_0_0P      -mode MODE_BP_DIR_A_RX -internal_pin f2g_in_en_A

# Module: I_BUF
# LinkedObject: din_serdes
# Location: HR_2_0_0P
# Port: EN
# Signal: in:f2g_in_en_{A|B}
set_io   $auto_505                        HR_2_0_0P      -mode MODE_RATE_8_A_RX -internal_pin f2g_in_en_A

# Module: I_SERDES
# LinkedObject: din_serdes
# Location: HR_2_0_0P
# Port: BITSLIP_ADJ
# Signal: in:rule=half-first:f2g_rx_bitslip_adj
# Remap location from HR_2_0_0P to HR_2_0_0P
set_io   $auto_524                        HR_2_0_0P      -mode MODE_RATE_8_A_RX -internal_pin f2g_rx_bitslip_adj

# Module: I_SERDES
# LinkedObject: din_serdes
# Location: HR_2_0_0P
# Port: DATA_VALID
# Signal: out:g2f_rx_dvalid_{A|B}
# Skip reason: User design does not utilize linked-object din_serdes wrapped-instance port DATA_VALID

# Module: I_SERDES
# LinkedObject: din_serdes
# Location: HR_2_0_0P
# Port: DPA_ERROR
# Signal: out:rule=half-first:g2f_rx_dpa_error
# Skip reason: User design does not utilize linked-object din_serdes wrapped-instance port DPA_ERROR

# Module: I_SERDES
# LinkedObject: din_serdes
# Location: HR_2_0_0P
# Port: DPA_LOCK
# Signal: out:rule=half-first:g2f_rx_dpa_lock
# Skip reason: User design does not utilize linked-object din_serdes wrapped-instance port DPA_LOCK

# Module: I_SERDES
# LinkedObject: din_serdes
# Location: HR_2_0_0P
# Port: EN
# Signal: in:TO_BE_DETERMINED
# Skip reason: TO_BE_DETERMINED
# set_io $auto_525                        HR_2_0_0P      -mode MODE_RATE_8_A_RX -internal_pin TO_BE_DETERMINED

# Module: I_SERDES
# LinkedObject: din_serdes
# Location: HR_2_0_0P
# Port: RST
# Signal: in:f2g_trx_reset_n_{A|B}
set_io   $auto_526                        HR_2_0_0P      -mode MODE_RATE_8_A_RX -internal_pin f2g_trx_reset_n_A

# Module: I_BUF
# LinkedObject: din_serdes_clk_out
# Location: HR_2_6_3P
# Port: EN
# Signal: in:f2g_in_en_{A|B}
set_io   $auto_506                        HR_2_6_3P      -mode MODE_BP_DIR_A_RX -internal_pin f2g_in_en_A

# Module: I_BUF
# LinkedObject: enable
# Location: 
# Port: EN
# Signal: in:f2g_in_en_{A|B}
# Skip reason: Location  does not have any mode to begin with

# Module: I_BUF
# LinkedObject: reset
# Location: HP_1_0_0P
# Port: EN
# Signal: in:f2g_in_en_{A|B}
set_io   $auto_508                        HP_1_0_0P      -mode MODE_BP_DIR_A_RX -internal_pin f2g_in_en_A

# Module: O_BUFT
# LinkedObject: clk_out
# Location: HR_2_4_2P
# Port: T
# Signal: in:f2g_tx_oe_{A|B}
set_io   $auto_509                        HR_2_4_2P      -mode MODE_BP_SDR_A_TX -internal_pin f2g_tx_oe_A

# Module: O_SERDES_CLK
# LinkedObject: clk_out
# Location: HR_2_4_2P
# Port: CLK_EN
# Signal: in:f2g_tx_clk_en_{A|B} 
set_io   $auto_533                        HR_2_4_2P      -mode MODE_BP_SDR_A_TX -internal_pin f2g_tx_clk_en_A 

# Module: O_BUFT
# LinkedObject: delay_tap[0]
# Location: HR_2_20_10P
# Port: T
# Signal: in:f2g_tx_oe_{A|B}
set_io   $auto_510                        HR_2_20_10P    -mode MODE_BP_DIR_A_TX -internal_pin f2g_tx_oe_A

# Module: O_BUFT
# LinkedObject: delay_tap[1]
# Location: HR_2_22_11P
# Port: T
# Signal: in:f2g_tx_oe_{A|B}
set_io   $auto_511                        HR_2_22_11P    -mode MODE_BP_DIR_A_TX -internal_pin f2g_tx_oe_A

# Module: O_BUFT
# LinkedObject: delay_tap[2]
# Location: HR_2_24_12P
# Port: T
# Signal: in:f2g_tx_oe_{A|B}
set_io   $auto_512                        HR_2_24_12P    -mode MODE_BP_DIR_A_TX -internal_pin f2g_tx_oe_A

# Module: O_BUFT
# LinkedObject: delay_tap[3]
# Location: HR_2_26_13P
# Port: T
# Signal: in:f2g_tx_oe_{A|B}
set_io   $auto_513                        HR_2_26_13P    -mode MODE_BP_DIR_A_TX -internal_pin f2g_tx_oe_A

# Module: O_BUFT
# LinkedObject: delay_tap[4]
# Location: HR_2_28_14P
# Port: T
# Signal: in:f2g_tx_oe_{A|B}
set_io   $auto_514                        HR_2_28_14P    -mode MODE_BP_DIR_A_TX -internal_pin f2g_tx_oe_A

# Module: O_BUFT
# LinkedObject: delay_tap[5]
# Location: HR_2_30_15P
# Port: T
# Signal: in:f2g_tx_oe_{A|B}
set_io   $auto_515                        HR_2_30_15P    -mode MODE_BP_DIR_A_TX -internal_pin f2g_tx_oe_A

# Module: O_BUFT
# LinkedObject: dout
# Location: HP_2_20_10P
# Port: T
# Signal: in:f2g_tx_oe_{A|B}
set_io   $auto_516                        HP_2_20_10P    -mode MODE_BP_DIR_A_TX -internal_pin f2g_tx_oe_A

# Module: O_DELAY
# LinkedObject: dout
# Location: HP_2_20_10P
# Port: DLY_ADJ
# Signal: in:rule=half-first:f2g_trx_dly_adj
# Remap location from HP_2_20_10P to HP_2_20_10P
set_io   $auto_527                        HP_2_20_10P    -mode MODE_BP_DIR_A_TX -internal_pin f2g_trx_dly_adj

# Module: O_DELAY
# LinkedObject: dout
# Location: HP_2_20_10P
# Port: DLY_INCDEC
# Signal: in:rule=half-first:f2g_trx_dly_inc
# Remap location from HP_2_20_10P to HP_2_20_10P
set_io   $auto_528                        HP_2_20_10P    -mode MODE_BP_DIR_A_TX -internal_pin f2g_trx_dly_inc

# Module: O_DELAY
# LinkedObject: dout
# Location: HP_2_20_10P
# Port: DLY_LOAD
# Signal: in:rule=half-first:f2g_trx_dly_ld
# Remap location from HP_2_20_10P to HP_2_20_10P
set_io   $auto_529                        HP_2_20_10P    -mode MODE_BP_DIR_A_TX -internal_pin f2g_trx_dly_ld

# Module: O_DELAY
# LinkedObject: dout
# Location: HP_2_20_10P
# Port: DLY_TAP_VALUE
# Signal: out:rule=half-first:g2f_trx_dly_tap
# Skip reason: User design does not utilize linked-object dout wrapped-instance port DLY_TAP_VALUE

# Module: O_BUFT
# LinkedObject: dout_clk2
# Location: HR_5_1_0N
# Port: T
# Signal: in:f2g_tx_oe_{A|B}
set_io   $auto_517                        HR_5_1_0N      -mode MODE_BP_DIR_B_TX -internal_pin f2g_tx_oe_B

# Module: O_BUFT
# LinkedObject: dout_serdes
# Location: HR_2_2_1P
# Port: T
# Signal: in:f2g_tx_oe_{A|B}
set_io   $auto_518                        HR_2_2_1P      -mode MODE_RATE_8_A_TX -internal_pin f2g_tx_oe_A

# Module: O_SERDES
# LinkedObject: dout_serdes
# Location: HR_2_2_1P
# Port: CHANNEL_BOND_SYNC_IN
# Signal: in:TO_BE_DETERMINED
# Skip reason: User design does not utilize linked-object dout_serdes wrapped-instance port CHANNEL_BOND_SYNC_IN

# Module: O_SERDES
# LinkedObject: dout_serdes
# Location: HR_2_2_1P
# Port: CHANNEL_BOND_SYNC_OUT
# Signal: out:TO_BE_DETERMINED
# Skip reason: User design does not utilize linked-object dout_serdes wrapped-instance port CHANNEL_BOND_SYNC_OUT

# Module: O_SERDES
# LinkedObject: dout_serdes
# Location: HR_2_2_1P
# Port: DATA_VALID
# Signal: in:f2g_tx_dvalid_{A|B}
set_io   $auto_530                        HR_2_2_1P      -mode MODE_RATE_8_A_TX -internal_pin f2g_tx_dvalid_A

# Module: O_SERDES
# LinkedObject: dout_serdes
# Location: HR_2_2_1P
# Port: OE_IN
# Signal: in:TO_BE_DETERMINED
# Skip reason: TO_BE_DETERMINED
# set_io $auto_531                        HR_2_2_1P      -mode MODE_RATE_8_A_TX -internal_pin TO_BE_DETERMINED

# Module: O_SERDES
# LinkedObject: dout_serdes
# Location: HR_2_2_1P
# Port: OE_OUT
# Signal: out:TO_BE_DETERMINED
# Skip reason: User design does not utilize linked-object dout_serdes wrapped-instance port OE_OUT

# Module: O_SERDES
# LinkedObject: dout_serdes
# Location: HR_2_2_1P
# Port: PLL_LOCK
# Signal: in:TO_BE_DETERMINED
# Skip reason: User design does not utilize linked-object dout_serdes wrapped-instance port PLL_LOCK

# Module: O_SERDES
# LinkedObject: dout_serdes
# Location: HR_2_2_1P
# Port: RST
# Signal: in:f2g_trx_reset_n_{A|B}
set_io   $auto_532                        HR_2_2_1P      -mode MODE_RATE_8_A_TX -internal_pin f2g_trx_reset_n_A

# Module: O_BUFT
# LinkedObject: dout_serdes_clk_out
# Location: HR_2_7_3N
# Port: T
# Signal: in:f2g_tx_oe_{A|B}
set_io   $auto_519                        HR_2_7_3N      -mode MODE_BP_DIR_B_TX -internal_pin f2g_tx_oe_B

# Module: PLL
# LinkedObject: BOOT_CLOCK#0
# Location: 
# Port: LOCK
# Signal: out:TO_BE_DETERMINED
# Skip reason: Location  does not have any mode to begin with

# Module: PLL
# LinkedObject: BOOT_CLOCK#0
# Location: 
# Port: PLL_EN
# Signal: in:TO_BE_DETERMINED
# Skip reason: Location  does not have any mode to begin with

# Module: I_BUF_DS
# LinkedObject: din_n+din_p
# Location: HP_1_4_2P
# Port: EN
# Signal: in:f2g_in_en_{A|B}
set_io   $auto_520                        HP_1_4_2P      -mode MODE_BP_DDR_A_RX -internal_pin f2g_in_en_A

# Module: I_DDR
# LinkedObject: din_n+din_p
# Location: HP_1_4_2P
# Port: E
# Signal: in:TO_BE_DETERMINED
# Skip reason: TO_BE_DETERMINED
# set_io $ofab_$ibuf_enable_4             HP_1_4_2P      -mode MODE_BP_DDR_A_RX -internal_pin TO_BE_DETERMINED

# Module: I_DDR
# LinkedObject: din_n+din_p
# Location: HP_1_4_2P
# Port: R
# Signal: in:TO_BE_DETERMINED
# Skip reason: TO_BE_DETERMINED
# set_io $f2g_trx_reset_n_A_$ibuf_reset_4 HP_1_4_2P      -mode MODE_BP_DDR_A_RX -internal_pin TO_BE_DETERMINED

# Module: O_DDR
# LinkedObject: dout_n+dout_p
# Location: HP_1_8_4P
# Port: E
# Signal: in:TO_BE_DETERMINED
# Skip reason: TO_BE_DETERMINED
# set_io $ofab_$ibuf_enable               HP_1_8_4P      -mode MODE_BP_DDR_A_TX -internal_pin TO_BE_DETERMINED

# Module: O_DDR
# LinkedObject: dout_n+dout_p
# Location: HP_1_8_4P
# Port: R
# Signal: in:TO_BE_DETERMINED
# Skip reason: TO_BE_DETERMINED
# set_io $f2g_trx_reset_n_A_$ibuf_reset   HP_1_8_4P      -mode MODE_BP_DDR_A_TX -internal_pin TO_BE_DETERMINED

# Module: O_DDR
# LinkedObject: dout_osc_n+dout_osc_p
# Location: HP_2_22_11P
# Port: E
# Signal: in:TO_BE_DETERMINED
# Skip reason: TO_BE_DETERMINED
# set_io $ofab_$ibuf_enable_2             HP_2_22_11P    -mode MODE_BP_DDR_A_TX -internal_pin TO_BE_DETERMINED

# Module: O_DDR
# LinkedObject: dout_osc_n+dout_osc_p
# Location: HP_2_22_11P
# Port: R
# Signal: in:TO_BE_DETERMINED
# Skip reason: TO_BE_DETERMINED
# set_io $f2g_trx_reset_n_A_$ibuf_reset_2 HP_2_22_11P    -mode MODE_BP_DDR_A_TX -internal_pin TO_BE_DETERMINED

#############
#
# Each gearbox core clock
#
#############
# Clock module: CLK_BUF
# Clock module name: $clkbuf$top.$ibuf_clk0
# Clock port: O
# Clock port net: $clk_buf_$ibuf_clk0
# Primitive module: $ibuf$top.$ibuf_din
# Location: HP_1_20_10P
set_core_clk HP_1_20_10P 0

# Clock module: CLK_BUF
# Clock module name: clk_buf
# Clock port: O
# Clock port net: clk1_buf
# Primitive module: $obuf$top.$obuf_dout
# Location: HP_2_20_10P
set_core_clk HP_2_20_10P 1

# Clock module: PLL
# Clock module name: pll
# Clock port: CLK_OUT
# Clock port net: pll_clk
# Primitive module: $ibuf$top.$ibuf_din_serdes
# Location: HR_2_0_0P
set_core_clk HR_2_0_0P   2

# Clock module: PLL
# Clock module name: pll
# Clock port: CLK_OUT
# Clock port net: pll_clk
# Primitive module: $obuf$top.$obuf_dout_serdes
# Location: HR_2_2_1P
set_core_clk HR_2_2_1P   2

# Clock module: PLL
# Clock module name: pll
# Clock port: CLK_OUT
# Clock port net: pll_clk
# Primitive module: i_buf_ds
# Location: HP_1_4_2P
set_core_clk HP_1_4_2P   3

# Clock module: PLL
# Clock module name: pll
# Clock port: CLK_OUT
# Clock port net: pll_clk
# Primitive module: o_buf_ds
# Location: HP_1_8_4P
set_core_clk HP_1_8_4P   3

# Clock module: PLL
# Clock module name: pll_osc
# Clock port: CLK_OUT
# Clock port net: osc_pll
# Primitive module: o_buf_ds_osc
# Location: HP_2_22_11P
set_core_clk HP_2_22_11P 4

