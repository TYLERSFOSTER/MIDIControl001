#!/usr/bin/env bash
# ============================================================
# MIDIControl001 — Post-build analyzer (Phase 4-E → Final)
# ============================================================

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
LOG_ANALYZER="$ROOT_DIR/tools/analyze_logs.py"
VENV_PYTHON="$ROOT_DIR/.venv/bin/python"
SYSTEM_PYTHON="$(command -v python3 || command -v python)"
REPORT_FILE="$ROOT_DIR/report_summary.txt"

echo "=== Running post-build diagnostics ==="

# Prefer local venv Python if it exists
if [[ -x "$VENV_PYTHON" ]]; then
  PY="$VENV_PYTHON"
  echo "Using local virtualenv Python."
else
  PY="$SYSTEM_PYTHON"
  echo "Using system Python."
fi

# Ensure dependencies quietly
if ! "$PY" - <<'PY' >/dev/null 2>&1
import importlib
for m in ("pandas","numpy"): importlib.import_module(m)
PY
then
  echo "Installing missing Python deps (pandas, numpy)..."
  "$PY" -m pip install --quiet --upgrade pip
  "$PY" -m pip install --quiet pandas numpy
fi

# ------------------------------------------------------------
# Run analyzer (never fail the build)
# ------------------------------------------------------------
if "$PY" "$LOG_ANALYZER" >/dev/null 2>&1; then
  status=$?
else
  status=$?
fi

case $status in
  0)  echo "✅ Analyzer clean (no anomalies)." ;;
  1)  echo "⚠️  Analyzer warnings (flat RMS)." ;;
  2)  echo "🔥 Analyzer found clipping/non-fatal anomalies." ;;
  99) echo "❌ Analyzer crashed or returned unexpected code (99)." ;;
  *)  echo "❌ Analyzer returned unknown exit code $status." ;;
esac

# ------------------------------------------------------------
# Highlight anomalies if present
# ------------------------------------------------------------
if [[ -f "$REPORT_FILE" ]]; then
  if grep -E '🔥|⚠️' "$REPORT_FILE"; then
    echo "⚠️  See $REPORT_FILE for details."
  else
    echo "✅ DSP diagnostics clean — no anomalies detected."
  fi
fi

# Always succeed so CMake continues
exit 0
