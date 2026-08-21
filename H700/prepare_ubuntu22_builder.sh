#!/bin/sh
set -eu

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"

MPL_H700_PREPARE_BUILDER_ONLY=1 sh "$SCRIPT_DIR/build_in_ubuntu22.sh"
