#!/bin/sh

hciconfig hci0 encrypt auth piscan
hcitool cc <BT_MAC_ADDRESS>
rfcomm bind hci0 <BT_MAC_ADDRESS> 0
 