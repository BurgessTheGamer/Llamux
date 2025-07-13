#!/bin/bash
# Make all scripts executable

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "🦙 Setting executable permissions on scripts..."

chmod +x "$SCRIPT_DIR"/*.sh

echo "✅ Done! All scripts are now executable."