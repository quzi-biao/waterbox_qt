#!/bin/bash

set -e

echo "Installing ZeroTier..."

if [ ! -f "zerotier-one.tar.gz" ]; then
    echo "Error: zerotier-one.tar.gz not found"
    echo "Please download ZeroTier package and place it in this directory"
    exit 1
fi

tar -xzf zerotier-one.tar.gz

if [ -f "zerotier-one_*.deb" ]; then
    sudo dpkg -i zerotier-one_*.deb
elif [ -f "zerotier-one-*.rpm" ]; then
    sudo rpm -i zerotier-one-*.rpm
else
    echo "Error: No installation package found"
    exit 1
fi

if [ -f "planet" ]; then
    echo "Installing custom planet file..."
    sudo mkdir -p /var/lib/zerotier-one
    sudo cp planet /var/lib/zerotier-one/
fi

echo "Starting ZeroTier service..."
sudo systemctl enable zerotier-one
sudo systemctl restart zerotier-one

echo "ZeroTier installation completed!"
echo "Use 'sudo zerotier-cli join <NETWORK_ID>' to join a network"
