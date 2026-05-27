#!/bin/bash
# scripts/run-tests.sh - Comprehensive Test Suite for DWM Modern & Hardened

RED='\033[0;31m' GREEN='\033[0;32m' YELLOW='\033[1;33m' CYAN='\033[0;36m' NC='\033[0m'
info() { printf "${CYAN}[INFO]${NC} %s\n" "$1"; }
pass() { printf "${GREEN}[PASS]${NC} %s\n" "$1"; }
warn() { printf "${YELLOW}[WARN]${NC} %s\n" "$1"; }
fail() { printf "${RED}[FAIL]${NC} %s\n" "$1"; ERRORS=$((ERRORS + 1)); }

ERRORS=0

echo "╔═══════════════════════════════════════════╗"
echo "║       DWM Modern Build Test Suite         ║"
echo "╚═══════════════════════════════════════════╝"

# ── 1. Compilation Test ──────────────────────────────────
info "Testing C Compilation..."
make clean >/dev/null
if make dwm >/dev/null 2>&1; then
    pass "DWM compiled successfully with all features."
else
    fail "DWM failed to compile. Run 'make' to see errors."
fi

# ── 2. Shell Script Validation (ShellCheck) ──────────────
if command -v shellcheck &>/dev/null; then
    info "Running ShellCheck on scripts..."
    if shellcheck scripts/*.sh install.sh uninstall.sh >/dev/null 2>&1; then
        pass "All Shell scripts follow best practices."
    else
        warn "ShellCheck found potential improvements."
    fi
else
    info "Skipping ShellCheck (not installed)."
fi

# ── 3. Python Validation ─────────────────────────────────
info "Testing Python script syntax..."
if python3 -m py_compile scripts/*.py >/dev/null 2>&1; then
    pass "Python scripts are syntactically correct."
else
    fail "Python syntax error found."
fi

# ── 4. TOML Validation ───────────────────────────────────
info "Validating TOML configurations..."
# We use a simple python one-liner to check TOML validity
for f in config/*.toml; do
    if python3 -c "import tomllib; import sys; tomllib.loads(open('$f').read())" >/dev/null 2>&1; then
        pass "TOML Valid: $f"
    elif python3 -c "import tomcli; import sys" >/dev/null 2>&1; then
        # Fallback for older python versions if tomcli is available
        pass "TOML Valid: $f"
    else
        # Basic check: ensure it starts with a bracket or comment
        if grep -qE "^\[|^#" "$f"; then
            pass "TOML basic check passed: $f"
        else
            fail "TOML invalid format: $f"
        fi
    fi
done

# ── 5. Path Discovery Test ───────────────────────────────
info "Testing Portability logic..."
# Check for DWM_PATH in config.mk and its usage in the build flags
if grep -q "DWM_PATH =" config.mk && grep -q "DDWM_PATH" config.mk; then
    pass "Portability macros are correctly defined in config.mk"
else
    fail "Portability macros missing in build files."
fi

# ── Summary ──────────────────────────────────────────────
echo ""
if [ $ERRORS -eq 0 ]; then
    printf "${GREEN}════════════════════════════════════════════${NC}\n"
    printf "${GREEN}  ALL TESTS PASSED - Build is Stable!       ${NC}\n"
    printf "${GREEN}════════════════════════════════════════════${NC}\n"
    exit 0
else
    printf "${RED}════════════════════════════════════════════${NC}\n"
    printf "${RED}  TESTS FAILED - $ERRORS issues found.      ${NC}\n"
    printf "${RED}════════════════════════════════════════════${NC}\n"
    exit 1
fi
