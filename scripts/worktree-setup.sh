#!/usr/bin/env bash
# Idempotent per-worktree setup for Knight-One.
#
# Run from anywhere inside a checkout (fresh worktrees especially):
#   ./scripts/worktree-setup.sh
#
# Creates only what's missing, borrowing gitignored prerequisites from the
# canonical checkout at ~/dev/KnightOne. Safe to re-run at any time.
#
# Adapted from Apsis's script of the same name. Its submodule steps don't apply
# here, as pandora/ is a git subtree.
set -euo pipefail

MAIN_CHECKOUT="${KNIGHTONE_MAIN_CHECKOUT:-$HOME/dev/KnightOne}"

cd "$(git rev-parse --show-toplevel)"

log() { echo "[worktree-setup] $*"; }

# --- 1. pandora/ext (emsdk + FetchContent deps, ~4 GB, gitignored) ------------
# Shared with the canonical checkout rather than re-downloaded. Note the
# consequence: a dependency bump or setup_emscripten.sh run in either tree is
# seen by both.
if [ ! -e pandora/ext ]; then
    if [ -e "$MAIN_CHECKOUT/pandora/ext" ]; then
        log "symlinking pandora/ext -> $MAIN_CHECKOUT/pandora/ext"
        ln -s "$MAIN_CHECKOUT/pandora/ext" pandora/ext
    else
        log "WARNING: $MAIN_CHECKOUT/pandora/ext not found; run pandora/scripts/setup_emscripten.sh"
    fi
fi

# --- 2. .env (FORGE_AUTH_KEY_SECRET, gitignored) ------------------------------
if [ ! -f .env ]; then
    if [ -f "$MAIN_CHECKOUT/.env" ]; then
        log "copying .env from $MAIN_CHECKOUT"
        cp "$MAIN_CHECKOUT/.env" .env
    else
        log "WARNING: $MAIN_CHECKOUT/.env not found; forge upload will fail"
    fi
fi

log "done"
