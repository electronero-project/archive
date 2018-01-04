#!/bin/bash

rlwrap mynt-wallet-cli --wallet-file wallet_m --password "" --testnet --trusted-daemon --daemon-address localhost:24090  --log-file wallet_miner.log stop_mining

