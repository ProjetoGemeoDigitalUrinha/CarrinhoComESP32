#!/bin/bash

# CONFIGURATION
HOSTNAME="twinesp32.local"
RESERVED_SHARE_NAME="digitaltwinmaria"

echo "🔍 Resolving IP for $HOSTNAME..."

# Use ping or avahi to resolve the IP address
IP=$(getent hosts "$HOSTNAME" | awk '{ print $1 }')

# Fallback if getent fails (Linux only)
if [ -z "$IP" ]; then
    if command -v avahi-resolve &> /dev/null; then
        IP=$(avahi-resolve -n "$HOSTNAME" | awk '{ print $2 }')
    fi
fi

# Fail if no IP found
if [ -z "$IP" ]; then
    echo "❌ Failed to resolve IP for $HOSTNAME"
    exit 1
fi

echo "✅ Resolved $HOSTNAME to $IP"

# Start zrok reserved share using the resolved IP
echo "🚀 Launching zrok reserved share: $RESERVED_SHARE_NAME"
zrok reserve public -n "$RESERVED_SHARE_NAME" $IP
zrok share reserved "$RESERVED_SHARE_NAME"
